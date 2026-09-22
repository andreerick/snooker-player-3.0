// Tests automatiques du moteur de regles (sans interface, sans camera).
// Lance par ctest ; renvoie un code non nul si un controle echoue.
#include "Frame.h"
#include "Match.h"
#include "Referee.h"
#include "SnookerGeometry.h"
#include "ScenarioReplay.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    int g_checks = 0;
    int g_failures = 0;

    void check(bool ok, const std::string& label)
    {
        ++g_checks;
        if (!ok)
        {
            ++g_failures;
            std::cout << "ECHEC : " << label << std::endl;
        }
    }

    template <typename A, typename B>
    void checkEq(const A& actual, const B& expected, const std::string& label)
    {
        ++g_checks;
        if (!(actual == expected))
        {
            ++g_failures;
            std::cout << "ECHEC : " << label << " (obtenu " << actual << ", attendu " << expected << ")" << std::endl;
        }
    }

    Ball red() { return Ball("Rouge", 1); }
    Ball yellow() { return Ball("Jaune", 2); }
    Ball green() { return Ball("Verte", 3); }
    Ball brown() { return Ball("Marron", 4); }
    Ball blue() { return Ball("Bleue", 5); }
    Ball pink() { return Ball("Rose", 6); }
    Ball black() { return Ball("Noire", 7); }

    void missFoul(Frame& frame)
    {
        Ball required = frame.getRequiredBall();
        Ball touched = red();
        Referee referee;
        frame.foul(required, touched, referee.calculateFoul(required, touched), Frame::kMissFoulReason);
    }

    void testMaximumBreak()
    {
        Frame f;
        f.setPlayerNames("A", "B");
        for (int i = 0; i < 15; ++i)
        {
            check(f.playShot(red()), "rouge " + std::to_string(i + 1) + " legale");
            check(f.playShot(black()), "noire apres la rouge " + std::to_string(i + 1) + " legale");
        }
        checkEq(f.getPlayer1().getScore(), 120, "15 x (rouge + noire) = 120");
        for (const Ball& c : { yellow(), green(), brown(), blue() })
        {
            check(f.playShot(c), "couleur finale " + c.getName() + " legale");
        }
        checkEq(f.getPlayer1().getScore(), 134, "120 + 2 + 3 + 4 + 5");
        check(!f.isFinished(), "frame encore en cours avant la rose");
        check(f.playShot(pink()), "rose legale");
        checkEq(f.getPlayer1().getScore(), 140, "134 + 6");
        // Regle voulue (decision de l'utilisateur) : tant que la noire est a
        // jouer, la frame reste ouverte, meme avec un ecart insurmontable.
        check(!f.isFinished(), "frame encore ouverte : la noire n'est pas jouee");
        check(f.playShot(black()), "derniere noire legale");
        checkEq(f.getPlayer1().getScore(), 147, "la noire empochee compte : 147");
        check(f.isFinished(), "frame terminee apres la derniere noire");
        checkEq(f.getWinnerName(), std::string("A"), "vainqueur du 147");
    }

    Frame buildFrameUpToPink()
    {
        Frame f;
        f.setPlayerNames("A", "B");
        for (int i = 0; i < 15; ++i)
        {
            f.playShot(red());
            f.playShot(black());
        }
        for (const Ball& c : { yellow(), green(), brown(), blue(), pink() })
        {
            f.playShot(c);
        }
        return f;
    }

    void testLastBlackMissed()
    {
        // Noire jouee mais non empochee (Fin de break) : ecart de 140 -> 140.
        Frame f = buildFrameUpToPink();
        check(!f.isFinished(), "frame ouverte avant le coup sur la noire");
        f.missShot();
        check(f.isFinished(), "noire ratee avec un ecart insurmontable : frame terminee");
        checkEq(f.getPlayer1().getScore(), 140, "le score reste 140");
        checkEq(f.getWinnerName(), std::string("A"), "vainqueur apres la noire ratee");

        // Faute sur la noire : le fautif donne 7 points, l'ecart reste insurmontable.
        Frame g = buildFrameUpToPink();
        g.foul(black(), red(), 7, "test");
        check(g.isFinished(), "faute sur la noire avec ecart insurmontable : frame terminee");
        checkEq(g.getPlayer2().getScore(), 7, "penalite de 7 pour l'adversaire");
    }

    void testCloseScoreOnLastBlack()
    {
        // Ecart de 7 ou moins : la frame continue apres une noire ratee.
        Frame f;
        f.setPlayerNames("A", "B");
        f.foul(red(), red(), 135, "test");   // A fautif : B a 135, la main passe a B
        f.missShot();                        // B ne marque pas, la main revient a A
        for (int i = 0; i < 15; ++i)
        {
            f.playShot(red());
            f.playShot(black());
        }
        for (const Ball& c : { yellow(), green(), brown(), blue(), pink() })
        {
            f.playShot(c);
        }
        checkEq(f.getPlayer1().getScore(), 140, "A a 140");
        checkEq(f.getPlayer2().getScore(), 135, "B a 135");
        f.missShot();
        check(!f.isFinished(), "ecart de 5 : la frame continue apres la noire ratee");
        check(f.playShot(black()), "B empoche la noire");
        checkEq(f.getPlayer2().getScore(), 142, "135 + 7");
        check(f.isFinished(), "frame terminee apres la noire");
        checkEq(f.getWinnerName(), std::string("B"), "B gagne 142-140");
    }

    void testFouls()
    {
        Referee referee;
        checkEq(referee.calculateFoul(red(), red()), 4, "penalite minimale 4");
        checkEq(referee.calculateFoul(red(), black()), 7, "rouge demandee, noire touchee = 7");
        checkEq(referee.calculateFoul(blue(), red()), 5, "bleue demandee = 5");
        checkEq(referee.calculateFoul(black(), yellow()), 7, "noire demandee = 7");
        checkEq(referee.calculateFoul(yellow(), red()), 4, "jaune demandee, rouge touchee = 4 (minimum)");

        Frame f;
        f.setPlayerNames("A", "B");
        check(!f.playShot(yellow()), "jouer une couleur d'entree est une faute");
        checkEq(f.getPlayer2().getScore(), 4, "faute d'entree : +4 a l'adversaire");
        checkEq(f.getPlayer1().getScore(), 0, "le fautif ne marque rien");
        checkEq(f.currentPlayer().getName(), std::string("B"), "la main passe apres une faute");
    }

    void testSequenceAndMiss()
    {
        Frame f;
        f.setPlayerNames("A", "B");
        check(f.playShot(red()), "rouge");
        check(f.playShot(blue()), "bleue apres une rouge");
        checkEq(f.getPlayer1().getScore(), 6, "1 + 5");
        checkEq(f.getRequiredBall().getName(), std::string("Rouge"), "une rouge est due apres une couleur");

        check(f.playShot(red()), "2e rouge");
        f.missShot();
        checkEq(f.currentPlayer().getName(), std::string("B"), "Fin de break : la main passe");
        checkEq(f.getPlayer1().getScore(), 7, "aucun point ajoute par Fin de break");
        checkEq(f.getRequiredBall().getName(), std::string("Rouge"), "l'adversaire reprend par une rouge (regle du projet)");

        Frame g;
        g.setPlayerNames("A", "B");
        check(g.playShot(red()), "rouge");
        check(g.playShot(blue()), "bleue");
        check(!g.playShot(black()), "deux couleurs de suite = faute");
        checkEq(g.getPlayer2().getScore(), 7, "faute couleur au lieu de rouge : +7");
    }

    void testTouchingBall()
    {
        Frame f;
        f.setPlayerNames("A", "B");
        f.playShot(red());
        f.playShot(black());
        f.setTouchingBall(true, "Noire");
        check(f.playShot(black()), "avec bille touchante, le coup est accepte");
        checkEq(f.getPlayer1().getScore(), 15, "1 + 7 + 7");
        checkEq(f.getPlayer2().getScore(), 0, "aucune faute avec bille touchante");

        Frame g;
        g.setPlayerNames("A", "B");
        g.playShot(red());
        g.playShot(black());
        check(!g.playShot(black()), "sans bille touchante, la meme bille est une faute");
        checkEq(g.getPlayer2().getScore(), 7, "faute de 7 sans bille touchante");
    }

    void testConcedeFrame()
    {
        Frame f;
        f.setPlayerNames("A", "B");
        f.playShot(red());
        f.playShot(black());
        checkEq(f.getPlayer1().getScore(), 8, "A mene 8-0");
        check(f.concedeFrame(f.getPlayer1()), "A concede la frame");
        check(f.isFinished(), "frame terminee apres concession");
        checkEq(f.getWinnerName(), std::string("B"), "l'adversaire gagne meme s'il est mene");
    }

    void testMissWarningChain()
    {
        Frame f;
        f.setPlayerNames("A", "B");
        std::string player, offender;
        checkEq(f.missReplayChain(player), 0, "aucune chaine au depart");

        missFoul(f);
        f.requestReplay();
        checkEq(f.missReplayChain(player), 1, "1ere remise en place");
        check(!f.isMissFrameForfeitDue(offender), "pas encore de perte de frame (1)");

        missFoul(f);
        f.requestReplay();
        checkEq(f.missReplayChain(player), 2, "2e remise en place");
        checkEq(player, std::string("A"), "le joueur a avertir est le fautif");

        missFoul(f);
        check(f.isMissFrameForfeitDue(offender), "3e Faute et Miss : frame attribuable");
        checkEq(offender, std::string("A"), "fautif de la 3e");

        Frame g;
        g.setPlayerNames("A", "B");
        missFoul(g);
        g.requestReplay();
        missFoul(g);
        g.requestReplay();
        g.playShot(red());
        checkEq(g.missReplayChain(player), 0, "un coup reussi rompt la chaine");
    }

    void testMatch()
    {
        Match m;
        m.setPlayerNames("A", "B");
        m.setFramesToWin(2);
        m.start();
        checkEq(m.getCurrentFrame().currentPlayer().getName(), std::string("A"), "A ouvre la frame 1");

        m.getCurrentFrame().concedeFrame(m.getCurrentFrame().getPlayer1());
        m.checkFrameEnd();
        checkEq(m.getFramesPlayer2(), 1, "B gagne la frame 1 par concession");
        check(!m.isMatchFinished(), "match pas fini a 1-0 sur 2 frames gagnantes");
        m.proceedToNextFrame();
        checkEq(m.getCurrentFrame().currentPlayer().getName(), std::string("B"), "B ouvre la frame 2 (alternance)");

        m.forceMatchEnd(m.getPlayer1());
        check(m.isMatchFinished(), "Conceder le match termine le match");
        checkEq(m.getFramesPlayer1(), 2, "le vainqueur passe au nombre de frames gagnantes");
    }

    // Rejoue un scenario_*.txt (les memes fichiers/logique que le bouton
    // "Rejouer le scenario" de la telecommande) sur un match neuf, puis
    // compare les scores finaux de la frame en cours.
    void replayScenario(const std::string& fileName, int expectedEric, int expectedDavid)
    {
        std::ifstream in(std::string(SCENARIO_DIR) + "/" + fileName, std::ios::binary);
        std::stringstream buffer;
        buffer << in.rdbuf();
        check(in.good() || in.eof(), "scenario lisible : " + fileName);

        std::vector<ReplayAction> actions = parseScenarioText(buffer.str());
        check(!actions.empty(), "scenario non vide : " + fileName);

        Match match;
        match.setPlayerNames("Eric", "David");
        match.setFramesToWin(5);
        match.start();
        for (const ReplayAction& action : actions)
        {
            applyReplayAction(match.getCurrentFrame(), action);
            match.checkFrameEnd();
        }

        Frame& frame = match.getCurrentFrame();
        if (match.isFrameJustFinished())
        {
            // La frame est terminee : Match la garde jusqu'a proceedToNextFrame().
            checkEq(match.getFrameResults().back().scorePlayer1, expectedEric, fileName + " : score Eric");
            checkEq(match.getFrameResults().back().scorePlayer2, expectedDavid, fileName + " : score David");
        }
        else
        {
            checkEq(frame.getPlayer1().getScore(), expectedEric, fileName + " : score Eric");
            checkEq(frame.getPlayer2().getScore(), expectedDavid, fileName + " : score David");
        }
    }

    void testScenarioReplay()
    {
        // Scores verifies a la main puis confirmes par rejeu reel (2026-09-15).
        replayScenario("s15_faute_pendant_free_ball.txt", 23, 4);
        replayScenario("s17_fautes_couleurs_finales.txt", 55, 71);
        replayScenario("s33_rate_complet_couleur_ambigue.txt", 1, 5);
        replayScenario("s60_noire_respotee_mort_subite.txt", 63, 56);
        // Score raisonne a la main (Rouge +1, Noire +7, puis Noire +7 de
        // nouveau grace a la Bille Touchante qui dispense de l'alternance
        // rouge/couleur pour ce seul coup) puis confirme par rejeu reel.
        replayScenario("s70_bille_touchante_bypass_couleur.txt", 15, 0);
        // Score raisonne a la main (Rouge +1 puis faute "Blanche sortie de
        // la table" : penalite = max(4, valeur bille demandee, valeur
        // bille touchee) = max(4, 0, 0) = 4 pour l'adversaire) puis
        // confirme par rejeu reel.
        replayScenario("s71_blanche_sortie_de_table.txt", 1, 4);

        // Une trace "CORRECTION ARBITRE" n'est pas un evenement de jeu, meme si
        // son texte contient " -- adverse +N".
        std::vector<ReplayAction> corrected = parseScenarioText(
            "4. CORRECTION ARBITRE : Rouge (+1) -> FAUTE (bille jouee : Rose) -- adverse +6\n");
        check(corrected.empty(), "une ligne CORRECTION ARBITRE est ignoree au rejeu");
    }

    void testSnookerGeometry()
    {
        PositionedBall cue{ "Blanche", 100, 89 };
        PositionedBall r{ "Rouge", 150, 89 };
        auto state = [&](const std::vector<PositionedBall>& balls, const std::string& required,
                         SnookerState expected, const std::string& label)
        {
            check(assessSnookerOnTable(cue, balls, required).state == expected, label);
        };
        state({ r }, "Rouge", SnookerState::NotSnookered, "geometrie : aucun obstacle");
        state({ r, { "Noire", 125, 89 } }, "Rouge", SnookerState::Snookered, "geometrie : noire au milieu");
        state({ r, { "Noire", 125, 93 } }, "Rouge", SnookerState::Snookered, "geometrie : un seul bord gene = snooke");
        state({ r, { "Noire", 125, 100 } }, "Rouge", SnookerState::NotSnookered, "geometrie : obstacle loin de l'axe");
        state({ r, { "Rouge", 150, 60 }, { "Noire", 125, 89 } }, "Rouge", SnookerState::NotSnookered, "geometrie : une rouge libre");
        state({ r, { "Noire", 125, 97.1 } }, "Rouge", SnookerState::Borderline, "geometrie : cas limite");
        state({ r, { "Noire", 175, 89 } }, "Rouge", SnookerState::NotSnookered, "geometrie : obstacle derriere la cible");
        state({ { "Jaune", 150, 89 }, { "Rouge", 125, 89 } }, "Couleur", SnookerState::Snookered, "geometrie : seule couleur bloquee");
        state({ { "Rouge", 125, 89 }, { "Rouge", 150, 89 } }, "Rouge", SnookerState::NotSnookered, "geometrie : une bille jouable devant une autre");
    }
}

int main()
{
    testMaximumBreak();
    testLastBlackMissed();
    testCloseScoreOnLastBlack();
    testFouls();
    testSequenceAndMiss();
    testTouchingBall();
    testConcedeFrame();
    testMissWarningChain();
    testMatch();
    testScenarioReplay();
    testSnookerGeometry();

    std::cout << g_checks << " controles, " << g_failures << " echec(s)" << std::endl;
    return g_failures == 0 ? 0 : 1;
}
