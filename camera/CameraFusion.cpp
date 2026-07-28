#include "CameraFusion.h"

#include <cmath>
#include <algorithm>
#include <numeric>

namespace Camera {

CameraFusion::CameraFusion(const std::array<CameraConfig, 3>& configs)
    : m_configs(configs)
    , m_calibrations{ CameraCalibration(configs[0]),
                      CameraCalibration(configs[1]),
                      CameraCalibration(configs[2]) }
{
    // Calibration par defaut pour chaque camera
    for (auto& cal : m_calibrations)
    {
        cal.calibrateDefault();
    }
}

void CameraFusion::submitFrame(const CameraFrame& frame)
{
    m_lastFrames[frame.camera_id] = frame;
}

FusedGameState CameraFusion::getFusedState() const
{
    FusedGameState result;

    if (m_lastFrames.empty())
    {
        return result;
    }

    // Timestamp = le plus recent parmi les cameras disponibles
    int64_t latest_ts = 0;
    for (const auto& [id, frame] : m_lastFrames)
    {
        if (frame.timestamp_ms > latest_ts)
        {
            latest_ts = frame.timestamp_ms;
        }
    }
    result.timestamp_ms = latest_ts;

    // Collecter toutes les detections et les regrouper par couleur
    std::map<std::string, std::vector<BallDetection>> byColor;

    for (const auto& [id, frame] : m_lastFrames)
    {
        // Ignorer les frames trop anciens (hors synchronisation)
        if (latest_ts - frame.timestamp_ms > SYNC_TOLERANCE_MS)
        {
            continue;
        }

        for (const auto& det : frame.detections)
        {
            byColor[det.color].push_back(det);
        }
    }

    // Fusionner les detections par couleur
    for (auto& [color, detections] : byColor)
    {
        result.balls.push_back(fuseBallDetections(detections));
    }

    result.valid = !result.balls.empty();
    return result;
}

CameraCalibration& CameraFusion::getCalibration(int camera_id)
{
    // camera_id de 1 a 3 -> index 0 a 2
    int idx = camera_id - 1;
    if (idx < 0 || idx > 2)
    {
        idx = 0;
    }
    return m_calibrations[idx];
}

void CameraFusion::reset()
{
    m_lastFrames.clear();
}

BallDetection CameraFusion::fuseBallDetections(
    const std::vector<BallDetection>& detections
) const
{
    if (detections.empty())
    {
        return {};
    }

    if (detections.size() == 1)
    {
        return detections[0];
    }

    // Moyenne ponderee par la confiance
    float total_weight = 0.0f;
    float weighted_x   = 0.0f;
    float weighted_y   = 0.0f;
    float max_conf     = 0.0f;

    for (const auto& det : detections)
    {
        float w    = det.confidence;
        total_weight += w;
        weighted_x   += det.x_mm * w;
        weighted_y   += det.y_mm * w;
        if (det.confidence > max_conf)
        {
            max_conf = det.confidence;
        }
    }

    BallDetection fused;
    fused.color      = detections[0].color;
    fused.camera_id  = -1; // fusionné
    fused.timestamp_ms = detections[0].timestamp_ms;

    if (total_weight > 0.0f)
    {
        fused.x_mm = weighted_x / total_weight;
        fused.y_mm = weighted_y / total_weight;
    }
    else
    {
        fused.x_mm = detections[0].x_mm;
        fused.y_mm = detections[0].y_mm;
    }

    // Confiance: augmente avec le nombre de cameras concordantes
    fused.confidence = std::min(1.0f, max_conf * (1.0f + 0.1f * (float)(detections.size() - 1)));

    return fused;
}

bool CameraFusion::areMatchingDetections(
    const BallDetection& a,
    const BallDetection& b,
    float tolerance_mm
) const
{
    float dx = a.x_mm - b.x_mm;
    float dy = a.y_mm - b.y_mm;
    return std::sqrt(dx * dx + dy * dy) < tolerance_mm;
}

} // namespace Camera
