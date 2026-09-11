#pragma once
#include "BallDetector.h"
#include <map>
#include <vector>
#include <string>

// =====================================================================
// TrackedBall
// ---------------------------------------------------------------------
// Position suivie dans le temps pour UNE bille precise, avec le nombre
// d'images consecutives depuis la derniere VRAIE detection (0 = detectee
// a l'instant present, >0 = on utilise la derniere position connue).
// =====================================================================
struct TrackedBall
{
    cv::Point2f lastPosition;
    float radius = 0.f;
    int framesSinceSeen = 0;
};

// =====================================================================
// RemovedBall
// ---------------------------------------------------------------------
// Une bille que update() vient de retirer du suivi (non revue depuis
// plus de maxFramesLost images), avec sa DERNIERE position connue.
// Renvoyee par update() pour que l'appelant (VisionGameBridge) puisse
// determiner OU elle a disparu (pres d'une poche, pres du bord, ou
// ailleurs) au lieu de supposer aveuglement un empochage valide.
// =====================================================================
struct RemovedBall
{
    std::string colorName;
    cv::Point2f lastPosition;
    float radius = 0.f;
};

// =====================================================================
// BallTracker
// ---------------------------------------------------------------------
// Combine les detections d'une image avec la memoire des positions
// precedentes, PAR COULEUR. Important : il y a 15 billes rouges en
// meme temps sur la table, donc chaque couleur garde une LISTE de
// positions suivies (pas une seule), avec un appariement au plus proche
// voisin d'une image a l'autre pour ne pas "melanger" les rouges entre eux.
//
// Utile pour :
//   - Ne pas "perdre" une bille a cause d'un reflet, d'une main qui
//     passe devant, ou d'un rate de detection ponctuel.
//   - Distinguer une bille "encore sur la table mais pas vue cette
//     image" d'une bille "reellement empochee" (voir maxFramesLost).
// =====================================================================
class BallTracker
{
public:
    // maxFramesLost : nombre d'images consecutives sans detection
    // avant de considerer une bille comme reellement disparue (empochee).
    // maxMatchDistancePx : distance max (en pixels) pour qu'une detection
    // soit consideree comme "la meme bille" qu'au tour precedent.
    explicit BallTracker(int maxFramesLost = 15, float maxMatchDistancePx = 40.f);

    // A appeler a chaque nouvelle image, avec les billes fraichement
    // detectees (detectBalls() et/ou detectBallsByShape() combines).
    // Renvoie les billes qui viennent d'etre definitivement retirees du
    // suivi pendant CET appel (non revues depuis plus de maxFramesLost
    // images), avec leur derniere position connue.
    std::vector<RemovedBall> update(const std::vector<DetectedBall>& currentDetections);

    // Nombre de billes actuellement suivies pour une couleur (encore
    // consideres presentes sur la table, meme si pas vues cette image).
    int countTracked(const std::string& colorName) const;

    // Renvoie toutes les positions suivies pour une couleur donnee.
    std::vector<TrackedBall> getTracked(const std::string& colorName) const;

    // Renvoie l'etat suivi complet, toutes couleurs confondues (utilise
    // par exemple pour l'alimenter directement a BallMapRecorder).
    const std::map<std::string, std::vector<TrackedBall>>& getAllTracked() const;

    // Une bille (a un index donne dans sa couleur) est consideree comme
    // empochee/disparue si elle n'a pas ete revue depuis plus de
    // maxFramesLost images. Cette fonction nettoie ces billes perdues
    // et renvoie le nombre de billes qui viennent de "disparaitre"
    // pour cette couleur (utile pour detecter un potentiel pot de bille).
    int removeStaleBalls(const std::string& colorName);

    void reset();

private:
    std::map<std::string, std::vector<TrackedBall>> m_tracked;
    int m_maxFramesLost;
    float m_maxMatchDistancePx;
};