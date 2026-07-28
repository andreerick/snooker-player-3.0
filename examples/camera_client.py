#!/usr/bin/env python3
"""
Snooker Player 3.0 - Client Camera Python
==========================================

Client Python pour envoyer des frames de detection de billes
au serveur API Snooker Player 3.0.

Supporte :
  - Webcam USB (via OpenCV)
  - Simulation (sans camera reelle)
  - Envoi des detections via HTTP POST

Configuration materielle :
  3 cameras plongeantes (vue a 90 degres) alignees sur la longueur :
  
    CAM 1 (~595mm)    CAM 2 (1785mm)    CAM 3 (~2975mm)
         |                  |                 |
  ┌──────────────────────────────────────────────────┐
  │              TABLE 3570 × 1770 mm                │
  └──────────────────────────────────────────────────┘

Usage :
  python3 camera_client.py                    # Mode simulation
  python3 camera_client.py --camera 1         # Camera 1 (USB device 0)
  python3 camera_client.py --camera 2 --device 1  # Camera 2 (USB device 1)
  python3 camera_client.py --server http://192.168.1.10:8080
"""

import argparse
import json
import time
import sys
import math
import requests

# Detection OpenCV optionnelle
try:
    import cv2
    import numpy as np
    OPENCV_AVAILABLE = True
except ImportError:
    OPENCV_AVAILABLE = False
    print("[INFO] OpenCV non disponible - mode simulation uniquement")


# -------------------------------------------------------------------
# Configuration
# -------------------------------------------------------------------

TABLE_LENGTH_MM = 3570.0
TABLE_WIDTH_MM  = 1770.0
CAMERA_HEIGHT_MM = 1200.0

# Couleurs des billes en HSV (pour detection OpenCV)
# Format : (H_min, H_max, S_min, S_max, V_min, V_max)
BALL_COLORS_HSV = {
    "Rouge":  (0,   10,  100, 255, 100, 255),
    "Jaune":  (20,  35,  100, 255, 100, 255),
    "Vert":   (40,  80,  80,  255, 60,  255),
    "Marron": (10,  20,  80,  255, 60,  200),
    "Bleu":   (100, 130, 80,  255, 80,  255),
    "Rose":   (150, 170, 80,  255, 150, 255),
    "Noir":   (0,   180, 0,   50,  0,   50),
    "Blanche":(0,   180, 0,   50,  200, 255),
}

BALL_RADIUS_MM = 26.0  # Rayon officiel bille snooker = 26.25mm


# -------------------------------------------------------------------
# Calibration Camera (vue plongeante)
# -------------------------------------------------------------------

class CameraCalibration:
    """
    Calibration simple pour vue plongeante (overhead a 90 degres).
    Conversion pixel <-> mm lineaire.
    """

    def __init__(self, camera_id: int, image_width: int = 1280,
                 image_height: int = 720):
        self.camera_id    = camera_id
        self.image_width  = image_width
        self.image_height = image_height
        self.scale_x      = None  # pixels / mm
        self.scale_y      = None
        self.origin_x     = 0.0
        self.origin_y     = 0.0

        # Zone couverte par cette camera
        spacing = TABLE_LENGTH_MM / 3.0
        overlap = 200.0

        if camera_id == 1:
            self.zone_start = 0.0
            self.zone_end   = spacing + overlap
        elif camera_id == 2:
            self.zone_start = spacing - overlap
            self.zone_end   = 2.0 * spacing + overlap
        else:  # camera_id == 3
            self.zone_start = 2.0 * spacing - overlap
            self.zone_end   = TABLE_LENGTH_MM

        self._calibrate_default()

    def _calibrate_default(self):
        """Calibration par defaut (suppose que la camera couvre toute la largeur)."""
        zone_length = self.zone_end - self.zone_start
        self.scale_x = self.image_width  / TABLE_WIDTH_MM
        self.scale_y = self.image_height / zone_length
        self.origin_x = 0.0
        self.origin_y = self.image_height + self.zone_start * self.scale_y

    def calibrate_from_markers(self, markers: list):
        """
        Calibration a partir de marqueurs connus sur la table.
        markers = [(x_px, y_px, x_mm, y_mm), ...]
        """
        if len(markers) < 2:
            print("[WARN] Au moins 2 marqueurs requis pour la calibration")
            return

        # Calcul simple de l'echelle
        dx_px = markers[1][0] - markers[0][0]
        dx_mm = markers[1][2] - markers[0][2]
        dy_px = markers[1][1] - markers[0][1]
        dy_mm = markers[1][3] - markers[0][3]

        if abs(dx_mm) > 10:
            self.scale_x = dx_px / dx_mm
        if abs(dy_mm) > 10:
            self.scale_y = abs(dy_px / dy_mm)

        self.origin_x = markers[0][0] - markers[0][2] * self.scale_x
        self.origin_y = markers[0][1] + markers[0][3] * self.scale_y

        print(f"[CAL] Camera {self.camera_id}: "
              f"scale_x={self.scale_x:.3f} px/mm, "
              f"scale_y={self.scale_y:.3f} px/mm")

    def pixel_to_mm(self, x_px: float, y_px: float):
        """Convertit des coordonnees pixel en mm sur la table."""
        x_mm = (x_px - self.origin_x) / self.scale_x
        y_mm = (self.origin_y - y_px) / self.scale_y
        return x_mm, y_mm

    def mm_to_pixel(self, x_mm: float, y_mm: float):
        """Convertit des coordonnees mm en pixels."""
        x_px = self.origin_x + x_mm * self.scale_x
        y_px = self.origin_y - y_mm * self.scale_y
        return x_px, y_px


# -------------------------------------------------------------------
# Detection de billes (OpenCV)
# -------------------------------------------------------------------

def detect_balls_opencv(frame, calibration: CameraCalibration) -> list:
    """
    Detectiondes billes par couleur avec OpenCV.
    Retourne une liste de {'color', 'x_px', 'y_px', 'confidence'}
    """
    if not OPENCV_AVAILABLE:
        return []

    detections = []
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    for color_name, (h_min, h_max, s_min, s_max, v_min, v_max) in BALL_COLORS_HSV.items():
        lower = np.array([h_min, s_min, v_min])
        upper = np.array([h_max, s_max, v_max])
        mask  = cv2.inRange(hsv, lower, upper)

        # Erosion/dilatation pour reduire le bruit
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
        mask   = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

        # Detection des cercles (Hough)
        circles = cv2.HoughCircles(
            mask, cv2.HOUGH_GRADIENT, dp=1,
            minDist=30, param1=50, param2=15,
            minRadius=10, maxRadius=40
        )

        if circles is not None:
            circles = np.uint16(np.around(circles))
            for c in circles[0, :]:
                x_px, y_px, r = int(c[0]), int(c[1]), int(c[2])
                # Confiance basee sur la rondeur et la taille
                confidence = min(1.0, 0.7 + 0.3 * (1.0 - abs(r - 20) / 20.0))
                detections.append({
                    "color":      color_name,
                    "x_px":       x_px,
                    "y_px":       y_px,
                    "confidence": round(confidence, 2)
                })

    return detections


# -------------------------------------------------------------------
# Client API
# -------------------------------------------------------------------

class SnookerApiClient:
    """Client HTTP pour l'API Snooker Player 3.0."""

    def __init__(self, server_url: str = "http://localhost:8080"):
        self.server_url = server_url.rstrip("/")
        self.session    = requests.Session()

    def health(self) -> dict:
        """Verifie l'etat du serveur."""
        r = self.session.get(f"{self.server_url}/health", timeout=5)
        return r.json()

    def get_game_state(self) -> dict:
        """Retourne l'etat courant du jeu."""
        r = self.session.get(f"{self.server_url}/api/game/state", timeout=5)
        return r.json()

    def play_shot(self, ball_name: str, ball_value: int) -> dict:
        """Joue un coup."""
        payload = {"ball_name": ball_name, "ball_value": ball_value}
        r = self.session.post(
            f"{self.server_url}/api/game/shot",
            json=payload, timeout=5
        )
        return r.json()

    def submit_camera_frame(self, camera_id: int, detections: list,
                            timestamp_ms: int = None) -> dict:
        """Envoie les detections d'une camera."""
        if timestamp_ms is None:
            timestamp_ms = int(time.time() * 1000)

        payload = {
            "timestamp_ms": timestamp_ms,
            "detections":   detections
        }
        r = self.session.post(
            f"{self.server_url}/api/camera/{camera_id}/frame",
            json=payload, timeout=5
        )
        return r.json()

    def get_fused_state(self) -> dict:
        """Retourne l'etat fusionne des cameras."""
        r = self.session.get(f"{self.server_url}/api/camera/fused", timeout=5)
        return r.json()

    def run_demo(self) -> dict:
        """Lance une simulation de quelques coups."""
        r = self.session.get(f"{self.server_url}/api/simulate/demo", timeout=5)
        return r.json()


# -------------------------------------------------------------------
# Mode simulation (sans camera)
# -------------------------------------------------------------------

def simulate_detections(camera_id: int, frame_num: int) -> list:
    """
    Simule les detections de billes pour tester sans camera reelle.
    Retourne des positions coherentes avec la configuration des cameras.
    """
    spacing = TABLE_LENGTH_MM / 3.0
    overlap = 200.0

    # Centre de la zone de cette camera
    if camera_id == 1:
        y_center = spacing / 2.0
    elif camera_id == 2:
        y_center = TABLE_LENGTH_MM / 2.0
    else:
        y_center = TABLE_LENGTH_MM - spacing / 2.0

    x_center = TABLE_WIDTH_MM / 2.0

    # Positions simulees des billes (avec legere variation)
    t = frame_num * 0.1
    balls = [
        {"color": "Blanche", "x_px": 640 + int(20 * math.sin(t)), "y_px": 360, "confidence": 0.95},
        {"color": "Rouge",   "x_px": 300, "y_px": 200, "confidence": 0.90},
        {"color": "Noir",    "x_px": 640, "y_px": 100, "confidence": 0.88},
    ]
    return balls


# -------------------------------------------------------------------
# Boucle principale
# -------------------------------------------------------------------

def run_camera_loop(camera_id: int, device_id: int, server_url: str,
                    simulate: bool = False):
    """
    Boucle principale : capture -> detection -> envoi API.
    """
    calibration = CameraCalibration(camera_id)
    client      = SnookerApiClient(server_url)

    # Verifier la connexion au serveur
    try:
        health = client.health()
        print(f"[OK] Serveur connecte : {health}")
    except Exception as e:
        print(f"[WARN] Impossible de contacter le serveur ({e})")
        print("[INFO] Les detections seront envoyees sans confirmation")

    cap = None
    if not simulate and OPENCV_AVAILABLE:
        cap = cv2.VideoCapture(device_id)
        if not cap.isOpened():
            print(f"[WARN] Impossible d'ouvrir la camera {device_id}, mode simulation")
            simulate = True

    print(f"[START] Camera {camera_id} - {'Simulation' if simulate else f'Device {device_id}'}")
    print(f"        Zone: [{calibration.zone_start:.0f} - {calibration.zone_end:.0f}]mm")
    print(f"        Serveur: {server_url}")
    print("        Appuyer sur Ctrl+C pour arreter")

    frame_num = 0
    try:
        while True:
            timestamp_ms = int(time.time() * 1000)

            if simulate:
                detections = simulate_detections(camera_id, frame_num)
            elif OPENCV_AVAILABLE and cap is not None:
                ret, frame = cap.read()
                if not ret:
                    print("[WARN] Erreur lecture camera")
                    time.sleep(0.1)
                    continue
                detections = detect_balls_opencv(frame, calibration)
            else:
                detections = []

            if detections:
                try:
                    result = client.submit_camera_frame(
                        camera_id, detections, timestamp_ms
                    )
                    print(f"[{timestamp_ms}] Camera {camera_id}: "
                          f"{len(detections)} bille(s) -> {result.get('message', 'ok')}")
                except Exception as e:
                    print(f"[{timestamp_ms}] Camera {camera_id}: "
                          f"{len(detections)} bille(s) detectee(s) [serveur: {e}]")

            frame_num += 1
            time.sleep(0.1)  # ~10 fps

    except KeyboardInterrupt:
        print("\n[STOP] Camera arretee")
    finally:
        if cap is not None:
            cap.release()


# -------------------------------------------------------------------
# Point d'entree
# -------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Client camera pour Snooker Player 3.0"
    )
    parser.add_argument(
        "--camera", type=int, default=1, choices=[1, 2, 3],
        help="Identifiant de la camera (1, 2 ou 3)"
    )
    parser.add_argument(
        "--device", type=int, default=0,
        help="Index du device video (ex: 0 pour /dev/video0)"
    )
    parser.add_argument(
        "--server", type=str, default="http://localhost:8080",
        help="URL du serveur API"
    )
    parser.add_argument(
        "--simulate", action="store_true",
        help="Mode simulation (sans camera reelle)"
    )
    parser.add_argument(
        "--demo", action="store_true",
        help="Lancer la simulation de demo via l'API"
    )

    args = parser.parse_args()

    if args.demo:
        client = SnookerApiClient(args.server)
        try:
            result = client.run_demo()
            print(json.dumps(result, indent=2, ensure_ascii=False))
        except Exception as e:
            print(f"Erreur: {e}")
        return

    run_camera_loop(
        camera_id  = args.camera,
        device_id  = args.device,
        server_url = args.server,
        simulate   = args.simulate or not OPENCV_AVAILABLE
    )


if __name__ == "__main__":
    main()
