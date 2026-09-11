#include "CameraCalibration.h"
#include <iostream>
#include <fstream>
#include <cmath>

// =====================================================================
// CameraCalibration
// =====================================================================
CameraCalibration::CameraCalibration()
{
    m_calibrated = false;
}

bool CameraCalibration::calibrate(
    const std::vector<cv::Point2f>& imagePoints,
    const std::vector<cv::Point2f>& tablePointsCm
)
{
    if (imagePoints.size() < 4 || tablePointsCm.size() < 4)
    {
        std::cout << "Erreur calibration : il faut au moins 4 points." << std::endl;
        m_calibrated = false;
        return false;
    }

    if (imagePoints.size() != tablePointsCm.size())
    {
        std::cout << "Erreur calibration : nombre de points image et reels different." << std::endl;
        m_calibrated = false;
        return false;
    }

    // findHomography calcule la matrice de transformation 3x3 qui
    // envoie les points image (pixels) vers les points reels (cm).
    // RANSAC permet de tolerer un point mal pointe (erreur de clic).
    cv::Mat h = cv::findHomography(imagePoints, tablePointsCm, cv::RANSAC);

    if (h.empty())
    {
        std::cout << "Erreur calibration : homographie non calculable "
                   << "(points probablement alignes ou incorrects)." << std::endl;
        m_calibrated = false;
        return false;
    }

    m_homography = h;
    m_calibrated = true;
    std::cout << "Calibration reussie." << std::endl;
    return true;
}

bool CameraCalibration::calibrateFromTwoPoints(
    const cv::Point2f& imagePoint1,
    const cv::Point2f& imagePoint2,
    const cv::Point2f& tablePoint1Cm,
    const cv::Point2f& tablePoint2Cm
)
{
    // Vecteur entre les 2 points, cote image et cote reel (cm).
    cv::Point2f dImg = imagePoint2 - imagePoint1;
    cv::Point2f dReal = tablePoint2Cm - tablePoint1Cm;

    double imgLen = std::sqrt(dImg.x * dImg.x + dImg.y * dImg.y);
    double realLen = std::sqrt(dReal.x * dReal.x + dReal.y * dReal.y);

    if (imgLen < 1e-6)
    {
        std::cout << "Erreur calibration 2 points : les 2 points image sont confondus." << std::endl;
        m_calibrated = false;
        return false;
    }

    // Echelle uniforme (cm par pixel) et angle de rotation entre les
    // 2 reperes (image -> table reelle).
    double scale = realLen / imgLen;
    double angleImg = std::atan2(dImg.y, dImg.x);
    double angleReal = std::atan2(dReal.y, dReal.x);
    double angle = angleReal - angleImg;

    double cosA = std::cos(angle);
    double sinA = std::sin(angle);

    // Matrice de similitude 2x2 (rotation + echelle) appliquee au
    // point image translate (image - imagePoint1), puis on ajoute
    // tablePoint1Cm pour obtenir la position reelle.
    // On construit directement une matrice 3x3 homogene, compatible
    // avec cv::perspectiveTransform (deja utilise par imageToTable),
    // pour reutiliser exactement le meme code que la calibration a 4 points.
    double a = scale * cosA;
    double b = -scale * sinA;
    double c = scale * sinA;
    double d = scale * cosA;
    double tx = tablePoint1Cm.x - (a * imagePoint1.x + b * imagePoint1.y);
    double ty = tablePoint1Cm.y - (c * imagePoint1.x + d * imagePoint1.y);

    cv::Mat h = (cv::Mat_<double>(3, 3) <<
        a, b, tx,
        c, d, ty,
        0.0, 0.0, 1.0);

    m_homography = h;
    m_calibrated = true;
    std::cout << "Calibration a 2 points reussie (similitude : echelle="
               << scale << ", rotation=" << (angle * 180.0 / CV_PI) << " deg)." << std::endl;
    return true;
}

cv::Point2f CameraCalibration::imageToTable(const cv::Point2f& imagePoint) const
{
    if (!m_calibrated)
    {
        std::cout << "Attention : camera non calibree, resultat invalide (0,0)." << std::endl;
        return cv::Point2f(0.f, 0.f);
    }

    std::vector<cv::Point2f> src = { imagePoint };
    std::vector<cv::Point2f> dst;
    cv::perspectiveTransform(src, dst, m_homography);
    return dst[0];
}

bool CameraCalibration::isCalibrated() const
{
    return m_calibrated;
}

bool CameraCalibration::saveToFile(const std::string& path) const
{
    if (!m_calibrated)
    {
        std::cout << "Impossible de sauvegarder : camera non calibree." << std::endl;
        return false;
    }

    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened())
    {
        std::cout << "Impossible d'ouvrir le fichier : " << path << std::endl;
        return false;
    }
    fs << "homography" << m_homography;
    fs.release();
    return true;
}

bool CameraCalibration::loadFromFile(const std::string& path)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cout << "Impossible de charger le fichier : " << path << std::endl;
        m_calibrated = false;
        return false;
    }
    fs["homography"] >> m_homography;
    fs.release();

    if (m_homography.empty())
    {
        m_calibrated = false;
        return false;
    }

    m_calibrated = true;
    return true;
}

// =====================================================================
// CameraSystem
// =====================================================================
CameraSystem::CameraSystem()
{
}

bool CameraSystem::calibrateCamera(
    int cameraIndex,
    const std::vector<cv::Point2f>& imagePoints,
    const std::vector<cv::Point2f>& tablePointsCm
)
{
    if (cameraIndex < 0 || cameraIndex >= NB_CAMERAS)
    {
        std::cout << "Index de camera invalide : " << cameraIndex << std::endl;
        return false;
    }
    return m_cameras[cameraIndex].calibrate(imagePoints, tablePointsCm);
}

cv::Point2f CameraSystem::toTableCoordinates(int cameraIndex, const cv::Point2f& imagePoint) const
{
    if (cameraIndex < 0 || cameraIndex >= NB_CAMERAS)
    {
        std::cout << "Index de camera invalide : " << cameraIndex << std::endl;
        return cv::Point2f(0.f, 0.f);
    }
    return m_cameras[cameraIndex].imageToTable(imagePoint);
}

bool CameraSystem::isCameraCalibrated(int cameraIndex) const
{
    if (cameraIndex < 0 || cameraIndex >= NB_CAMERAS)
    {
        return false;
    }
    return m_cameras[cameraIndex].isCalibrated();
}

bool CameraSystem::areAllCamerasCalibrated() const
{
    for (int i = 0; i < NB_CAMERAS; i++)
    {
        if (!m_cameras[i].isCalibrated())
        {
            return false;
        }
    }
    return true;
}

bool CameraSystem::saveAll(const std::string& folderPath) const
{
    bool allOk = true;
    for (int i = 0; i < NB_CAMERAS; i++)
    {
        std::string path = folderPath + "/camera_" + std::to_string(i) + ".yml";
        if (!m_cameras[i].saveToFile(path))
        {
            allOk = false;
        }
    }
    return allOk;
}

bool CameraSystem::loadAll(const std::string& folderPath)
{
    bool allOk = true;
    for (int i = 0; i < NB_CAMERAS; i++)
    {
        std::string path = folderPath + "/camera_" + std::to_string(i) + ".yml";
        if (!m_cameras[i].loadFromFile(path))
        {
            allOk = false;
        }
    }
    return allOk;
}
