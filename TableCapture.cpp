#include "TableCapture.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

TableCapture::TableCapture(int overlapPixels)
{
    m_overlapPixels = overlapPixels;
}

cv::Mat TableCapture::trimOverlap(const cv::Mat& image, bool trimLeft, bool trimRight) const
{
    int half = m_overlapPixels / 2;
    int left = trimLeft ? half : 0;
    int right = trimRight ? half : 0;

    int newWidth = image.cols - left - right;
    if (newWidth <= 0)
    {
        std::cout << "Attention : overlapPixels trop grand pour la largeur de l'image." << std::endl;
        return image.clone();
    }

    cv::Rect roi(left, 0, newWidth, image.rows);
    return image(roi).clone();
}

cv::Mat TableCapture::buildFullTableImage(
    const cv::Mat& camera0,
    const cv::Mat& camera1,
    const cv::Mat& camera2
) const
{
    if (camera0.empty() || camera1.empty() || camera2.empty())
    {
        std::cout << "Erreur : au moins une des 3 images est vide." << std::endl;
        return cv::Mat();
    }

    // On s'assure que les 3 images ont la meme hauteur avant de les
    // coller cote a cote (sinon le resultat serait deforme/decale).
    int targetHeight = camera0.rows;
    cv::Mat cam0 = camera0;
    cv::Mat cam1 = camera1;
    cv::Mat cam2 = camera2;

    if (camera1.rows != targetHeight)
    {
        double scale = static_cast<double>(targetHeight) / camera1.rows;
        cv::resize(camera1, cam1, cv::Size(), scale, scale);
    }
    if (camera2.rows != targetHeight)
    {
        double scale = static_cast<double>(targetHeight) / camera2.rows;
        cv::resize(camera2, cam2, cv::Size(), scale, scale);
    }

    // Camera 0 (gauche) : on ne rogne que son bord droit (recouvrement avec camera 1).
    // Camera 1 (milieu) : on rogne les deux bords (recouvrement avec 0 et avec 2).
    // Camera 2 (droite) : on ne rogne que son bord gauche (recouvrement avec camera 1).
    cv::Mat left = trimOverlap(cam0, false, true);
    cv::Mat middle = trimOverlap(cam1, true, true);
    cv::Mat right = trimOverlap(cam2, true, false);

    cv::Mat fullImage;
    std::vector<cv::Mat> pieces = { left, middle, right };
    cv::hconcat(pieces, fullImage);

    return fullImage;
}

std::string TableCapture::saveSnapshot(const cv::Mat& fullTableImage, const std::string& folderPath) const
{
    if (fullTableImage.empty())
    {
        std::cout << "Impossible de sauvegarder : image vide." << std::endl;
        return "";
    }

    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream oss;
    oss << folderPath << "/table_"
        << std::put_time(&localTime, "%Y%m%d_%H%M%S")
        << ".png";

    std::string path = oss.str();

    bool ok = cv::imwrite(path, fullTableImage);
    if (!ok)
    {
        std::cout << "Echec de la sauvegarde dans : " << path << std::endl;
        return "";
    }

    std::cout << "Image complete de la table sauvegardee : " << path << std::endl;
    return path;
}
