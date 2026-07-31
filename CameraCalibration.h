#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

// =====================================================================
// CameraCalibration
// ---------------------------------------------------------------------
// Gere la conversion "pixels d'une camera" -> "coordonnees reelles sur
// la table (en cm)" grace a une homographie (cv::findHomography).
//
// Chaque camera couvre environ un tiers de la table (voir notes du
// projet : 3 cameras ELP a 1.20m de hauteur, ~10-15cm de recouvrement
// entre zones). Chaque camera a donc SA PROPRE homographie, calculee a
// partir de 4 points connus (les 4 coins de la zone qu'elle observe).
// =====================================================================
class CameraCalibration
{
public:
    CameraCalibration();

    // Calcule l'homographie a partir de 4 points image (pixels) et de
    // leurs 4 points reels correspondants sur la table (en cm).
    // Retourne false si le calcul echoue (points mal choisis, alignes, etc.)
    bool calibrate(
        const std::vector<cv::Point2f>& imagePoints,
        const std::vector<cv::Point2f>& tablePointsCm
    );

    // Convertit un point (pixel de la camera) en coordonnees reelles
    // sur la table (cm). Ne fonctionne que si calibrate() a reussi.
    cv::Point2f imageToTable(const cv::Point2f& imagePoint) const;

    bool isCalibrated() const;

    // Sauvegarde / chargement de l'homographie dans un fichier,
    // pour ne pas avoir a recalibrer a chaque demarrage.
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    cv::Mat m_homography;
    bool m_calibrated;
};

// =====================================================================
// CameraSystem
// ---------------------------------------------------------------------
// Regroupe les 3 CameraCalibration (une par camera / zone de la table)
// et fournit une seule fonction pour convertir un point détecté par
// N'IMPORTE QUELLE des 3 caméras vers la coordonnée réelle sur la table.
// =====================================================================
class CameraSystem
{
public:
    static const int NB_CAMERAS = 3;

    CameraSystem();

    // Calibre une camera precise (0, 1 ou 2) avec ses 4 points.
    bool calibrateCamera(
        int cameraIndex,
        const std::vector<cv::Point2f>& imagePoints,
        const std::vector<cv::Point2f>& tablePointsCm
    );

    // Convertit un point detecte par une camera donnee en coordonnee
    // reelle sur la table (cm).
    cv::Point2f toTableCoordinates(int cameraIndex, const cv::Point2f& imagePoint) const;

    bool isCameraCalibrated(int cameraIndex) const;
    bool areAllCamerasCalibrated() const;

    bool saveAll(const std::string& folderPath) const;
    bool loadAll(const std::string& folderPath);

private:
    CameraCalibration m_cameras[NB_CAMERAS];
};
