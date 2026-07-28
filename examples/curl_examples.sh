#!/usr/bin/env bash
# Snooker Player 3.0 - Exemples cURL
# ====================================
# Ces exemples supposent que le serveur tourne sur localhost:8080
# Démarrer le serveur : ./build/snooker_server 8080

SERVER="http://localhost:8080"

echo "=== Snooker Player 3.0 - Exemples cURL ==="
echo ""

# -------------------------------------------------------------------
# 1. Sante du serveur
# -------------------------------------------------------------------
echo "--- 1. Sante du serveur ---"
curl -s "$SERVER/health" | python3 -m json.tool 2>/dev/null || \
curl -s "$SERVER/health"
echo ""

# -------------------------------------------------------------------
# 2. Etat courant du jeu
# -------------------------------------------------------------------
echo "--- 2. Etat du jeu ---"
curl -s "$SERVER/api/game/state" | python3 -m json.tool 2>/dev/null || \
curl -s "$SERVER/api/game/state"
echo ""

# -------------------------------------------------------------------
# 3. Jouer un coup (Rouge)
# -------------------------------------------------------------------
echo "--- 3. Jouer Rouge (1 pt) ---"
curl -s -X POST "$SERVER/api/game/shot" \
  -H "Content-Type: application/json" \
  -d '{"ball_name":"Rouge","ball_value":1}'
echo ""

# -------------------------------------------------------------------
# 4. Jouer un coup (Noir = couleur apres rouge)
# -------------------------------------------------------------------
echo "--- 4. Jouer Noir (7 pts) ---"
curl -s -X POST "$SERVER/api/game/shot" \
  -H "Content-Type: application/json" \
  -d '{"ball_name":"Noir","ball_value":7}'
echo ""

# -------------------------------------------------------------------
# 5. Jouer un autre coup (Vert)
# -------------------------------------------------------------------
echo "--- 5. Jouer Rouge puis Vert (3 pts) ---"
curl -s -X POST "$SERVER/api/game/shot" \
  -H "Content-Type: application/json" \
  -d '{"ball_name":"Rouge","ball_value":1}'
echo ""
curl -s -X POST "$SERVER/api/game/shot" \
  -H "Content-Type: application/json" \
  -d '{"ball_name":"Vert","ball_value":3}'
echo ""

# -------------------------------------------------------------------
# 6. Declarer une faute (penalite 4 pts)
# -------------------------------------------------------------------
echo "--- 6. Declarer une faute (4 pts) ---"
curl -s -X POST "$SERVER/api/game/foul" \
  -H "Content-Type: application/json" \
  -d '{"points":4}'
echo ""

# -------------------------------------------------------------------
# 7. Etat apres les coups
# -------------------------------------------------------------------
echo "--- 7. Etat du jeu apres les coups ---"
curl -s "$SERVER/api/game/state" | python3 -m json.tool 2>/dev/null || \
curl -s "$SERVER/api/game/state"
echo ""

# -------------------------------------------------------------------
# 8. Historique des coups
# -------------------------------------------------------------------
echo "--- 8. Historique des coups ---"
curl -s "$SERVER/api/game/history"
echo ""

# -------------------------------------------------------------------
# 9. Envoyer des detections de camera (Camera 1)
# -------------------------------------------------------------------
echo "--- 9. Camera 1 : detection de billes ---"
curl -s -X POST "$SERVER/api/camera/1/frame" \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp_ms": 1700000000000,
    "detections": [
      {"color":"Blanche","x_px":320,"y_px":360,"confidence":0.95},
      {"color":"Rouge",  "x_px":400,"y_px":200,"confidence":0.90},
      {"color":"Noir",   "x_px":640,"y_px":100,"confidence":0.88}
    ]
  }'
echo ""

# -------------------------------------------------------------------
# 10. Envoyer des detections de camera (Camera 2)
# -------------------------------------------------------------------
echo "--- 10. Camera 2 : detection de billes ---"
curl -s -X POST "$SERVER/api/camera/2/frame" \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp_ms": 1700000000010,
    "detections": [
      {"color":"Bleu","x_px":640,"y_px":360,"confidence":0.92},
      {"color":"Rose","x_px":500,"y_px":250,"confidence":0.87}
    ]
  }'
echo ""

# -------------------------------------------------------------------
# 11. Envoyer des detections de camera (Camera 3)
# -------------------------------------------------------------------
echo "--- 11. Camera 3 : detection de billes ---"
curl -s -X POST "$SERVER/api/camera/3/frame" \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp_ms": 1700000000020,
    "detections": [
      {"color":"Noir","x_px":640,"y_px":100,"confidence":0.91}
    ]
  }'
echo ""

# -------------------------------------------------------------------
# 12. Etat fusionne des 3 cameras
# -------------------------------------------------------------------
echo "--- 12. Etat fusionne des cameras ---"
curl -s "$SERVER/api/camera/fused" | python3 -m json.tool 2>/dev/null || \
curl -s "$SERVER/api/camera/fused"
echo ""

# -------------------------------------------------------------------
# 13. Mode simulation (demo automatique)
# -------------------------------------------------------------------
echo "--- 13. Simulation demo ---"
curl -s "$SERVER/api/simulate/demo" | python3 -m json.tool 2>/dev/null || \
curl -s "$SERVER/api/simulate/demo"
echo ""

echo "=== Fin des exemples ==="
