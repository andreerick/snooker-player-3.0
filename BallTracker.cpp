#include "BallTracker.h"
#include <algorithm>
#include <limits>
#include <cmath>

BallTracker::BallTracker(int maxFramesLost, float maxMatchDistancePx)
{
    m_maxFramesLost = maxFramesLost;
    m_maxMatchDistancePx = maxMatchDistancePx;
}

void BallTracker::reset()
{
    m_tracked.clear();
}

static float distance(const cv::Point2f& a, const cv::Point2f& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

void BallTracker::update(const std::vector<DetectedBall>& currentDetections)
{
    // Regroupe les detections de cette image par couleur.
    std::map<std::string, std::vector<DetectedBall>> byColor;
    for (const auto& det : currentDetections)
    {
        byColor[det.colorName].push_back(det);
    }

    // Pour CHAQUE couleur deja suivie (ou nouvellement vue), on fait
    // un appariement au plus proche voisin entre les positions
    // precedentes et les nouvelles detections.
    std::map<std::string, std::vector<TrackedBall>> newTracked;

    // On traite d'abord les couleurs qu'on suivait deja.
    for (auto& entry : m_tracked)
    {
        const std::string& color = entry.first;
        std::vector<TrackedBall>& previousBalls = entry.second;
        std::vector<DetectedBall> newDetections = byColor.count(color) ? byColor[color] : std::vector<DetectedBall>();
        std::vector<bool> detectionUsed(newDetections.size(), false);

        std::vector<TrackedBall> updatedBalls;

        for (auto& prev : previousBalls)
        {
            // Cherche la detection la plus proche pas encore utilisee.
            int bestIndex = -1;
            float bestDist = m_maxMatchDistancePx;
            for (size_t i = 0; i < newDetections.size(); i++)
            {
                if (detectionUsed[i])
                {
                    continue;
                }
                float d = distance(prev.lastPosition, newDetections[i].pixelPosition);
                if (d < bestDist)
                {
                    bestDist = d;
                    bestIndex = static_cast<int>(i);
                }
            }

            TrackedBall updated;
            if (bestIndex >= 0)
            {
                // Bille retrouvee : on met a jour sa position.
                updated.lastPosition = newDetections[bestIndex].pixelPosition;
                updated.radius = newDetections[bestIndex].radius;
                updated.framesSinceSeen = 0;
                detectionUsed[bestIndex] = true;
            }
            else
            {
                // Pas retrouvee cette image : on garde la derniere position connue.
                updated.lastPosition = prev.lastPosition;
                updated.radius = prev.radius;
                updated.framesSinceSeen = prev.framesSinceSeen + 1;
            }

            // On ne garde que si elle n'est pas consideree comme
            // definitivement disparue (empochee).
            if (updated.framesSinceSeen <= m_maxFramesLost)
            {
                updatedBalls.push_back(updated);
            }
        }

        // Les detections restantes (non appariees a une bille existante)
        // sont de nouvelles billes pour cette couleur (rare en pratique,
        // sauf par exemple une bille respotee apres une faute).
        for (size_t i = 0; i < newDetections.size(); i++)
        {
            if (!detectionUsed[i])
            {
                TrackedBall fresh;
                fresh.lastPosition = newDetections[i].pixelPosition;
                fresh.radius = newDetections[i].radius;
                fresh.framesSinceSeen = 0;
                updatedBalls.push_back(fresh);
            }
        }

        newTracked[color] = updatedBalls;
        byColor.erase(color); // deja traite
    }

    // Couleurs jamais vues avant (premiere apparition) : on les ajoute telles quelles.
    for (auto& entry : byColor)
    {
        std::vector<TrackedBall> freshBalls;
        for (const auto& det : entry.second)
        {
            TrackedBall fresh;
            fresh.lastPosition = det.pixelPosition;
            fresh.radius = det.radius;
            fresh.framesSinceSeen = 0;
            freshBalls.push_back(fresh);
        }
        newTracked[entry.first] = freshBalls;
    }

    m_tracked = newTracked;
}

int BallTracker::countTracked(const std::string& colorName) const
{
    auto it = m_tracked.find(colorName);
    if (it == m_tracked.end())
    {
        return 0;
    }
    return static_cast<int>(it->second.size());
}

std::vector<TrackedBall> BallTracker::getTracked(const std::string& colorName) const
{
    auto it = m_tracked.find(colorName);
    if (it == m_tracked.end())
    {
        return {};
    }
    return it->second;
}

int BallTracker::removeStaleBalls(const std::string& colorName)
{
    auto it = m_tracked.find(colorName);
    if (it == m_tracked.end())
    {
        return 0;
    }

    int before = static_cast<int>(it->second.size());

    std::vector<TrackedBall> kept;
    for (const auto& ball : it->second)
    {
        if (ball.framesSinceSeen <= m_maxFramesLost)
        {
            kept.push_back(ball);
        }
    }

    int removedCount = before - static_cast<int>(kept.size());
    it->second = kept;
    return removedCount;
}
