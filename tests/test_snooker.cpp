// Tests unitaires pour Snooker Player 3.0
// Utilise un framework de test minimal (aucune dependance externe)

#include <iostream>
#include <stdexcept>
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include <cmath>

#include "../Ball.h"
#include "../BallSet.h"
#include "../Player.h"
#include "../Frame.h"
#include "../Referee.h"
#include "../Match.h"
#include "../Shot.h"
#include "../ShotHistory.h"
#include "../core/Table.h"
#include "../camera/CameraConfig.h"
#include "../camera/CameraCalibration.h"
#include "../camera/CameraFusion.h"
#include "../api/JsonSerializer.h"
#include "../GameManager.h"

// --- Framework de test minimal ---
namespace Test {

int passed = 0;
int failed = 0;
std::vector<std::string> failures;

void assert_true(bool condition, const std::string& name)
{
    if (condition)
    {
        ++passed;
        std::cout << "  [OK] " << name << std::endl;
    }
    else
    {
        ++failed;
        failures.push_back(name);
        std::cout << "  [FAIL] " << name << std::endl;
    }
}

void assert_eq(int a, int b, const std::string& name)
{
    bool ok = (a == b);
    if (!ok)
        std::cout << "       attendu=" << b << " obtenu=" << a << std::endl;
    assert_true(ok, name);
}

void assert_eq(const std::string& a, const std::string& b, const std::string& name)
{
    bool ok = (a == b);
    if (!ok)
        std::cout << "       attendu=\"" << b << "\" obtenu=\"" << a << "\"" << std::endl;
    assert_true(ok, name);
}

void section(const std::string& title)
{
    std::cout << "\n[" << title << "]" << std::endl;
}

int summary()
{
    std::cout << "\n=== Resultats : " << passed << " OK / "
              << failed << " FAIL ===" << std::endl;
    if (!failures.empty())
    {
        std::cout << "Tests en echec :" << std::endl;
        for (const auto& f : failures)
            std::cout << "  - " << f << std::endl;
    }
    return failed == 0 ? 0 : 1;
}

} // namespace Test
using namespace Test;

// =======================================================================
// Tests Ball
// =======================================================================
void testBall()
{
    section("Ball");

    Ball rouge("Rouge", 1);
    assert_eq(rouge.getName(), "Rouge", "Ball::getName() Rouge");
    assert_eq(rouge.getValue(), 1,      "Ball::getValue() Rouge=1");

    Ball noir("Noir", 7);
    assert_eq(noir.getName(), "Noir", "Ball::getName() Noir");
    assert_eq(noir.getValue(), 7,     "Ball::getValue() Noir=7");

    Ball blanche("Blanche", 0);
    assert_eq(blanche.getValue(), 0, "Ball::getValue() Blanche=0");
}

// =======================================================================
// Tests BallSet
// =======================================================================
void testBallSet()
{
    section("BallSet");

    BallSet bs;
    const auto& balls = bs.getBalls();

    // 15 rouges + 6 couleurs + blanche = 22
    assert_eq(static_cast<int>(balls.size()), 22, "BallSet : 22 billes au total");

    // Compter les rouges
    int reds = 0;
    for (const auto& b : balls)
        if (b.getName() == "Rouge") ++reds;
    assert_eq(reds, 15, "BallSet : 15 rouges");

    // Verifier la terminologie (pas de 'Noire', 'Verte', 'Bleue')
    bool hasNoir  = false;
    bool hasNoNoire = true;
    bool hasVert  = false;
    bool hasNoVerte = true;
    bool hasBleu  = false;
    bool hasNoBleu  = true;
    for (const auto& b : balls)
    {
        if (b.getName() == "Noir")  hasNoir  = true;
        if (b.getName() == "Noire") hasNoNoire = false;
        if (b.getName() == "Vert")  hasVert  = true;
        if (b.getName() == "Verte") hasNoVerte = false;
        if (b.getName() == "Bleu")  hasBleu  = true;
        if (b.getName() == "Bleue") hasNoBleu  = false;
    }
    assert_true(hasNoir,    "BallSet : terminologie 'Noir' (pas 'Noire')");
    assert_true(hasNoNoire, "BallSet : pas de 'Noire'");
    assert_true(hasVert,    "BallSet : terminologie 'Vert'");
    assert_true(hasNoVerte, "BallSet : pas de 'Verte'");
    assert_true(hasBleu,    "BallSet : terminologie 'Bleu'");
    assert_true(hasNoBleu,  "BallSet : pas de 'Bleue'");
}

// =======================================================================
// Tests Player
// =======================================================================
void testPlayer()
{
    section("Player");

    Player p;
    p.setName("Alice");
    assert_eq(p.getName(), "Alice", "Player::setName/getName");
    assert_eq(p.getScore(), 0, "Player : score initial=0");

    p.addPoints(7);
    assert_eq(p.getScore(), 7, "Player::addPoints(7)");

    p.addPoints(1);
    assert_eq(p.getScore(), 8, "Player::addPoints(1) cumul");

    p.resetScore();
    assert_eq(p.getScore(), 0, "Player::resetScore");
}

// =======================================================================
// Tests Referee
// =======================================================================
void testReferee()
{
    section("Referee");

    Referee ref;

    // Contact correct : meme bille
    Ball rouge1("Rouge", 1);
    Ball rouge2("Rouge", 1);
    assert_true(ref.checkContact(rouge1, rouge2), "Referee : contact correct Rouge-Rouge");

    // Faute : mauvaise bille
    Ball noir("Noir", 7);
    assert_true(!ref.checkContact(rouge1, noir), "Referee : faute Rouge-Noir");

    // Penalite minimale = 4
    Ball bleu("Bleu", 5);
    Ball jaune("Jaune", 2);
    // required=Jaune(2) touched=bleu(5) -> penalite = max(2,5,4) = 5
    assert_eq(ref.calculateFoul(jaune, bleu), 5, "Referee : penalite Jaune->Bleu = 5");

    // Penalite minimale = 4 quand les deux sont faibles
    Ball rouge_low("Rouge", 1);
    Ball rouge_hit("Rouge", 1);
    // required=Rouge(1) touched=Rouge(1) -> min 4
    // (Mais un contact correct ne devrait pas appeler calculateFoul)
    Ball marron("Marron", 4);
    // required=Marron(4) touched=rouge(1) -> penalite = max(4,1,4) = 4
    assert_eq(ref.calculateFoul(marron, rouge_low), 4, "Referee : penalite minimale = 4");

    // Couleur requise : n'importe quelle couleur valide
    Ball couleur("Couleur", 0);
    Ball rose("Rose", 6);
    assert_true(ref.checkContact(couleur, rose), "Referee : couleur requise - Rose valide");
    assert_true(ref.checkContact(couleur, noir), "Referee : couleur requise - Noir valide");
    Ball vert("Vert", 3);
    assert_true(ref.checkContact(couleur, vert), "Referee : couleur requise - Vert valide");
    // Rouge n'est pas valide quand couleur requise
    assert_true(!ref.checkContact(couleur, rouge1), "Referee : couleur requise - Rouge invalide");
}

// =======================================================================
// Tests Frame - phases de jeu
// =======================================================================
void testFrame()
{
    section("Frame - Phases de jeu");

    Frame f;
    assert_eq(f.redsRemaining(), 15, "Frame : 15 rouges au depart");
    assert_true(!f.isFinished(), "Frame : pas terminee au depart");
    assert_true(f.getPhase() == FramePhase::Reds, "Frame : phase Reds au depart");

    // Jouer une rouge
    bool ok = f.playShot(Ball("Rouge", 1));
    assert_true(ok, "Frame : coup rouge valide");
    assert_eq(f.redsRemaining(), 14, "Frame : 14 rouges apres potRed");
    assert_eq(f.getPlayer1().getScore(), 1, "Frame : score J1 = 1 apres rouge");
    assert_true(f.isColorNeeded(), "Frame : couleur requise apres rouge");

    // Jouer une couleur (Noir = 7 pts)
    ok = f.playShot(Ball("Noir", 7));
    assert_true(ok, "Frame : coup Noir valide apres rouge");
    assert_eq(f.getPlayer1().getScore(), 8, "Frame : score J1 = 8 apres rouge+Noir");
    assert_true(!f.isColorNeeded(), "Frame : plus de couleur requise");

    // Faute : jouer une rouge quand couleur attendue - mais ici on rejoue rouge
    // Apres Noir repote, on doit jouer rouge a nouveau
    ok = f.playShot(Ball("Rouge", 1));
    assert_true(ok, "Frame : 2eme rouge valide");
    assert_eq(f.redsRemaining(), 13, "Frame : 13 rouges");
}

void testFrameFoul()
{
    section("Frame - Fautes");

    Frame f;

    // Faute : jouer une couleur quand rouge attendue
    bool ok = f.playShot(Ball("Noir", 7));
    assert_true(!ok, "Frame : faute - Noir quand Rouge attendue");
    // L'adversaire recoit la penalite : max(7, 1, 4) = 7
    assert_eq(f.getPlayer2().getScore(), 7, "Frame : penalite J2 = 7 (Noir)");
}

void testFrameFinalColors()
{
    section("Frame - Couleurs finales");

    // Simuler la fin de la phase des rouges
    Frame f;

    // Poter les 15 rouges et 15 couleurs
    for (int i = 0; i < 15; ++i)
    {
        f.playShot(Ball("Rouge", 1));
        f.playShot(Ball("Noir", 7));
    }

    assert_eq(f.redsRemaining(), 0, "Frame : 0 rouges restantes");
    assert_true(f.getPhase() == FramePhase::FinalColors, "Frame : phase FinalColors");

    // Jouer les couleurs dans l'ordre
    assert_eq(f.getNextColorName(), "Jaune", "Frame : prochaine couleur = Jaune");
    f.playShot(Ball("Jaune", 2));
    assert_eq(f.getNextColorName(), "Vert", "Frame : prochaine couleur = Vert");
    f.playShot(Ball("Vert", 3));
    f.playShot(Ball("Marron", 4));
    f.playShot(Ball("Bleu", 5));
    f.playShot(Ball("Rose", 6));
    assert_eq(f.getNextColorName(), "Noir", "Frame : prochaine couleur = Noir");
    f.playShot(Ball("Noir", 7));

    assert_true(f.isFinished(), "Frame : terminee apres toutes les couleurs");
    assert_true(f.getPhase() == FramePhase::Finished, "Frame : phase Finished");
}

// =======================================================================
// Tests Match
// =======================================================================
void testMatch()
{
    section("Match");

    Match m;
    assert_true(!m.isMatchFinished(), "Match : pas termine au depart");
    assert_eq(m.getFramesPlayer1(), 0, "Match : frames J1 = 0");
    assert_eq(m.getFramesPlayer2(), 0, "Match : frames J2 = 0");
}

// =======================================================================
// Tests Table
// =======================================================================
void testTable()
{
    section("Table");

    using namespace Snooker;
    assert_true(TableDimensions::LENGTH_MM == 3570.0f, "Table : longueur = 3570mm");
    assert_true(TableDimensions::WIDTH_MM  == 1770.0f, "Table : largeur = 1770mm");
    assert_true(BallSpots::CENTER_X == 885.0f,          "Table : centre = 885mm");
    assert_true(BallSpots::BLUE.y   == 1785.0f,         "Table : Bleu au centre (1785mm)");
    assert_true(BallSpots::BLACK.y  == 3246.0f,         "Table : Noir spot y = 3246mm");
}

// =======================================================================
// Tests Camera
// =======================================================================
void testCamera()
{
    section("Camera - Configuration");

    auto configs = Camera::TripleCameraSystem::getDefaultConfig();
    assert_eq(static_cast<int>(configs.size()), 3, "Camera : 3 cameras");

    assert_eq(configs[0].id, 1, "Camera 1 : id=1");
    assert_eq(configs[1].id, 2, "Camera 2 : id=2");
    assert_eq(configs[2].id, 3, "Camera 3 : id=3");

    // Toutes a 1200mm
    assert_true(configs[0].height_mm == 1200.0f, "Camera 1 : hauteur 1200mm");
    assert_true(configs[1].height_mm == 1200.0f, "Camera 2 : hauteur 1200mm");
    assert_true(configs[2].height_mm == 1200.0f, "Camera 3 : hauteur 1200mm");

    // Camera 2 au centre
    assert_true(configs[1].position_mm == Camera::TripleCameraSystem::TABLE_LENGTH_MM / 2.0f,
                "Camera 2 : au centre de la table");

    // Les zones couvrent toute la table avec overlap
    assert_true(configs[0].zone_start_mm == 0.0f,
                "Camera 1 : zone debut = 0");
    assert_true(configs[2].zone_end_mm   == Camera::TripleCameraSystem::TABLE_LENGTH_MM,
                "Camera 3 : zone fin = 3570mm");
}

void testCameraCalibration()
{
    section("Camera - Calibration");

    auto configs = Camera::TripleCameraSystem::getDefaultConfig();
    Camera::CameraCalibration cal(configs[0]);
    cal.calibrateDefault();

    assert_true(cal.isValid(), "Calibration : valide apres calibrateDefault");

    // Tester la conversion pixel -> mm
    float x_mm, y_mm;
    bool ok = cal.pixelToTable(0.0f, 720.0f, x_mm, y_mm);
    assert_true(ok, "Calibration : pixelToTable valide");
    // Pixel (0, 720) = coin bas-gauche = (0mm, zone_start_mm)
    assert_true(x_mm >= -5.0f && x_mm <= 5.0f,
                "Calibration : x=0px -> x_mm ~0");

    // Aller-retour
    float x_px, y_px;
    ok = cal.tableToPixel(885.0f, 595.0f, x_px, y_px);
    assert_true(ok, "Calibration : tableToPixel valide");
    float x2, y2;
    cal.pixelToTable(x_px, y_px, x2, y2);
    assert_true(std::abs(x2 - 885.0f) < 1.0f, "Calibration : aller-retour x");
    assert_true(std::abs(y2 - 595.0f) < 1.0f, "Calibration : aller-retour y");
}

void testCameraFusion()
{
    section("Camera - Fusion");

    auto configs = Camera::TripleCameraSystem::getDefaultConfig();
    Camera::CameraFusion fusion(configs);

    // Soumettre des detections de 2 cameras pour la meme bille
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    Camera::CameraFrame f1;
    f1.camera_id    = 1;
    f1.timestamp_ms = now_ms;
    {
        Camera::BallDetection det;
        det.color        = "Noir";
        det.x_mm         = 885.0f;
        det.y_mm         = 595.0f;
        det.confidence   = 0.9f;
        det.camera_id    = 1;
        det.timestamp_ms = now_ms;
        f1.detections.push_back(det);
    }

    Camera::CameraFrame f2;
    f2.camera_id    = 2;
    f2.timestamp_ms = now_ms;
    {
        Camera::BallDetection det;
        det.color        = "Noir";
        det.x_mm         = 890.0f;
        det.y_mm         = 598.0f;
        det.confidence   = 0.85f;
        det.camera_id    = 2;
        det.timestamp_ms = now_ms;
        f2.detections.push_back(det);
    }

    fusion.submitFrame(f1);
    fusion.submitFrame(f2);

    auto state = fusion.getFusedState();
    assert_true(state.valid, "Fusion : etat valide");
    assert_eq(static_cast<int>(state.balls.size()), 1, "Fusion : 1 bille Noir fusionnee");
    assert_true(state.balls[0].color == "Noir", "Fusion : bille = Noir");
    // Position fusionnee entre 885 et 890
    assert_true(state.balls[0].x_mm >= 885.0f && state.balls[0].x_mm <= 890.0f,
                "Fusion : position x fusionnee correcte");
    // Confiance augmentee grace a 2 cameras
    assert_true(state.balls[0].confidence > 0.9f, "Fusion : confiance augmentee par 2 cameras");
}

// =======================================================================
// Tests JSON
// =======================================================================
void testJson()
{
    section("API - JSON");

    GameManager manager;
    manager.startMatch();

    std::string json = Api::gameStateToJson(manager);
    assert_true(!json.empty(), "JSON : gameStateToJson non vide");
    assert_true(json.find("\"status\"") != std::string::npos, "JSON : contient status");
    assert_true(json.find("\"frame\"")  != std::string::npos, "JSON : contient frame");
    assert_true(json.find("\"players\"") != std::string::npos, "JSON : contient players");

    // Test parseShotRequest
    Ball b = Api::parseShotRequest("{\"ball_name\":\"Noir\",\"ball_value\":7}");
    assert_eq(b.getName(), "Noir", "JSON : parseShotRequest name=Noir");
    assert_eq(b.getValue(), 7,     "JSON : parseShotRequest value=7");

    // Test errorJson
    std::string err = Api::errorJson("test erreur");
    assert_true(err.find("error") != std::string::npos, "JSON : errorJson contient 'error'");
}

// =======================================================================
// Main
// =======================================================================
int main()
{
    std::cout << "=== Tests Snooker Player 3.0 ===" << std::endl;

    testBall();
    testBallSet();
    testPlayer();
    testReferee();
    testFrame();
    testFrameFoul();
    testFrameFinalColors();
    testMatch();
    testTable();
    testCamera();
    testCameraCalibration();
    testCameraFusion();
    testJson();

    return Test::summary();
}
