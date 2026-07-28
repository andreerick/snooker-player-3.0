#include "CameraCalibration.h"

#include <cmath>
#include <stdexcept>

namespace Camera {

CameraCalibration::CameraCalibration(const CameraConfig& config)
    : m_config(config)
{
}

void CameraCalibration::calibrateFromCorners(
    const CalibrationPoint& topLeft,
    const CalibrationPoint& topRight,
    const CalibrationPoint& bottomLeft,
    const CalibrationPoint& bottomRight
)
{
    // Calcule l'echelle moyenne en x (largeur)
    float dx_px_top    = topRight.x_px - topLeft.x_px;
    float dx_px_bot    = bottomRight.x_px - bottomLeft.x_px;
    float dx_mm_top    = topRight.x_mm - topLeft.x_mm;
    float dx_mm_bot    = bottomRight.x_mm - bottomLeft.x_mm;

    float scale_x_top = (dx_mm_top > 0.0f) ? dx_px_top / dx_mm_top : 0.0f;
    float scale_x_bot = (dx_mm_bot > 0.0f) ? dx_px_bot / dx_mm_bot : 0.0f;
    m_matrix.scale_x  = (scale_x_top + scale_x_bot) / 2.0f;

    // Calcule l'echelle moyenne en y (longueur)
    float dy_px_left  = topLeft.y_px - bottomLeft.y_px;
    float dy_px_right = topRight.y_px - bottomRight.y_px;
    float dy_mm_left  = topLeft.y_mm - bottomLeft.y_mm;
    float dy_mm_right = topRight.y_mm - bottomRight.y_mm;

    float scale_y_left  = (dy_mm_left  > 0.0f) ? dy_px_left  / dy_mm_left  : 0.0f;
    float scale_y_right = (dy_mm_right > 0.0f) ? dy_px_right / dy_mm_right : 0.0f;
    m_matrix.scale_y = (scale_y_left + scale_y_right) / 2.0f;

    // Origine en pixels (coin bas-gauche de la zone)
    m_matrix.origin_x = bottomLeft.x_px - bottomLeft.x_mm * m_matrix.scale_x;
    m_matrix.origin_y = bottomLeft.y_px + bottomLeft.y_mm * m_matrix.scale_y;

    m_matrix.valid = (m_matrix.scale_x > 0.0f && m_matrix.scale_y > 0.0f);
}

void CameraCalibration::calibrateDefault()
{
    // Calibration simplifiee :
    // La camera couvre toute la largeur de la table et sa zone longitudinale.
    // Zone longitudinale : [zone_start_mm, zone_end_mm]

    float zone_length_mm = m_config.zone_end_mm - m_config.zone_start_mm;

    if (zone_length_mm <= 0.0f || TripleCameraSystem::TABLE_WIDTH_MM <= 0.0f)
    {
        m_matrix.valid = false;
        return;
    }

    // pixels / mm
    m_matrix.scale_x = static_cast<float>(m_config.image_width_px)
                       / TripleCameraSystem::TABLE_WIDTH_MM;
    m_matrix.scale_y = static_cast<float>(m_config.image_height_px)
                       / zone_length_mm;

    // En pixels, l'origine (x=0mm, y=zone_start_mm) est en (0, image_height)
    m_matrix.origin_x = 0.0f;
    m_matrix.origin_y = static_cast<float>(m_config.image_height_px)
                        + m_config.zone_start_mm * m_matrix.scale_y;

    m_matrix.valid = true;
}

bool CameraCalibration::pixelToTable(float x_px, float y_px,
                                     float& x_mm, float& y_mm) const
{
    if (!m_matrix.valid || m_matrix.scale_x == 0.0f || m_matrix.scale_y == 0.0f)
    {
        return false;
    }

    x_mm = (x_px - m_matrix.origin_x) / m_matrix.scale_x;
    y_mm = (m_matrix.origin_y - y_px) / m_matrix.scale_y;

    return true;
}

bool CameraCalibration::tableToPixel(float x_mm, float y_mm,
                                     float& x_px, float& y_px) const
{
    if (!m_matrix.valid)
    {
        return false;
    }

    x_px = m_matrix.origin_x + x_mm * m_matrix.scale_x;
    y_px = m_matrix.origin_y - y_mm * m_matrix.scale_y;

    return true;
}

} // namespace Camera
