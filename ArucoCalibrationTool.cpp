#include "CameraCalibration.h"
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

// =====================================================================
// Outil de calibration AUTOMATIQUE par marqueurs ArUco
// ---------------------------------------------------------------------
// Version a 10 marqueurs au total, colles sur le bord EXTERIEUR de la
// table (le chassis en bois, pas la bande de jeu) :
//
//   - Camera 1 (jaune/verte/marron, INCLINEE ~4 deg) : 4 marqueurs aux
//     4 coins de sa zone -> calibration homographie complete (4 points)
//   - Camera 2 (bleue, centre, VERTICALE)            : 2 marqueurs,
//     legerement decales de la poche du milieu       -> calibration
//     simplifiee a 2 points (similitude)
//   - Camera 3 (rouges/rose/noire, INCLINEE ~4 deg)  : 4 marqueurs aux
//     4 coins de sa zone -> calibration homographie complete (4 points)
//
// Le programme detecte tout seul COMBIEN de marqueurs il voit sur la
// photo (2 ou 4) et choisit automatiquement la bonne methode :
//   - 2 marqueurs detectes -> CameraCalibration::calibrateFromTwoPoints
//   - 4 marqueurs detectes -> CameraCalibration::calibrate (4 points)
//
// Numerotation humaine (planche a imprimer) = ID ArUco + 1, pour que
// les fichiers s'appellent marker_1.png ... marker_10.png :
//   ID 0 -> marker_1.png  : Camera 1, HAUT, cote baulk
//   ID 1 -> marker_2.png  : Camera 1, HAUT, cote camera 2
//   ID 2 -> marker_3.png  : Camera 1, BAS,  cote baulk
//   ID 3 -> marker_4.png  : Camera 1, BAS,  cote camera 2
//   ID 4 -> marker_5.png  : Camera 2, HAUT
//   ID 5 -> marker_6.png  : Camera 2, BAS
//   ID 6 -> marker_7.png  : Camera 3, HAUT, cote camera 2
//   ID 7 -> marker_8.png  : Camera 3, HAUT, cote noire
//   ID 8 -> marker_9.png  : Camera 3, BAS,  cote camera 2
//   ID 9 -> marker_10.png : Camera 3, BAS,  cote noire
//
// UTILISATION :
//   1. Genere les 10 marqueurs :  ArucoCalibrationTool.exe --generate
//   2. Colle-les sur le bord EXTERIEUR de la table (voir schema du manuel)
//   3. Calibre une camera :
//        ArucoCalibrationTool.exe <index_camera 0/1/2> <image_ou_flux>
//      (0 = Camera 1, 1 = Camera 2, 2 = Camera 3)
// =====================================================================

static const int NB_MARKERS = 10;

void generateMarkers()
{
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

    for (int id = 0; id < NB_MARKERS; id++)
    {
        cv::Mat markerImage;
        cv::aruco::generateImageMarker(dictionary, id, 400, markerImage);
        std::string filename = "marker_" + std::to_string(id + 1) + ".png";
        cv::imwrite(filename, markerImage);
        std::cout << "Marqueur genere : " << filename << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Colle ces 10 marqueurs sur le bord EXTERIEUR de la table (voir manuel, section 2) :" << std::endl;
    std::cout << "  marker_1.png  -> Camera 1, HAUT, cote baulk" << std::endl;
    std::cout << "  marker_2.png  -> Camera 1, HAUT, cote camera 2" << std::endl;
    std::cout << "  marker_3.png  -> Camera 1, BAS,  cote baulk" << std::endl;
    std::cout << "  marker_4.png  -> Camera 1, BAS,  cote camera 2" << std::endl;
    std::cout << "  marker_5.png  -> Camera 2, HAUT" << std::endl;
    std::cout << "  marker_6.png  -> Camera 2, BAS" << std::endl;
    std::cout << "  marker_7.png  -> Camera 3, HAUT, cote camera 2" << std::endl;
    std::cout << "  marker_8.png  -> Camera 3, HAUT, cote noire" << std::endl;
    std::cout << "  marker_9.png  -> Camera 3, BAS,  cote camera 2" << std::endl;
    std::cout << "  marker_10.png -> Camera 3, BAS,  cote noire" << std::endl;
}

// -----------------------------------------------------------------
// Pour chaque camera : la liste des ID ArUco attendus, et leur
// position reelle sur la table (cm), dans le MEME ORDRE.
// Table 357 x 178 cm. Poche centrale a x = 178.5 cm.
// -----------------------------------------------------------------
struct CameraMarkerPlan
{
    std::vector<int> expectedIds;
    std::vector<cv::Point2f> tablePointsCm;
};

CameraMarkerPlan getMarkerPlan(int cameraIndex)
{
    CameraMarkerPlan plan;

    if (cameraIndex == 0)
    {
        // Camera 1 : 4 coins de la zone 0 -> 130 cm
        plan.expectedIds = { 0, 1, 2, 3 };
        plan.tablePointsCm = {
            cv::Point2f(0.0f,   0.0f),    // ID 0 : haut, cote baulk
            cv::Point2f(130.0f, 0.0f),    // ID 1 : haut, cote camera 2
            cv::Point2f(0.0f,   178.0f),  // ID 2 : bas,  cote baulk
            cv::Point2f(130.0f, 178.0f)   // ID 3 : bas,  cote camera 2
        };
    }
    else if (cameraIndex == 1)
    {
        // Camera 2 : 2 marqueurs, decales de 15 cm de part et d'autre
        // de la poche centrale (x = 178.5 cm), en quinconce.
        plan.expectedIds = { 4, 5 };
        plan.tablePointsCm = {
            cv::Point2f(163.5f, 0.0f),    // ID 4 : haut (poche - 15cm)
            cv::Point2f(193.5f, 178.0f)   // ID 5 : bas  (poche + 15cm)
        };
    }
    else
    {
        // Camera 3 : 4 coins de la zone 227 -> 357 cm
        plan.expectedIds = { 6, 7, 8, 9 };
        plan.tablePointsCm = {
            cv::Point2f(227.0f, 0.0f),    // ID 6 : haut, cote camera 2
            cv::Point2f(357.0f, 0.0f),    // ID 7 : haut, cote noire
            cv::Point2f(227.0f, 178.0f),  // ID 8 : bas,  cote camera 2
            cv::Point2f(357.0f, 178.0f)   // ID 9 : bas,  cote noire
        };
    }

    return plan;
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
    // Detection de TOUS les marqueurs visibles sur la photo, puis on
    // ne garde que ceux attendus pour cette camera.
    // -----------------------------------------------------------------
    CameraMarkerPlan plan = getMarkerPlan(cameraIndex);

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);

    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> rejected;
    detector.detectMarkers(frame, corners, ids, rejected);

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
    }

    // Ne garder que les marqueurs attendus pour cette camera, dans
    // l'ordre du plan (important pour que image<->table se correspondent).
    std::vector<cv::Point2f> foundImagePoints;
    std::vector<cv::Point2f> foundTablePoints;
    std::vector<int> missingIds;

    for (size_t i = 0; i < plan.expectedIds.size(); i++)
    {
        int id = plan.expectedIds[i];
        auto it = markerCenters.find(id);
        if (it != markerCenters.end())
        {
            foundImagePoints.push_back(it->second);
            foundTablePoints.push_back(plan.tablePointsCm[i]);
            std::cout << "  ID " << id << " (marqueur " << (id + 1) << ") detecte au centre ("
                      << it->second.x << ", " << it->second.y << ")" << std::endl;
        }
        else
        {
            missingIds.push_back(id);
        }
    }

    std::cout << "Marqueurs detectes : " << foundImagePoints.size()
              << " / " << plan.expectedIds.size() << " attendus pour la camera " << cameraIndex << std::endl;

    if (!missingIds.empty())
    {
        std::cout << "Erreur : marqueur(s) manquant(s) - ID";
        for (int id : missingIds) std::cout << " " << id << " (marqueur " << (id + 1) << ")";
        std::cout << ". Verifiez l'eclairage, la nettete, et qu'aucun marqueur n'est masque." << std::endl;
        return 1;
    }

    // -----------------------------------------------------------------
    // Choix automatique de la methode selon le nombre de marqueurs :
    //   2 marqueurs -> calibrateFromTwoPoints (Camera 2, verticale)
    //   4 marqueurs -> calibrate / homographie complete (Cameras 1 et 3, inclinees)
    // -----------------------------------------------------------------
    CameraCalibration calib;
    bool ok = false;

    if (foundImagePoints.size() == 2)
    {
        std::cout << "2 marqueurs detectes -> calibration simplifiee a 2 points (similitude)." << std::endl;
        ok = calib.calibrateFromTwoPoints(
            foundImagePoints[0], foundImagePoints[1],
            foundTablePoints[0], foundTablePoints[1]
        );
    }
    else if (foundImagePoints.size() == 4)
    {
        std::cout << "4 marqueurs detectes -> calibration complete a 4 points (homographie)." << std::endl;
        ok = calib.calibrate(foundImagePoints, foundTablePoints);
    }
    else
    {
        std::cout << "Erreur : nombre de marqueurs inattendu (" << foundImagePoints.size()
                   << "), impossible de choisir une methode de calibration." << std::endl;
        return 1;
    }

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
