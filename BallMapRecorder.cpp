#include "BallMapRecorder.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cmath>

BallMapRecorder::BallMapRecorder(cv::Size tableImageSize)
{
    m_tableImageSize = tableImageSize;
}

cv::Scalar BallMapRecorder::colorForName(const std::string& colorName) const
{
    if (colorName == "Rouge")   return cv::Scalar(30, 30, 200);
    if (colorName == "Jaune")   return cv::Scalar(0, 220, 220);
    if (colorName == "Verte")   return cv::Scalar(40, 160, 40);
    if (colorName == "Marron")  return cv::Scalar(30, 60, 120);
    if (colorName == "Bleue")   return cv::Scalar(200, 100, 30);
    if (colorName == "Rose")    return cv::Scalar(180, 120, 220);
    if (colorName == "Noire")   return cv::Scalar(20, 20, 20);
    if (colorName == "Blanche") return cv::Scalar(240, 240, 240);
    return cv::Scalar(128, 128, 128); // couleur inconnue -> gris
}

std::string BallMapRecorder::saveSnapshot(
    const std::map<std::string, std::vector<TrackedBall>>& currentState,
    const std::string& folderPath,
    int shotNumber
) const
{
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream oss;
    oss << folderPath << "/ball_map_coup"
        << std::setw(4) << std::setfill('0') << shotNumber
        << "_" << std::put_time(&localTime, "%Y%m%d_%H%M%S")
        << ".yml";
    std::string path = oss.str();

    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened())
    {
        std::cout << "Impossible de creer le fichier : " << path << std::endl;
        return "";
    }

    fs << "shotNumber" << shotNumber;
    fs << "colors" << "[";
    for (const auto& entry : currentState)
    {
        fs << "{";
        fs << "name" << entry.first;
        fs << "positions" << "[";
        for (const auto& ball : entry.second)
        {
            fs << "{";
            fs << "x" << ball.lastPosition.x;
            fs << "y" << ball.lastPosition.y;
            fs << "}";
        }
        fs << "]";
        fs << "}";
    }
    fs << "]";
    fs.release();

    std::cout << "Cartographie des billes sauvegardee : " << path << std::endl;
    return path;
}

std::map<std::string, std::vector<cv::Point2f>> BallMapRecorder::loadSnapshot(const std::string& filePath) const
{
    std::map<std::string, std::vector<cv::Point2f>> result;

    cv::FileStorage fs(filePath, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cout << "Impossible d'ouvrir le fichier : " << filePath << std::endl;
        return result;
    }

    cv::FileNode colorsNode = fs["colors"];
    for (cv::FileNodeIterator it = colorsNode.begin(); it != colorsNode.end(); ++it)
    {
        std::string name;
        (*it)["name"] >> name;

        std::vector<cv::Point2f> positions;
        cv::FileNode positionsNode = (*it)["positions"];
        for (cv::FileNodeIterator posIt = positionsNode.begin(); posIt != positionsNode.end(); ++posIt)
        {
            float x, y;
            (*posIt)["x"] >> x;
            (*posIt)["y"] >> y;
            positions.push_back(cv::Point2f(x, y));
        }
        result[name] = positions;
    }

    fs.release();
    return result;
}

cv::Mat BallMapRecorder::renderVisualMap(
    const std::map<std::string, std::vector<TrackedBall>>& currentState
) const
{
    // Fond vert type tapis, aux dimensions de l'image complete de la table.
    cv::Mat canvas(m_tableImageSize, CV_8UC3, cv::Scalar(30, 90, 30));

    // Petit cadre pour delimiter visuellement la table.
    cv::rectangle(
        canvas,
        cv::Point(2, 2),
        cv::Point(m_tableImageSize.width - 2, m_tableImageSize.height - 2),
        cv::Scalar(200, 200, 200),
        2
    );

    for (const auto& entry : currentState)
    {
        const std::string& colorName = entry.first;
        cv::Scalar drawColor = colorForName(colorName);

        for (const auto& ball : entry.second)
        {
            float radius = (ball.radius > 0.f) ? ball.radius : 12.f;
            cv::circle(canvas, ball.lastPosition, static_cast<int>(radius), drawColor, -1);
            cv::circle(canvas, ball.lastPosition, static_cast<int>(radius), cv::Scalar(255, 255, 255), 1);
        }
    }

    return canvas;
}

namespace
{
    // Dessine un cercle en pointilles (OpenCV n'a pas ça nativement) :
    // utilise pour marquer la position CIBLE dans le guide de
    // repositionnement, pour la distinguer visuellement de la bille
    // pleine (position actuelle).
    void drawDashedCircle(cv::Mat& img, cv::Point center, int radius, const cv::Scalar& color, int thickness)
    {
        const int nDashes = 16;
        for (int i = 0; i < nDashes; i += 2)
        {
            double a0 = (360.0 * i) / nDashes;
            double a1 = (360.0 * (i + 1)) / nDashes;
            cv::ellipse(img, center, cv::Size(radius, radius), 0.0, a0, a1, color, thickness, cv::LINE_AA);
        }
    }
}

cv::Mat BallMapRecorder::renderRepositioningGuide(
    const std::map<std::string, std::vector<TrackedBall>>& currentState,
    const std::map<std::string, std::vector<cv::Point2f>>& targetState,
    double cmPerPixel
) const
{
    cv::Mat canvas(m_tableImageSize, CV_8UC3, cv::Scalar(30, 90, 30));
    cv::rectangle(
        canvas,
        cv::Point(2, 2),
        cv::Point(m_tableImageSize.width - 2, m_tableImageSize.height - 2),
        cv::Scalar(200, 200, 200),
        2
    );

    const float matchThresholdPx = 60.f; // au-dela, on considere que ce n'est plus "la meme bille" mais une bille manquante.
    const float inPlaceThresholdPx = 4.f;

    for (const auto& targetEntry : targetState)
    {
        const std::string& colorName = targetEntry.first;
        cv::Scalar drawColor = colorForName(colorName);

        // Copie des positions actuelles de cette couleur, pour un
        // appariement au plus proche voisin (une cible <-> une bille
        // actuelle), sans reutiliser deux fois la meme bille actuelle.
        std::vector<cv::Point2f> currentPositions;
        if (currentState.count(colorName))
        {
            for (const auto& ball : currentState.at(colorName))
            {
                currentPositions.push_back(ball.lastPosition);
            }
        }
        std::vector<bool> used(currentPositions.size(), false);

        for (const auto& target : targetEntry.second)
        {
            int bestIndex = -1;
            float bestDist = matchThresholdPx;
            for (size_t i = 0; i < currentPositions.size(); ++i)
            {
                if (used[i])
                {
                    continue;
                }
                float dx = currentPositions[i].x - target.x;
                float dy = currentPositions[i].y - target.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestIndex = static_cast<int>(i);
                }
            }

            cv::Point targetPt(static_cast<int>(target.x), static_cast<int>(target.y));
            drawDashedCircle(canvas, targetPt, 20, cv::Scalar(255, 255, 255), 2);
            drawDashedCircle(canvas, targetPt, 20, drawColor, 1);

            if (bestIndex < 0)
            {
                // Aucune bille actuelle proche : elle manque sur la
                // table (empochee ou completement egaree), il faut la
                // reposer, pas juste la deplacer.
                cv::putText(
                    canvas, "MANQUANTE",
                    targetPt + cv::Point(-30, 35),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1, cv::LINE_AA
                );
                continue;
            }

            used[bestIndex] = true;
            cv::Point currentPt(
                static_cast<int>(currentPositions[bestIndex].x),
                static_cast<int>(currentPositions[bestIndex].y)
            );
            float distPx = bestDist;
            bool inPlace = distPx < inPlaceThresholdPx;

            if (!inPlace)
            {
                cv::arrowedLine(canvas, currentPt, targetPt, cv::Scalar(0, 255, 255), 2, cv::LINE_AA, 0, 0.25);

                std::string label = (cmPerPixel > 0.0)
                    ? cv::format("%.1f cm", distPx * cmPerPixel)
                    : cv::format("%.0f px", distPx);
                cv::Point midPt((currentPt.x + targetPt.x) / 2 + 10, (currentPt.y + targetPt.y) / 2);
                cv::putText(canvas, label, midPt, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
            }

            cv::circle(canvas, currentPt, 18, drawColor, -1, cv::LINE_AA);
            cv::circle(
                canvas, currentPt, 18,
                inPlace ? cv::Scalar(0, 255, 0) : cv::Scalar(255, 255, 255),
                inPlace ? 3 : 1, cv::LINE_AA
            );
        }
    }

    cv::putText(
        canvas, "Pointille = position a atteindre   |   Plein = position actuelle",
        cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1, cv::LINE_AA
    );

    return canvas;
}