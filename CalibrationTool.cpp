#include "CameraCalibration.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

// =====================================================================
// Outil de calibration interactif
// ---------------------------------------------------------------------
// Affiche l'image d'une camera, et permet de cliquer les 4 coins de la
// zone de table qu'elle observe (dans l'ordre : haut-gauche, haut-droit,
// bas-droit, bas-gauche). Une fois les 4 points cliques, l'homographie
// est calculee et sauvegardee dans un fichier .yml.
//
// Utilisation :
//   CalibrationTool.exe <index_camera 0/1/2> <chemin_image_ou_flux>
//
// Exemple avec une image de test (avant d'avoir les vraies cameras) :
//   CalibrationTool.exe 0 test_zone1.jpg
//
// Exemple avec une vraie camera USB branchee (index Windows, souvent 0,1,2...) :
//   CalibrationTool.exe 0 0
// =====================================================================

struct ClickState
{
    std::vector<cv::Point2f> points;
    cv::Mat displayImage;
};

void onMouseClick(int event, int x, int y, int, void* userdata)
{
    if (event != cv::EVENT_LBUTTONDOWN)
    {
        return;
    }

    ClickState* state = static_cast<ClickState*>(userdata);

    if (state->points.size() >= 4)
    {
        std::cout << "4 points deja places. Appuie sur 'r' pour recommencer, "
                   << "ou une autre touche pour valider." << std::endl;
        return;
    }

    state->points.push_back(cv::Point2f(static_cast<float>(x), static_cast<float>(y)));

    cv::circle(state->displayImage, cv::Point(x, y), 6, cv::Scalar(0, 255, 0), -1);
    cv::putText(
        state->displayImage,
        std::to_string(state->points.size()),
        cv::Point(x + 10, y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.7,
        cv::Scalar(0, 255, 0),
        2
    );

    std::cout << "Point " << state->points.size() << " place a ("
               << x << ", " << y << ")" << std::endl;

    if (state->points.size() == 4)
    {
        std::cout << std::endl;
        std::cout << "Les 4 points sont places !" << std::endl;
        std::cout << "Appuie sur une touche pour calculer et sauvegarder "
                   << "la calibration, ou 'r' pour recommencer." << std::endl;
    }

    cv::imshow("Calibration - clique les 4 coins", state->displayImage);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Utilisation : CalibrationTool.exe <index_camera 0/1/2> <chemin_image_ou_index_flux>" << std::endl;
        std::cout << "Exemple      : CalibrationTool.exe 0 test_zone1.jpg" << std::endl;
        return 1;
    }

    int cameraIndex = std::stoi(argv[1]);
    std::string source = argv[2];

    if (cameraIndex < 0 || cameraIndex >= CameraSystem::NB_CAMERAS)
    {
        std::cout << "Erreur : l'index camera doit etre 0, 1 ou 2." << std::endl;
        return 1;
    }

    // -----------------------------------------------------------------
    // Chargement de l'image (photo fixe) ou d'un flux camera en direct.
    // Si "source" est un nombre, on considere que c'est un index de
    // camera USB branchee. Sinon, on essaie de l'ouvrir comme fichier image.
    // -----------------------------------------------------------------
    cv::Mat frame;
    bool isNumber = !source.empty() &&
        std::all_of(source.begin(), source.end(), ::isdigit);

    if (isNumber)
    {
        cv::VideoCapture cap(std::stoi(source));
        if (!cap.isOpened())
        {
            std::cout << "Impossible d'ouvrir la camera " << source << std::endl;
            return 1;
        }
        cap >> frame;
        if (frame.empty())
        {
            std::cout << "Aucune image recue de la camera." << std::endl;
            return 1;
        }
    }
    else
    {
        frame = cv::imread(source);
        if (frame.empty())
        {
            std::cout << "Impossible de charger l'image : " << source << std::endl;
            return 1;
        }
    }

    // -----------------------------------------------------------------
    // Les 4 coins REELS (en cm) de la zone couverte par cette camera.
    // A ADAPTER selon la zone exacte de chaque camera sur ta table
    // (357 x 178 cm, ~130cm par camera avec recouvrement).
    // Ordre : haut-gauche, haut-droit, bas-droit, bas-gauche.
    // -----------------------------------------------------------------
    std::vector<cv::Point2f> tableCornersCm;
    float zoneLength = 130.0f; // longueur de la zone (a ajuster par camera)
    float tableWidth = 178.0f;
    float zoneStart = cameraIndex * 119.0f; // ~1/3 de la table, avec recouvrement

    tableCornersCm.push_back(cv::Point2f(zoneStart, 0.0f));
    tableCornersCm.push_back(cv::Point2f(zoneStart + zoneLength, 0.0f));
    tableCornersCm.push_back(cv::Point2f(zoneStart + zoneLength, tableWidth));
    tableCornersCm.push_back(cv::Point2f(zoneStart, tableWidth));

    std::cout << "=====================================================" << std::endl;
    std::cout << "Calibration de la camera " << cameraIndex << std::endl;
    std::cout << "=====================================================" << std::endl;
    std::cout << "Clique les 4 coins de la zone de table visible, DANS CET ORDRE :" << std::endl;
    std::cout << "  1. Coin haut-gauche" << std::endl;
    std::cout << "  2. Coin haut-droit" << std::endl;
    std::cout << "  3. Coin bas-droit" << std::endl;
    std::cout << "  4. Coin bas-gauche" << std::endl;
    std::cout << std::endl;
    std::cout << "Coordonnees reelles attendues pour ces 4 coins (cm) :" << std::endl;
    for (size_t i = 0; i < tableCornersCm.size(); i++)
    {
        std::cout << "  Point " << (i + 1) << " : ("
                   << tableCornersCm[i].x << ", " << tableCornersCm[i].y << ")" << std::endl;
    }
    std::cout << std::endl;

    ClickState state;
    state.displayImage = frame.clone();

    cv::namedWindow("Calibration - clique les 4 coins");
    cv::setMouseCallback("Calibration - clique les 4 coins", onMouseClick, &state);
    cv::imshow("Calibration - clique les 4 coins", state.displayImage);

    while (true)
    {
        int key = cv::waitKey(0);

        if (key == 'r' || key == 'R')
        {
            state.points.clear();
            state.displayImage = frame.clone();
            cv::imshow("Calibration - clique les 4 coins", state.displayImage);
            std::cout << "Points effaces, recommence." << std::endl;
            continue;
        }

        if (state.points.size() == 4)
        {
            break;
        }

        std::cout << "Il faut placer 4 points avant de valider ("
                   << state.points.size() << "/4 places)." << std::endl;
    }

    // -----------------------------------------------------------------
    // Calcul et sauvegarde de la calibration
    // -----------------------------------------------------------------
    CameraCalibration calib;
    bool ok = calib.calibrate(state.points, tableCornersCm);

    if (!ok)
    {
        std::cout << "Echec de la calibration. Reessaie avec des points plus precis." << std::endl;
        return 1;
    }

    std::string outputPath = "camera_" + std::to_string(cameraIndex) + ".yml";
    if (calib.saveToFile(outputPath))
    {
        std::cout << "Calibration sauvegardee dans : " << outputPath << std::endl;
    }

    // Petit test : reconvertit le premier point clique, doit redonner
    // (a peu pres) le premier coin reel.
    cv::Point2f testResult = calib.imageToTable(state.points[0]);
    std::cout << std::endl;
    std::cout << "Verification : le premier point clique redonne ("
               << testResult.x << ", " << testResult.y << ") cm, "
               << "attendu (" << tableCornersCm[0].x << ", " << tableCornersCm[0].y << ") cm." << std::endl;

    cv::waitKey(0);
    return 0;
}
