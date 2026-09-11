#include "TableCapture.h"
#include "BallDetector.h"
#include "BallTracker.h"
#include "BallMapRecorder.h"
#include "CameraCalibration.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

// =====================================================================
// MesureVisionDemo
// ---------------------------------------------------------------------
// Assemble en un seul outil les briques de MesureVision : assemblage
// des 3 images cameras (TableCapture), detection des billes par couleur
// (BallDetector), suivi (BallTracker), cartographie (BallMapRecorder :
// schema visuel + fichier .yml), conversion en position reelle sur la
// table (cm) via une calibration existante (CameraCalibration), et
// guide de repositionnement des billes (BallMapRecorder, roadmap v1.0).
//
// Prevu pour etre appele une fois APRES CHAQUE COUP (pas seulement en
// fin de frame) : la photo brute de la table est systematiquement
// sauvegardee, pour servir de preuve visuelle en cas de contestation
// ou de miss a verifier (l'arbitre/le joueur peut revoir la photo,
// meme si la detection automatique s'est trompee).
//
// Fonctionne sur des images statiques (photos ou images de test) :
// aucune camera physique n'est necessaire pour l'utiliser.
//
// Utilisation :
//   MesureVisionDemo.exe <image.png> [dossier_sortie]
//       [--calib fichier.yml] [--shot N] [--reposition cible.yml]
//     -> image de table deja assemblee (une seule photo).
//        --calib : affiche en plus la position reelle (cm) de chaque bille.
//        --reposition : compare l'etat actuel a un instantane cible
//          (sauvegarde par un coup precedent) et genere un guide visuel
//          de repositionnement (reposition_guide.png).
//   MesureVisionDemo.exe <cam0.png> <cam1.png> <cam2.png> [dossier_sortie] [--shot N]
//     -> 3 photos separees (une par camera), assemblees via TableCapture
//        avant detection, comme dans le vrai pipeline a 3 cameras.
//        (--calib et --reposition non supportes ici : il faudrait une
//        calibration par camera, recalculee sur l'image assemblee.)
//
// --shot N : numero du coup en cours (defaut 1), utilise pour nommer
// la photo et la cartographie sauvegardees (table_coup000N_..., etc.),
// pour pouvoir retrouver facilement les preuves d'un coup precis.
// =====================================================================
int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);

    // Extrait une option "--nom valeur" de args, ou renvoie defaultValue
    // si absente. Retire les 2 tokens de args au passage.
    auto extractOption = [&args](const std::string& name, const std::string& defaultValue) -> std::string
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (args[i] == name && i + 1 < args.size())
                {
                    std::string value = args[i + 1];
                    args.erase(args.begin() + static_cast<long>(i), args.begin() + static_cast<long>(i) + 2);
                    return value;
                }
            }
            return defaultValue;
        };

    std::string calibPath = extractOption("--calib", "");
    std::string repositionPath = extractOption("--reposition", "");
    int shotNumber = std::stoi(extractOption("--shot", "1"));

    if (args.empty())
    {
        std::cout << "Utilisation :" << std::endl;
        std::cout << "  MesureVisionDemo.exe <image.png> [dossier_sortie] [--calib fichier.yml] [--shot N] [--reposition cible.yml]" << std::endl;
        std::cout << "  MesureVisionDemo.exe <cam0.png> <cam1.png> <cam2.png> [dossier_sortie] [--shot N]" << std::endl;
        return 1;
    }

    // 3 ou 4 arguments restants = mode "3 cameras a assembler".
    // Sinon (1 ou 2) = mode "image de table deja assemblee".
    bool threeCameraMode = (args.size() == 3 || args.size() == 4);

    cv::Mat frame;
    std::string outputFolder;

    if (threeCameraMode)
    {
        cv::Mat cam0 = cv::imread(args[0]);
        cv::Mat cam1 = cv::imread(args[1]);
        cv::Mat cam2 = cv::imread(args[2]);

        if (cam0.empty() || cam1.empty() || cam2.empty())
        {
            std::cout << "Impossible de charger une ou plusieurs images cameras." << std::endl;
            return 1;
        }

        std::cout << "3 images cameras chargees, assemblage en cours..." << std::endl;

        TableCapture capture;
        frame = capture.buildFullTableImage(cam0, cam1, cam2);

        outputFolder = (args.size() == 4) ? args[3] : ".";
    }
    else
    {
        frame = cv::imread(args[0]);
        outputFolder = (args.size() >= 2) ? args[1] : ".";

        if (frame.empty())
        {
            std::cout << "Impossible de charger l'image : " << args[0] << std::endl;
            return 1;
        }
    }

    std::cout << "Image de table : " << frame.cols << "x" << frame.rows << std::endl;

    // -----------------------------------------------------------------
    // 0. Photo brute de la table pour CE coup, avant toute analyse :
    //    c'est la seule preuve fiable de l'etat reel de la table si la
    //    detection automatique se trompe (miss, contestation, etc.).
    // -----------------------------------------------------------------
    TableCapture capture;
    capture.saveSnapshot(frame, outputFolder, shotNumber);

    // -----------------------------------------------------------------
    // 1. Detection des billes : couleur (HSV) en priorite, complete par
    //    la forme (Hough) pour les couleurs uniques non trouvees par la
    //    couleur seule (ex: bille verte qui se fond dans le tapis).
    // -----------------------------------------------------------------
    BallDetector detector;
    std::vector<DetectedBall> detections = detector.detectBallsCombined(frame, 8, 40);
    std::cout << detections.size() << " bille(s) detectee(s) sur l'image." << std::endl;

    // -----------------------------------------------------------------
    // 2. Suivi (une seule image ici, mais meme mecanisme qu'en video :
    //    en flux continu, update() serait appele a chaque nouvelle image).
    // -----------------------------------------------------------------
    BallTracker tracker;
    tracker.update(detections);

    for (const auto& entry : tracker.getAllTracked())
    {
        std::cout << "  " << entry.first << " : " << entry.second.size() << " bille(s) suivie(s)" << std::endl;
    }

    // -----------------------------------------------------------------
    // 3. Cartographie : image visuelle (pour un humain) + fichier .yml
    //    (donnees brutes rechargeables).
    // -----------------------------------------------------------------
    BallMapRecorder recorder(frame.size());
    cv::Mat visualMap = recorder.renderVisualMap(tracker.getAllTracked());

    std::string mapImagePath = outputFolder + "/ball_map.png";
    cv::imwrite(mapImagePath, visualMap);
    std::cout << "Schema visuel sauvegarde : " << mapImagePath << std::endl;

    recorder.saveSnapshot(tracker.getAllTracked(), outputFolder, shotNumber);

    // -----------------------------------------------------------------
    // 4. Mesure reelle (cm) : convertit chaque position pixel en
    //    coordonnee reelle sur la table, via une calibration existante
    //    (fichier .yml genere par CalibrationTool ou ArucoCalibrationTool).
    // -----------------------------------------------------------------
    double cmPerPixel = 0.0;
    CameraCalibration calib;
    bool hasCalib = false;

    if (!calibPath.empty())
    {
        hasCalib = calib.loadFromFile(calibPath);
        if (!hasCalib)
        {
            std::cout << "Impossible de charger la calibration : " << calibPath << std::endl;
        }
        else
        {
            std::cout << std::endl << "Positions reelles sur la table (calibration "
                << calibPath << ") :" << std::endl;
            for (const auto& ball : detections)
            {
                cv::Point2f cm = calib.imageToTable(ball.pixelPosition);
                std::cout << "  " << ball.colorName << " a ("
                    << cm.x << ", " << cm.y << ") cm" << std::endl;
            }

            // Echelle cm/pixel approximative (distance reelle entre 2
            // points image separes d'1 pixel), pour afficher les
            // distances de repositionnement en cm plutot qu'en pixels.
            cv::Point2f p0 = calib.imageToTable(cv::Point2f(0.f, 0.f));
            cv::Point2f p1 = calib.imageToTable(cv::Point2f(1.f, 0.f));
            cmPerPixel = std::hypot(p1.x - p0.x, p1.y - p0.y);
        }
    }

    // -----------------------------------------------------------------
    // 5. Guide de repositionnement (roadmap v1.0) : compare l'etat
    //    actuel a un instantane cible (coup precedent), pour aider a
    //    remettre les billes en place apres une contestation ou un miss.
    // -----------------------------------------------------------------
    if (!repositionPath.empty())
    {
        std::map<std::string, std::vector<cv::Point2f>> target = recorder.loadSnapshot(repositionPath);
        if (target.empty())
        {
            std::cout << "Impossible de charger l'instantane cible : " << repositionPath << std::endl;
        }
        else
        {
            cv::Mat guide = recorder.renderRepositioningGuide(tracker.getAllTracked(), target, cmPerPixel);
            std::string guidePath = outputFolder + "/reposition_guide.png";
            cv::imwrite(guidePath, guide);
            std::cout << "Guide de repositionnement sauvegarde : " << guidePath << std::endl;
        }
    }

    return 0;
}