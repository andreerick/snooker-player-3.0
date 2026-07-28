#pragma once

#include "CameraConfig.h"
#include <array>
#include <cmath>

namespace Camera {

// Matrice de calibration pour une camera plongeante (vue a 90 degres)
//
// Pour une vue overhead, la conversion pixel -> mm est lineaire :
//   x_mm = (x_px - cx) / scale_x + origin_x
//   y_mm = (y_px - cy) / scale_y + origin_y
//
// ou scale = pixels / mm (calculee depuis 4 points de reference connus)
struct CalibrationMatrix
{
    float scale_x;    // pixels par mm (axe largeur)
    float scale_y;    // pixels par mm (axe longueur)
    float origin_x;   // x de l'origine table en pixels
    float origin_y;   // y de l'origine table en pixels
    bool  valid = false;
};

// Point de reference pour la calibration
struct CalibrationPoint
{
    float x_px;  // position en pixels
    float y_px;
    float x_mm;  // position reelle sur la table (mm)
    float y_mm;
};

// Calibration d'une camera plongeante
class CameraCalibration
{
public:
    explicit CameraCalibration(const CameraConfig& config);

    // Calibration a partir de 4 coins connus (vue overhead = simple)
    void calibrateFromCorners(
        const CalibrationPoint& topLeft,
        const CalibrationPoint& topRight,
        const CalibrationPoint& bottomLeft,
        const CalibrationPoint& bottomRight
    );

    // Calibration automatique simple (suppose que la camera
    // couvre exactement sa zone de couverture configuree)
    void calibrateDefault();

    // Conversion pixel -> coordonnees table (mm)
    bool pixelToTable(float x_px, float y_px,
                      float& x_mm, float& y_mm) const;

    // Conversion coordonnees table -> pixel
    bool tableToPixel(float x_mm, float y_mm,
                      float& x_px, float& y_px) const;

    bool isValid() const { return m_matrix.valid; }

    const CalibrationMatrix& getMatrix() const { return m_matrix; }

private:
    CameraConfig      m_config;
    CalibrationMatrix m_matrix;
};

} // namespace Camera
