#pragma once

#include <string>
#include <vector>

class Frame;

// Rejeu d'un scenario_*.txt (journal des coups exporte par l'appli) sur une
// Frame : module sans Qt, partage par la telecommande ("Rejouer le
// scenario") et par les tests automatiques (tests/EngineTests.cpp), pour
// que les deux utilisent EXACTEMENT la meme logique.

// Une action extraite du journal, dans l'ordre.
struct ReplayAction
{
    // TouchingBall = arme Frame::setTouchingBall(true) sans jouer de coup.
    // BlancheOffTable = faute directe sur la blanche (Frame::playShot() ne
    // sait pas traiter la blanche comme une bille jouable normale).
    // DirectFoul = faute "Miss" pure ou la bille jouee EST la bille demandee
    // (aucune bille distincte touchee) : rejouer via playShot() la compterait
    // a tort comme legale, donc Frame::foul() est appele directement.
    // FreeBallShot = bille empochee pendant un Free Ball (valeur enregistree
    // differente de la valeur standard) : rejeu via Frame::playFreeBall().
    // FreeBallFoul = faute survenue PENDANT un Free Ball (motif "Faute pendant
    // un Free Ball") : seul playFreeBall() desarme correctement le Free Ball.
    // Replay = "Faire rejouer" (Frame::requestReplay()).
    enum class Kind { Shot, Miss, TouchingBall, BlancheOffTable, DirectFoul, FreeBallShot, FreeBallFoul, Replay };

    Kind kind = Kind::Shot;
    std::string ballName;        // vide sauf Shot/DirectFoul/FreeBallShot ; bille DESIGNEE pour FreeBallFoul
    std::string touchedBallName; // uniquement FreeBallFoul : bille reellement touchee/jouee
};

// Valeur standard d'une bille par son nom (0 si inconnue).
int standardBallValueOf(const std::string& ballName);

// Extrait les actions d'un texte de scenario (une ligne numerotee "N. ..."
// par evenement ; l'en-tete et les lignes non numerotees sont ignores).
std::vector<ReplayAction> parseScenarioText(const std::string& text);

// Applique UNE action a la frame (sans afterShot() : c'est a l'appelant de
// laisser le Match/GameManager traiter une eventuelle fin de frame).
void applyReplayAction(Frame& frame, const ReplayAction& action);
