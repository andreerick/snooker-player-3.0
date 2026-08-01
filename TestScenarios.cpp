// =============================================================
// TestScenarios.cpp
// Scénarios de test automatisés pour la logique de jeu snooker.
//
// Compilation (Linux/macOS) via le Makefile :
//   make test
//
// Ou directement :
//   g++ -std=c++17 -o test_scenarios Ball.cpp BallSet.cpp Player.cpp
//     Shot.cpp ShotHistory.cpp Referee.cpp Frame.cpp GameManager.cpp
//     Match.cpp Tournament.cpp TestScenarios.cpp
//
// Exécution :
//   ./test_scenarios
// =============================================================

#include <iostream>
#include <string>

#include "Frame.h"
#include "Ball.h"
#include "BallSet.h"
#include "GameManager.h"

// -------------------------------------------------------------
// Macros utilitaires PASS / FAIL
// -------------------------------------------------------------
static int g_total  = 0;
static int g_passed = 0;

static void check(bool condition,
                  const std::string& description)
{
    ++g_total;
    if (condition)
    {
        ++g_passed;
        std::cout << "  [PASS] " << description << std::endl;
    }
    else
    {
        std::cout << "  [FAIL] " << description << std::endl;
    }
}

// -------------------------------------------------------------
// Scénario 1 : Séquence de coup normale (rouge puis couleur)
// -------------------------------------------------------------
static void scenario1_coup_normal()
{
    std::cout << std::endl
              << "=== Scenario 1 : Sequence de coup normale ===" << std::endl;

    Frame frame;

    Ball rouge("Rouge", 1);
    Ball noire("Noire", 7);

    // Avant tout coup
    check(frame.redsRemaining() == 15,
          "Depart : 15 rouges restantes");
    check(!frame.isColorNeeded(),
          "Depart : aucune couleur requise");
    check(frame.currentPlayer().getScore() == 0,
          "Depart : score joueur 1 = 0");

    // Joue une rouge
    bool ok1 = frame.playShot(rouge);
    check(ok1,
          "Rouge jouee : coup valide");
    check(frame.redsRemaining() == 14,
          "Apres rouge : 14 rouges restantes");
    check(frame.isColorNeeded(),
          "Apres rouge : une couleur est requise");
    check(frame.currentPlayer().getScore() == 1,
          "Apres rouge : score joueur 1 = 1");

    // Joue la noire
    bool ok2 = frame.playShot(noire);
    check(ok2,
          "Noire jouee apres rouge : coup valide");
    check(!frame.isColorNeeded(),
          "Apres noire : aucune couleur requise");
    check(frame.currentPlayer().getScore() == 8,
          "Apres rouge+noire : score joueur 1 = 8");
    check(frame.getHistory().getShotCount() == 2,
          "Historique : 2 coups enregistres");
}

// -------------------------------------------------------------
// Scénario 2 : Faute (mauvaise bille jouée)
// -------------------------------------------------------------
static void scenario2_faute()
{
    std::cout << std::endl
              << "=== Scenario 2 : Faute ===" << std::endl;

    Frame frame;

    Ball rouge("Rouge", 1);
    Ball noire("Noire", 7);

    // Eric joue rouge + noire (break valide)
    frame.playShot(rouge);
    frame.playShot(noire);

    int scoreEricAvant = frame.currentPlayer().getScore(); // Eric = 8

    // Passage à Jean
    frame.switchPlayer();
    int scoreJeanAvant = frame.currentPlayer().getScore(); // Jean = 0

    // Jean joue la noire alors qu'une rouge est attendue
    bool okFaute = frame.playShot(noire);
    check(!okFaute,
          "Noire quand rouge attendue : coup invalide (faute)");

    // La pénalité de la noire est 7 (max(4, rouge=1, noire=7))
    // Les points vont à l'adversaire (Eric)
    // Le joueur courant redevient Eric après la faute
    int scoreEricApres  = frame.getPlayer1().getScore();
    int scoreJeanApres  = frame.getPlayer2().getScore();

    check(scoreEricApres == scoreEricAvant + 7,
          "Apres faute noire : Eric recoit 7 points de penalite");
    check(scoreJeanApres == scoreJeanAvant,
          "Apres faute : score de Jean inchange");
}

// -------------------------------------------------------------
// Scénario 3 : Changement de joueur / reprise après faute
// -------------------------------------------------------------
static void scenario3_reprise_apres_faute()
{
    std::cout << std::endl
              << "=== Scenario 3 : Reprise apres faute ===" << std::endl;

    Frame frame;

    Ball rouge("Rouge", 1);
    Ball noire("Noire", 7);
    Ball verte("Verte", 3);

    // Eric joue rouge + noire
    frame.playShot(rouge);
    frame.playShot(noire);

    // Passage à Jean
    frame.switchPlayer();
    check(frame.currentPlayer().getName() == "Jean",
          "Apres switchPlayer : joueur courant = Jean");

    // Jean commet une faute (noire quand rouge attendue)
    frame.playShot(noire);

    // Après faute : le tour revient à Eric
    check(frame.currentPlayer().getName() == "Eric",
          "Apres faute de Jean : joueur courant = Eric");

    // Eric peut rejouer normalement : rouge puis verte
    bool ok1 = frame.playShot(rouge);
    check(ok1,
          "Eric rejoue apres faute : rouge valide");
    bool ok2 = frame.playShot(verte);
    check(ok2,
          "Eric continue : verte valide apres rouge");
}

// -------------------------------------------------------------
// Scénario 4 : Free Ball
// -------------------------------------------------------------
static void scenario4_free_ball()
{
    std::cout << std::endl
              << "=== Scenario 4 : Free Ball ===" << std::endl;

    Frame frame;

    Ball bleue("Bleue", 5);
    Ball noire("Noire", 7);

    // Activation du free ball
    check(!frame.isFreeBall(),
          "Depart : free ball inactive");

    frame.setFreeBall(true);
    frame.setFreeBallColor(bleue);

    check(frame.isFreeBall(),
          "Apres activation : free ball active");
    check(frame.getFreeBallColor().getName() == "Bleue",
          "Couleur free ball choisie = Bleue");

    int scoreBefore = frame.currentPlayer().getScore();

    // Joue la bleue en free ball (compte pour 1 point = rouge)
    bool okFB = frame.playFreeBall(bleue);
    check(okFB,
          "Free ball jouee avec la bonne couleur : valide");
    check(frame.currentPlayer().getScore() == scoreBefore + 1,
          "Apres free ball : score +1 (comme une rouge)");
    check(!frame.isFreeBall(),
          "Apres free ball : free ball desactivee");
    check(frame.isColorNeeded(),
          "Apres free ball : une couleur est requise (m_needColor = true)");

    // Joue ensuite une couleur (noire) – doit être légal
    bool okNoire = frame.playShot(noire);
    check(okNoire,
          "Apres free ball : noire jouee comme couleur - valide");
    check(!frame.isColorNeeded(),
          "Apres noire suite free ball : aucune couleur requise");
}

// -------------------------------------------------------------
// Scénario 5 : Gestion du BallSet
// -------------------------------------------------------------
static void scenario5_ball_set()
{
    std::cout << std::endl
              << "=== Scenario 5 : Gestion du BallSet ===" << std::endl;

    BallSet table;

    // État initial
    check(table.countBalls("Rouge") == 15,
          "Depart : 15 rouges presentes");
    check(table.isOnTable("Bleue"),
          "Depart : Bleue presente");
    check(table.isOnTable("Noire"),
          "Depart : Noire presente");
    check(table.isOnTable("Blanche"),
          "Depart : Blanche presente");

    // Retrait d'une bille
    table.removeBall("Bleue");
    check(!table.isOnTable("Bleue"),
          "Apres removeBall(Bleue) : Bleue absente");

    // Restauration
    table.restoreBall(Ball("Bleue", 5));
    check(table.isOnTable("Bleue"),
          "Apres restoreBall(Bleue) : Bleue presente");

    // Retrait d'une rouge
    int rougesAvant = table.countBalls("Rouge");
    table.removeBall("Rouge");
    check(table.countBalls("Rouge") == rougesAvant - 1,
          "Apres removeBall(Rouge) : une rouge en moins");

    // Retrait d'une bille inexistante : ne doit pas planter
    int rougesApres = table.countBalls("Rouge");
    table.removeBall("BilleFantome");
    check(table.countBalls("Rouge") == rougesApres,
          "removeBall bille inexistante : comptage inchange");
}

// -------------------------------------------------------------
// Scénario 6 : Re-spot des couleurs en phase des rouges
// -------------------------------------------------------------
static void scenario6_respot_couleur_phase_rouges()
{
    std::cout << std::endl
              << "=== Scenario 6 : Re-spot couleur en phase rouges ===" << std::endl;

    Frame frame;

    Ball rouge("Rouge", 1);
    Ball noire("Noire", 7);

    // Joue rouge puis noire
    frame.playShot(rouge);
    check(frame.getBallSet().isOnTable("Noire"),
          "Avant coup noire : Noire presente sur la table");

    frame.playShot(noire);

    // En phase des rouges, la noire doit être re-spottée
    check(frame.getBallSet().isOnTable("Noire"),
          "Apres coup noire (phase rouges) : Noire re-spottee sur la table");
    check(frame.getPhase() == FramePhase::Reds,
          "Phase restee Reds apres rouge+noire");
}

// -------------------------------------------------------------
// Scénario 7 : Calcul de la pénalité de faute
// -------------------------------------------------------------
static void scenario7_calcul_penalite()
{
    std::cout << std::endl
              << "=== Scenario 7 : Calcul de penalite de faute ===" << std::endl;

    // Rouge attendue, noire jouee : max(4, rouge=1, noire=7) = 7
    {
        Frame frame;
        Ball noire("Noire", 7);
        // Il suffit de jouer la noire sans rouge avant pour déclencher une faute
        frame.playShot(noire);
        int scoreJ2 = frame.getPlayer2().getScore();
        check(scoreJ2 == 7,
              "Faute noire quand rouge attendue : penalite = 7");
    }

    // Rouge attendue, verte jouee : max(4, rouge=1, verte=3) = 4
    {
        Frame frame;
        Ball verte("Verte", 3);
        frame.playShot(verte);
        int scoreJ2 = frame.getPlayer2().getScore();
        check(scoreJ2 == 4,
              "Faute verte quand rouge attendue : penalite = 4 (minimum)");
    }

    // Rouge attendue, jaune jouee : max(4, rouge=1, jaune=2) = 4
    {
        Frame frame;
        Ball jaune("Jaune", 2);
        frame.playShot(jaune);
        int scoreJ2 = frame.getPlayer2().getScore();
        check(scoreJ2 == 4,
              "Faute jaune quand rouge attendue : penalite = 4 (minimum)");
    }
}

// -------------------------------------------------------------
// Point d'entrée
// -------------------------------------------------------------
int main()
{
    std::cout << "============================================" << std::endl;
    std::cout << "  TESTS SCENARIOS - SNOOKER PLAYER 3.0"     << std::endl;
    std::cout << "============================================" << std::endl;

    scenario1_coup_normal();
    scenario2_faute();
    scenario3_reprise_apres_faute();
    scenario4_free_ball();
    scenario5_ball_set();
    scenario6_respot_couleur_phase_rouges();
    scenario7_calcul_penalite();

    std::cout << std::endl
              << "============================================" << std::endl;
    std::cout << "  RESULTATS : "
              << g_passed << " / " << g_total << " tests passes" << std::endl;
    std::cout << "============================================" << std::endl;

    return (g_passed == g_total) ? 0 : 1;
}
