#include "ShotAnalyzer.h"
#include <iostream>
#include <cmath>
#include <limits>

ShotAnalyzer::ShotAnalyzer()
{
    m_contactThresholdPx = 25.f;
    m_edgeMarginPx = 20.f;
}

void ShotAnalyzer::setPocketZones(const std::vector<PocketZone>& zones)
{
    m_pocketZones = zones;
}

const std::vector<PocketZone>& ShotAnalyzer::getPocketZones() const
{
    return m_pocketZones;
}

void ShotAnalyzer::setContactThreshold(float pixels)
{
    m_contactThresholdPx = pixels;
}

void ShotAnalyzer::setEdgeMargin(float pixels)
{
    m_edgeMarginPx = pixels;
}

float ShotAnalyzer::distance(const cv::Point2f& a, const cv::Point2f& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

ContactResult ShotAnalyzer::findFirstContact(
    const std::vector<BallPosition>& whiteBallTrajectory,
    const std::map<std::string, std::vector<std::vector<BallPosition>>>& otherBallsTrajectories
) const
{
    ContactResult result;

    for (size_t frame = 0; frame < whiteBallTrajectory.size(); frame++)
    {
        const BallPosition& whitePos = whiteBallTrajectory[frame];
        if (!whitePos.visible)
        {
            continue;
        }

        // A cette image, on regarde TOUTES les autres billes et on
        // garde la plus proche (en cas de contacts quasi simultanes,
        // on considere la plus proche comme le vrai premier contact).
        std::string bestColor;
        int bestIndex = -1;
        float bestDist = std::numeric_limits<float>::max();

        for (const auto& colorEntry : otherBallsTrajectories)
        {
            const std::string& colorName = colorEntry.first;
            const auto& ballsOfThisColor = colorEntry.second;

            for (size_t ballIdx = 0; ballIdx < ballsOfThisColor.size(); ballIdx++)
            {
                if (frame >= ballsOfThisColor[ballIdx].size())
                {
                    continue;
                }
                const BallPosition& otherPos = ballsOfThisColor[ballIdx][frame];
                if (!otherPos.visible)
                {
                    continue;
                }

                float d = distance(whitePos.position, otherPos.position);
                if (d < m_contactThresholdPx && d < bestDist)
                {
                    bestDist = d;
                    bestColor = colorName;
                    bestIndex = static_cast<int>(ballIdx);
                }
            }
        }

        if (bestIndex >= 0)
        {
            result.contactFound = true;
            result.touchedColorName = bestColor;
            result.touchedBallIndex = bestIndex;
            result.frameIndex = static_cast<int>(frame);
            return result; // on s'arrete au tout premier contact trouve
        }
    }

    return result; // contactFound reste false : aucun contact detecte
}

DisappearanceResult ShotAnalyzer::analyzeDisappearance(
    const cv::Point2f& lastKnownPosition,
    const cv::Size& tableImageSize
) const
{
    DisappearanceResult result;

    // 1. Est-ce pres d'une poche connue ?
    for (const auto& pocket : m_pocketZones)
    {
        float d = distance(lastKnownPosition, pocket.center);
        if (d <= pocket.radius)
        {
            result.type = DisappearanceType::Pocketed;
            result.pocketName = pocket.name;
            return result;
        }
    }

    // 2. Sinon, est-ce pres du bord de l'image (mais pas d'une poche) ?
    // -> suspect : la bille est peut-etre sortie de la table.
    bool nearEdge =
        lastKnownPosition.x <= m_edgeMarginPx ||
        lastKnownPosition.y <= m_edgeMarginPx ||
        lastKnownPosition.x >= (tableImageSize.width - m_edgeMarginPx) ||
        lastKnownPosition.y >= (tableImageSize.height - m_edgeMarginPx);

    if (nearEdge)
    {
        result.type = DisappearanceType::OffTable;
        return result;
    }

    // 3. Ni pres d'une poche, ni pres du bord : disparition suspecte
    // mais pas franchement identifiable (occlusion probable, main du
    // joueur, reflet...). A NE PAS considerer comme un empochage valide
    // sans verification supplementaire.
    result.type = DisappearanceType::Unknown;
    return result;
}

bool ShotAnalyzer::isNearTableEdge(
    const cv::Point2f& position,
    const cv::Size& tableImageSize
) const
{
    return
        position.x <= m_edgeMarginPx ||
        position.y <= m_edgeMarginPx ||
        position.x >= (tableImageSize.width - m_edgeMarginPx) ||
        position.y >= (tableImageSize.height - m_edgeMarginPx);
}
