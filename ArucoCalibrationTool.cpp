#include "CameraCalibration.h"
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <iostream>
#include <map>

// =====================================================================
// Outil de calibration AUTOMATIQUE par marqueurs ArUco
// ---------------------------------------------------------------------
// Remplace le clic a la souris (CalibrationTool.cpp) par une detection
// 100% automatique : colle 4 marqueurs ArUco imprimes aux 4 coins de
// la zone couverte par une camera, et le programme retrouve leurs
// positions avec une precision inferieure au pixel.
//
// AVANTAGES par rapport au clic souris :
//   - Precision bien meilleure et constante (pas de tremblement de main)
//   - Reproductible : si la camera bouge un peu, on recalibre a
//     l'identique sans avoir a re-cliquer "a peu pres au bon endroit"
//   - Resout le probleme des points de transition entre 2 zones de
//     cameras, qui n'ont aucun repere physique naturel sur le tapis.
//
// UTILISATION :
//   1. Génère les 4 marqueurs a imprimer avec --generate
//        ArucoCalibrationTool.exe --generate
//      (cree marker_0.png, marker_1.png, marker_2.png, marker_3.png)
//   2. Imprime-les et colle-les aux 4 coins de la zone d'une camera :
//        marker_0 = haut-gauche, marker_1 = haut-droit,
//        marker_2 = bas-droit,   marker_3 = bas-gauche
//   3. Calibre une camera avec :
//        ArucoCalibrationTool.exe <index_camera 0/1/2> <image_ou_flux>
// =====================================================================

// =====================================================================
// Outil de calibration AUTOMATIQUE par marqueurs ArUco
// ---------------------------------------------------------------------
// Version simplifiee a 6 marqueurs au total (2 par camera), colles sur
// le bord EXTERIEUR de la table (le chassis en bois, pas la bande de
// jeu), sur les 2 longs cotes :
//   - Camera 1 (zone billes jaune/verte/marron) : marqueurs ID 0 (haut) et ID 1 (bas)
//   - Camera 2 (zone bille bleue, centre table)  : marqueurs ID 2 (haut) et ID 3 (bas)
//   - Camera 3 (zone billes rouges/rose/noire)   : marqueurs ID 4 (haut) et ID 5 (bas)
//
// Cette version a 2 points suffit car les cameras regardent tout droit
// vers le bas (1.20m de hauteur, zone directement sous la camera, peu
// de perspective) : une simple similitude (rotation + echelle +
// translation) suffit, pas besoin d'une homographie complete a 4 points.
//
// UTILISATION :
//   1. Génère les 6 marqueurs :  ArucoCalibrationTool.exe --generate
//   2. Colle-les sur le bord EXTERIEUR de la table (voir schema du manuel)
//   3. Calibre une camera :
//        ArucoCalibrationTool.exe <index_camera 0/1/2> <image_ou_flux>
// =====================================================================

void generateMarkers()
{
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

    for (int id = 0; id < 6; id++)
    {
        cv::Mat markerImage;
        cv::aruco::generateImageMarker(dictionary, id, 400, markerImage);
        std::string filename = "marker_" + std::to_string(id) + ".png";
        cv::imwrite(filename, markerImage);
        std::cout << "Marqueur genere : " << filename << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Colle ces 6 marqueurs sur le bord EXTERIEUR de la table (chassis), sur les 2 longs cotes :" << std::endl;
    std::cout << "  marker_0.png -> Camera 1 (jaune/verte/marron), bord HAUT" << std::endl;
    std::cout << "  marker_1.png -> Camera 1 (jaune/verte/marron), bord BAS" << std::endl;
    std::cout << "  marker_2.png -> Camera 2 (bleue, centre),      bord HAUT" << std::endl;
    std::cout << "  marker_3.png -> Camera 2 (bleue, centre),      bord BAS" << std::endl;
    std::cout << "  marker_4.png -> Camera 3 (rouges/rose/noire),  bord HAUT" << std::endl;
    std::cout << "  marker_5.png -> Camera 3 (rouges/rose/noire),  bord BAS" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc >= 2 && std::string(argv[1]) == "--generate")
    {
        generateMarkers();
        return 0;
    }

    if (argc < 3)
    {
        std::cout << "Utilisation :" << std::endl;
        std::cout << "  Generer les marqueurs : ArucoCalibrationTool.exe --generate" << std::endl;
        std::cout << "  Calibrer une camera   : ArucoCalibrationTool.exe <index_camera 0/1/2> <image_ou_index_flux>" << std::endl;
        return 1;
    }

    int cameraIndex = std::stoi(argv[1]);
    std::string source = argv[2];

    if (cameraIndex < 0 || cameraIndex >= CameraSystem::NB_CAMERAS)
    {
        std::cout << "Erreur : l'index camera doit etre 0, 1 ou 2." << std::endl;
        return 1;
    }

    cv::Mat frame;
    bool isNumber = !source.empty() && std::all_of(source.begin(), source.end(), ::isdigit);

    if (isNumber)
    {
        cv::VideoCapture cap(std::stoi(source));
        if (!cap.isOpened())
        {
            std::cout << "Impossible d'ouvrir la camera " << source << std::endl;
            return 1;
        }
        cap >> frame;
    }
    else
    {
        frame = cv::imread(source);
    }

    if (frame.empty())
    {
        std::cout << "Image vide, impossible de continuer." << std::endl;
        return 1;
    }

    // -----------------------------------------------------------------
    // Detection automatique des 2 marqueurs de CETTE camera.
    // Camera 0 -> IDs 0 (haut) et 1 (bas)
    // Camera 1 -> IDs 2 (haut) et 3 (bas)
    // Camera 2 -> IDs 4 (haut) et 5 (bas)
    // -----------------------------------------------------------------
    int idHaut = cameraIndex * 2;
    int idBas = cameraIndex * 2 + 1;

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);

    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> rejected;
    detector.detectMarkers(frame, corners, ids, rejected);

    std::cout << "Marqueurs detectes : " << ids.size() << " (attendus : ID " << idHaut << " et ID " << idBas << ")" << std::endl;

    std::map<int, cv::Point2f> markerCenters;
    for (size_t i = 0; i < ids.size(); i++)
    {
        cv::Point2f center(0.f, 0.f);
        for (const auto& pt : corners[i])
        {
            center += pt;
        }
        center *= (1.0f / corners[i].size());
        markerCenters[ids[i]] = center;
        std::cout << "  ID " << ids[i] << " detecte au centre (" << center.x << ", " << center.y << ")" << std::endl;
    }

    if (markerCenters.find(idHaut) == markerCenters.end() || markerCenters.find(idBas) == markerCenters.end())
    {
        std::cout << "Erreur : il manque le marqueur ID " << idHaut << " et/ou ID " << idBas << "." << std::endl;
        return 1;
    }

    // -----------------------------------------------------------------
    // Position reelle (cm) du centre de la zone de cette camera, sur
    // les 2 longs bords EXTERIEURS de la table (voir schema du manuel).
    // Table 357 x 178 cm, 3 zones centrees sur : baulk (~59.5cm),
    // bille bleue/centre (~178.5cm), billes rouges (~297.5cm).
    // -----------------------------------------------------------------
    float zoneCenters[3] = { 59.5f, 178.5f, 297.5f };
    float tableWidth = 178.0f;
    float x = zoneCenters[cameraIndex];

    cv::Point2f tablePointHaut(x, 0.0f);
    cv::Point2f tablePointBas(x, tableWidth);

    CameraCalibration calib;
    bool ok = calib.calibrateFromTwoPoints(
        markerCenters[idHaut], markerCenters[idBas],
        tablePointHaut, tablePointBas
    );

    if (!ok)
    {
        std::cout << "Echec de la calibration." << std::endl;
        return 1;
    }

    std::string outputPath = "camera_" + std::to_string(cameraIndex) + ".yml";
    calib.saveToFile(outputPath);
    std::cout << "Calibration automatique reussie et sauvegardee : " << outputPath << std::endl;

    return 0;
}
