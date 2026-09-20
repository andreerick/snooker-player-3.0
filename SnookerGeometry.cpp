#include "SnookerGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    bool isColour(const std::string& name)
    {
        return name == "Jaune" || name == "Verte" || name == "Marron"
            || name == "Bleue" || name == "Rose" || name == "Noire";
    }

    // Distance du point (px,py) au segment [(ax,ay),(bx,by)].
    double distanceToSegment(double px, double py, double ax, double ay, double bx, double by)
    {
        double dx = bx - ax;
        double dy = by - ay;
        double len2 = dx * dx + dy * dy;
        double t = (len2 > 0.0) ? ((px - ax) * dx + (py - ay) * dy) / len2 : 0.0;
        t = std::max(0.0, std::min(1.0, t));
        double cx = ax + t * dx;
        double cy = ay + t * dy;
        return std::hypot(px - cx, py - cy);
    }

    struct PathClearance
    {
        double clearance = std::numeric_limits<double>::infinity();
        const PositionedBall* limiting = nullptr;
    };

    // Degagement du trajet de la blanche qui frole `target` d'un cote
    // (side = +1 ou -1) : trajet rectiligne du centre de la blanche jusqu'au
    // point de contact rasant, tangent au cercle de rayon 2r autour de la
    // cible. Un obstacle gene s'il passe a moins de 2r (rayon blanche + rayon
    // obstacle) de ce trajet. Renvoie la plus petite marge (cm) et l'obstacle
    // correspondant.
    PathClearance clearanceOnEdge(const PositionedBall& cue, const PositionedBall& target,
                                  int side, const std::vector<PositionedBall>& blockers)
    {
        const double r = kSnookerBallRadiusCm;
        const double R = 2.0 * r;

        double dx = target.x - cue.x;
        double dy = target.y - cue.y;
        double d = std::hypot(dx, dy);

        PathClearance result;
        if (d <= R)
        {
            // Blanche deja en contact avec la cible : rien ne peut s'interposer.
            return result;
        }

        double phi = std::atan2(dy, dx);
        double alpha = std::asin(R / d);
        double angle = phi + side * alpha;
        double tangentLength = std::sqrt(d * d - R * R);
        double ex = cue.x + tangentLength * std::cos(angle);
        double ey = cue.y + tangentLength * std::sin(angle);

        for (const PositionedBall& blocker : blockers)
        {
            double dist = distanceToSegment(blocker.x, blocker.y, cue.x, cue.y, ex, ey);
            double clearance = dist - R;
            if (clearance < result.clearance)
            {
                result.clearance = clearance;
                result.limiting = &blocker;
            }
        }
        return result;
    }
}

std::vector<PositionedBall> ballsOn(const std::string& requiredBall,
                                    const std::vector<PositionedBall>& balls)
{
    std::vector<PositionedBall> on;
    for (const PositionedBall& ball : balls)
    {
        bool playable = (requiredBall == "Couleur")
            ? isColour(ball.name)
            : (ball.name == requiredBall);
        if (playable)
        {
            on.push_back(ball);
        }
    }
    return on;
}

SnookerAssessment assessSnooker(const PositionedBall& cue,
                                const std::vector<PositionedBall>& on,
                                const std::vector<PositionedBall>& blockers,
                                double toleranceCm)
{
    SnookerAssessment assessment;
    if (on.empty())
    {
        return assessment;
    }

    double bestMargin = -std::numeric_limits<double>::infinity();
    const PositionedBall* bestLimiting = nullptr;

    for (const PositionedBall& target : on)
    {
        PathClearance left = clearanceOnEdge(cue, target, +1, blockers);
        PathClearance right = clearanceOnEdge(cue, target, -1, blockers);

        // Il faut que les DEUX bords soient degages : la marge de la cible
        // est la plus petite des deux.
        PathClearance worst = (left.clearance <= right.clearance) ? left : right;
        double margin = worst.clearance;

        if (margin > bestMargin)
        {
            bestMargin = margin;
            bestLimiting = worst.limiting;
        }
    }

    assessment.margin = std::isinf(bestMargin) ? toleranceCm * 100.0 : bestMargin;

    if (bestMargin >= toleranceCm)
    {
        assessment.state = SnookerState::NotSnookered;
    }
    else if (bestMargin >= -toleranceCm)
    {
        assessment.state = SnookerState::Borderline;
    }
    else
    {
        assessment.state = SnookerState::Snookered;
    }

    if (assessment.state != SnookerState::NotSnookered && bestLimiting != nullptr)
    {
        assessment.blockingBall = bestLimiting->name;
    }
    return assessment;
}

SnookerAssessment assessSnookerOnTable(const PositionedBall& cue,
                                       const std::vector<PositionedBall>& allBalls,
                                       const std::string& requiredBall,
                                       double toleranceCm)
{
    std::vector<PositionedBall> on = ballsOn(requiredBall, allBalls);

    std::vector<PositionedBall> blockers;
    for (const PositionedBall& ball : allBalls)
    {
        bool isOn = (requiredBall == "Couleur") ? isColour(ball.name) : (ball.name == requiredBall);
        if (!isOn && ball.name != "Blanche")
        {
            blockers.push_back(ball);
        }
    }
    return assessSnooker(cue, on, blockers, toleranceCm);
}
