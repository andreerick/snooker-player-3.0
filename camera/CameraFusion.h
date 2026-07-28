#pragma once

#include "CameraConfig.h"
#include "CameraCalibration.h"

#include <string>
#include <vector>
#include <array>
#include <map>
#include <cstdint>

namespace Camera {

// Detection d'une bille par une camera
struct BallDetection
{
    std::string color;       // Nom de la bille ("Rouge", "Noir", etc.)
    float       x_mm;        // Position sur la table (mm)
    float       y_mm;
    float       confidence;  // Confiance de la detection [0.0 - 1.0]
    int         camera_id;   // Camera ayant fait la detection
    int64_t     timestamp_ms; // Horodatage (ms depuis epoch)
};

// Frame brute envoyee par une camera
struct CameraFrame
{
    int                         camera_id;
    int64_t                     timestamp_ms;
    std::vector<BallDetection>  detections; // positions en mm apres calibration
};

// Etat fusionne de toutes les cameras
struct FusedGameState
{
    int64_t                     timestamp_ms;
    std::vector<BallDetection>  balls;  // positions fusionnees
    bool                        valid = false;
};

// Systeme de fusion multi-camera
//
// Reçoit les detections de 3 cameras plongeantes alignees et
// produit une vue consolidee de la table.
//
// Algorithme de fusion :
//   1. Pour chaque couleur de bille, collecter toutes les detections
//   2. Dans les zones de chevauchement, valider par consensus
//   3. Calculer la position moyenne ponderee par la confiance
class CameraFusion
{
public:
    explicit CameraFusion(const std::array<CameraConfig, 3>& configs);

    // Enregistre le frame d'une camera (detections deja en mm)
    void submitFrame(const CameraFrame& frame);

    // Retourne l'etat fusionne le plus recent
    FusedGameState getFusedState() const;

    // Calibrations (une par camera)
    CameraCalibration& getCalibration(int camera_id);

    // Vide les buffers de frames
    void reset();

private:
    // Fusionne les detections de plusieurs cameras pour une meme couleur
    BallDetection fuseBallDetections(
        const std::vector<BallDetection>& detections
    ) const;

    // Verifie si deux detections correspondent a la meme bille
    // (distance < seuil de tolerance)
    bool areMatchingDetections(
        const BallDetection& a,
        const BallDetection& b,
        float tolerance_mm = 50.0f
    ) const;

    std::array<CameraConfig, 3>         m_configs;
    std::array<CameraCalibration, 3>    m_calibrations;

    // Dernier frame recu par chaque camera
    std::map<int, CameraFrame>          m_lastFrames;

    // Tolerance de synchronisation entre cameras (ms)
    static constexpr int64_t SYNC_TOLERANCE_MS = 100;

    // Distance maximale entre deux detections de la meme bille (mm)
    static constexpr float MATCH_TOLERANCE_MM = 50.0f;
};

} // namespace Camera
