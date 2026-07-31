#include "BallMapRecorder.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

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
