#include "VisionGameBridge.h"
#include "Frame.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <set>

// =====================================================================
// VisionGameBridgeDemo
// ---------------------------------------------------------------------
// Demonstration/test du pont MesureVision <-> moteur de jeu : simule
// une courte sequence de coups en generant des images de table
// synthetiques (aucune camera necessaire), et verifie que le score du
// vrai moteur de jeu (Frame) evolue correctement au fil des coups.
//
// Scenario joue :
//   1. Table de depart (3 rouges + les 6 couleurs + la blanche).
//   2. Coup 1 : une rouge disparait (empochee) -> +1 point attendu.
//   3. Coup 2 : la noire disparait (couleur apres une rouge, respotee)
//      -> +7 points attendus.
//   4. La noire reapparait a sa place (respot physique par l'arbitre)
//      -> AUCUN point supplementaire attendu (pas un nouveau pot).
//   5. Coup 3 : la blanche disparait (empochee par erreur) -> faute,
//      l'adversaire doit gagner des points.
// =====================================================================

namespace
{
    struct BallSpec
    {
        std::string name;
        cv::Point pos;
        cv::Scalar color;
    };

    const std::vector<BallSpec> kLayout = {
        { "Rouge1",  {150, 150}, cv::Scalar(0,   0,   255) },
        { "Rouge2",  {200, 150}, cv::Scalar(0,   0,   255) },
        { "Rouge3",  {250, 150}, cv::Scalar(0,   0,   255) },
        { "Jaune",   {350, 150}, cv::Scalar(0,   220, 220) },
        { "Verte",   {450, 150}, cv::Scalar(0,   180, 0) },
        { "Marron",  {550, 150}, cv::Scalar(20,  80,  140) },
        { "Bleue",   {350, 300}, cv::Scalar(200, 50,  30) },
        { "Rose",    {450, 300}, cv::Scalar(180, 120, 255) },
        { "Noire",   {550, 300}, cv::Scalar(20,  20,  20) },
        { "Blanche", {700, 450}, cv::Scalar(240, 240, 240) },
    };

    // Construit l'image de la table, en omettant les billes dont le nom
    // figure dans `absent` (simule des billes empochees/disparues).
    cv::Mat buildScene(const std::set<std::string>& absent)
    {
        cv::Mat table(600, 900, CV_8UC3, cv::Scalar(50, 50, 50));
        for (const auto& b : kLayout)
        {
            if (absent.count(b.name))
            {
                continue;
            }
            cv::circle(table, b.pos, 18, b.color, -1);
        }
        return table;
    }

    void printState(const std::string& label, Frame& frame)
    {
        std::cout << "[" << label << "] Joueur au tir : " << frame.currentPlayer().getName()
            << " | Score J1=" << frame.getPlayer1().getScore()
            << " Score J2=" << frame.getPlayer2().getScore()
            << " | Break=" << frame.currentPlayer().getBreak()
            << std::endl;
    }
}

int main()
{
    Frame frame;
    frame.setPlayerNames("Joueur 1", "Joueur 2");

    // maxFramesLost reduit a 5 (au lieu du defaut 15) pour que la demo
    // reste courte : le principe est identique, seul le nombre d'images
    // necessaires pour confirmer une disparition change.
    VisionGameBridge bridge(/*maxFramesLost=*/5, /*maxMatchDistancePx=*/40.f);

    std::cout << "=== Etat initial ===" << std::endl;
    printState("depart", frame);
    std::cout << std::endl;

    // ---------------------------------------------------------------
    // Etape 0 : quelques images stables, pour etablir le suivi initial
    // (toutes les billes presentes).
    // ---------------------------------------------------------------
    std::set<std::string> absent;
    for (int i = 0; i < 3; ++i)
    {
        cv::Mat img = buildScene(absent);
        bridge.processImage(img, frame);
    }
    std::cout << "=== Apres stabilisation initiale (aucun coup) ===" << std::endl;
    printState("stable", frame);
    std::cout << std::endl;

    // ---------------------------------------------------------------
    // Coup 1 : une rouge disparait. Il faut plusieurs images consecutives
    // sans la revoir pour que ce soit confirme (evite les faux positifs).
    // ---------------------------------------------------------------
    absent.insert("Rouge1");
    std::vector<std::string> log1;
    for (int i = 0; i < 8; ++i)
    {
        cv::Mat img = buildScene(absent);
        bridge.processImage(img, frame, &log1);
    }
    std::cout << "=== Coup 1 : une rouge empochee ===" << std::endl;
    for (const auto& line : log1) std::cout << "  " << line << std::endl;
    printState("coup 1", frame);
    std::cout << std::endl;

    // ---------------------------------------------------------------
    // Coup 2 : la noire disparait (couleur jouee apres la rouge, donc
    // legale et respotee).
    // ---------------------------------------------------------------
    absent.insert("Noire");
    std::vector<std::string> log2;
    for (int i = 0; i < 8; ++i)
    {
        cv::Mat img = buildScene(absent);
        bridge.processImage(img, frame, &log2);
    }
    std::cout << "=== Coup 2 : la noire empochee (apres une rouge) ===" << std::endl;
    for (const auto& line : log2) std::cout << "  " << line << std::endl;
    printState("coup 2", frame);
    std::cout << std::endl;

    // ---------------------------------------------------------------
    // La noire est respotee physiquement (redetectee a sa place) :
    // aucun nouveau point ne doit etre attribue, ce n'est pas un
    // nouveau coup.
    // ---------------------------------------------------------------
    absent.erase("Noire");
    std::vector<std::string> log3;
    for (int i = 0; i < 8; ++i)
    {
        cv::Mat img = buildScene(absent);
        bridge.processImage(img, frame, &log3);
    }
    std::cout << "=== Noire respotee (pas un nouveau coup) ===" << std::endl;
    std::cout << "  Actions declenchees : " << log3.size() << " (attendu : 0)" << std::endl;
    printState("respot", frame);
    std::cout << std::endl;

    // ---------------------------------------------------------------
    // Coup 3 : la blanche disparait -> faute automatique.
    // ---------------------------------------------------------------
    absent.insert("Blanche");
    std::vector<std::string> log4;
    for (int i = 0; i < 8; ++i)
    {
        cv::Mat img = buildScene(absent);
        bridge.processImage(img, frame, &log4);
    }
    std::cout << "=== Coup 3 : la blanche empochee (faute) ===" << std::endl;
    for (const auto& line : log4) std::cout << "  " << line << std::endl;
    printState("coup 3", frame);

    return 0;
}