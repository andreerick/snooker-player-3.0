#pragma once

#include <string>
#include <vector>

// Geometrie de la "position de snooker" (Sect. 2 §17 du reglement) : sert a
// detecter qu'un joueur arrive snooke apres une faute, donc qu'une Free
// Ball est peut-etre possible (Sect. 2 §13, Sect. 3 §12).
//
// Module purement geometrique (aucune dependance Qt/OpenCV). Les positions
// sont en centimetres dans un repere commun de la table (le meme pour toutes
// les billes) ; c'est a l'appelant de convertir les positions camera (voir
// CameraCalibration) avant l'appel. Ne sert qu'a SUGGERER : la decision
// reste a l'arbitre.
//
// Limites connues : la bille en main (Sect. 2 §17(a), a tester depuis tous
// les points du "D") n'est pas geree -- l'appelant doit passer une position
// precise de blanche ; les bandes sont ignorees (une bande ne snooke jamais,
// §17(e)).

struct PositionedBall
{
    std::string name; // "Blanche", "Rouge", "Jaune", "Verte", "Marron", "Bleue", "Rose", "Noire"
    double x = 0.0;   // cm
    double y = 0.0;   // cm
};

enum class SnookerState
{
    NotSnookered, // au moins une bille jouable touchable sur ses 2 bords, sans ambiguite
    Borderline,   // cas limite (a quelques mm pres) : a SUGGERER a l'arbitre
    Snookered     // aucune bille jouable touchable sur ses 2 bords, sans ambiguite
};

struct SnookerAssessment
{
    SnookerState state = SnookerState::NotSnookered;
    // Meilleure marge de degagement (cm) parmi les billes jouables : > 0 =
    // passage libre sur les 2 bords, < 0 = bloque. Sert de mesure de
    // confiance (proche de 0 = cas litigieux).
    double margin = 0.0;
    // Nom de la bille qui gene le mieux placee (la plus contraignante pour
    // la meilleure cible), vide si NotSnookered.
    std::string blockingBall;
};

// Rayon d'une bille de snooker (diametre 52,5 mm), en cm.
constexpr double kSnookerBallRadiusCm = 2.625;

// Traduit la bille demandee par le moteur (Frame::getRequiredBall() : "Rouge",
// "Couleur" pour "n'importe quelle couleur", ou une couleur precise) en
// ensemble de billes jouables parmi celles presentes sur la table.
std::vector<PositionedBall> ballsOn(const std::string& requiredBall,
                                    const std::vector<PositionedBall>& balls);

// Evalue si la blanche est snookee. `on` = billes jouables (voir ballsOn()),
// `blockers` = les billes NON jouables de la table (les seules qui peuvent
// snooker : toucher une bille jouable, meme derriere une autre, n est pas
// un snooker). `toleranceCm` = incertitude de position (erreur camera,
// ~0,5 cm) : en dessous, le resultat est Borderline.
SnookerAssessment assessSnooker(const PositionedBall& cue,
                                const std::vector<PositionedBall>& on,
                                const std::vector<PositionedBall>& blockers,
                                double toleranceCm = 0.5);

// Raccourci : `allBalls` = toutes les billes de la table SANS la blanche,
// `requiredBall` = Frame::getRequiredBall().getName(). Deduit les billes
// jouables (ballsOn()) et les obstacles (toutes les autres), puis appelle
// assessSnooker().
SnookerAssessment assessSnookerOnTable(const PositionedBall& cue,
                                       const std::vector<PositionedBall>& allBalls,
                                       const std::string& requiredBall,
                                       double toleranceCm = 0.5);
