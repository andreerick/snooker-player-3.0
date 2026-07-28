#pragma once

#include <string>
#include <array>

namespace Camera {

// Configuration d'une camera unique
struct CameraConfig
{
    int         id;                // Identifiant (1, 2 ou 3)
    std::string type;              // "usb_webcam" | "phone_wifi" | "ip_camera"

    // Position de la camera sur la table (longueur, en mm)
    float position_mm;            // position sur l'axe longueur (Y)
    float height_mm = 1200.0f;   // hauteur au-dessus de la surface de jeu (mm)

    // Zone de couverture sur l'axe longueur (mm)
    float zone_start_mm;
    float zone_end_mm;
    float overlap_mm = 200.0f;   // chevauchement avec les cameras voisines (mm)

    // Resolution de la camera (pixels)
    int image_width_px  = 1280;
    int image_height_px = 720;

    // URL pour les cameras WiFi / IP
    std::string stream_url;
};

// Configuration du systeme a 3 cameras alignees sur la longueur
//
// Vue de dessus :
//
//    CAM 1 (1,20m)     CAM 2 (1,20m)     CAM 3 (1,20m)
//         ↓                  ↓                 ↓
//  ┌──────────────────────────────────────────────────┐
//  │                  TABLE 3,57m × 1,77m             │
//  └──────────────────────────────────────────────────┘
//  0mm              1785mm                           3570mm
//
struct TripleCameraSystem
{
    static constexpr float TABLE_LENGTH_MM = 3570.0f;
    static constexpr float TABLE_WIDTH_MM  = 1770.0f;
    static constexpr float CAMERA_HEIGHT_MM = 1200.0f;

    // Espacement regulier : ~1190 mm entre cameras
    static constexpr float SPACING_MM = TABLE_LENGTH_MM / 3.0f; // ~1190 mm

    // Positions des 3 cameras sur l'axe longueur
    static constexpr float CAM1_POS_MM = SPACING_MM / 2.0f;        // ~595 mm
    static constexpr float CAM2_POS_MM = TABLE_LENGTH_MM / 2.0f;   // 1785 mm
    static constexpr float CAM3_POS_MM = TABLE_LENGTH_MM - SPACING_MM / 2.0f; // ~2975 mm

    // Zones de couverture avec overlap de 200 mm
    static constexpr float OVERLAP_MM = 200.0f;

    // Retourne la configuration par defaut pour les 3 cameras
    static std::array<CameraConfig, 3> getDefaultConfig()
    {
        std::array<CameraConfig, 3> cameras;

        cameras[0] = {
            .id             = 1,
            .type           = "usb_webcam",
            .position_mm    = CAM1_POS_MM,
            .height_mm      = CAMERA_HEIGHT_MM,
            .zone_start_mm  = 0.0f,
            .zone_end_mm    = SPACING_MM + OVERLAP_MM,
            .overlap_mm     = OVERLAP_MM,
            .image_width_px  = 1280,
            .image_height_px = 720
        };

        cameras[1] = {
            .id             = 2,
            .type           = "usb_webcam",
            .position_mm    = CAM2_POS_MM,
            .height_mm      = CAMERA_HEIGHT_MM,
            .zone_start_mm  = SPACING_MM - OVERLAP_MM,
            .zone_end_mm    = 2.0f * SPACING_MM + OVERLAP_MM,
            .overlap_mm     = OVERLAP_MM,
            .image_width_px  = 1280,
            .image_height_px = 720
        };

        cameras[2] = {
            .id             = 3,
            .type           = "usb_webcam",
            .position_mm    = CAM3_POS_MM,
            .height_mm      = CAMERA_HEIGHT_MM,
            .zone_start_mm  = 2.0f * SPACING_MM - OVERLAP_MM,
            .zone_end_mm    = TABLE_LENGTH_MM,
            .overlap_mm     = OVERLAP_MM,
            .image_width_px  = 1280,
            .image_height_px = 720
        };

        return cameras;
    }
};

} // namespace Camera
