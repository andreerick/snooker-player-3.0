// Tests automatiques de la logique Championnat / Tournoi (TournamentManager) :
// sans interface ni camera. Lance par ctest ; code non nul si un controle echoue.
#include "TournamentManager.h"

#include <iostream>
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

    using TM = TournamentManager;

    // Joue le match (p1 vs p2, ordre exact) avec le vainqueur et le score donnes.
    void play(TM& tm, const QString& p1, const QString& p2, const QString& winner, int scoreWinner, int scoreLoser)
    {
        tm.recordResult(p1, p2, winner, scoreWinner, scoreLoser);
    }
}

int main()
{
    // --- Generation aller-retour ---
    {
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "A", "B", "C" }, 3);
        check(tm.matchups().size() == 6, "3 joueurs => 6 matchs (aller + retour)");
        check(tm.framesToWin() == 3, "framesToWin conserve");
        int legs1 = 0, legs2 = 0;
        for (const TM::Matchup& m : tm.matchups())
        {
            (m.leg == 1 ? legs1 : legs2)++;
        }
        check(legs1 == 3 && legs2 == 3, "3 matchs aller + 3 matchs retour");
        // Chaque match retour est l'inverse d'un match aller.
        bool reversedOk = true;
        for (const TM::Matchup& back : tm.matchups())
        {
            if (back.leg != 2) continue;
            bool found = false;
            for (const TM::Matchup& go : tm.matchups())
            {
                if (go.leg == 1 && go.player1 == back.player2 && go.player2 == back.player1) found = true;
            }
            reversedOk = reversedOk && found;
        }
        check(reversedOk, "chaque retour inverse les roles de l'aller");
        check(tm.pendingMatches().size() == 6, "6 matchs en attente au depart");
        check(!tm.isFinished(), "pas termine au depart");
        check(tm.champions().isEmpty(), "pas de champion au depart");
    }

    // --- Rattachement du resultat a la bonne manche (aller vs retour) ---
    {
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "A", "B" }, 2);
        play(tm, "B", "A", "B", 2, 0); // joue d'abord le RETOUR
        check(tm.matchups()[0].winner.isEmpty(), "l'aller (A-B) reste en attente");
        check(tm.matchups()[1].winner == "B", "le retour (B-A) est enregistre");
        play(tm, "A", "B", "A", 2, 1);
        check(tm.matchups()[0].winner == "A", "puis l'aller est enregistre");
        check(tm.isFinished(), "termine apres les 2 matchs");
    }

    // --- Points : 2 par victoire, 0 par defaite ---
    {
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "A", "B", "C" }, 2);
        play(tm, "A", "B", "A", 2, 0);
        play(tm, "A", "C", "A", 2, 1);
        play(tm, "B", "C", "C", 2, 0);
        auto table = tm.standings();
        check(table.size() == 3, "3 lignes de classement");
        check(table[0].name == "A" && table[0].points == 4, "A : 4 points");
        check(table[1].name == "C" && table[1].points == 2, "C : 2 points");
        check(table[2].name == "B" && table[2].points == 0, "B : 0 point");
        check(table[0].rank == 1 && table[1].rank == 2 && table[2].rank == 3, "rangs 1,2,3");
    }

    // --- Depart a la difference de frames (meme nombre de points) ---
    {
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "A", "B" }, 3);
        play(tm, "A", "B", "A", 3, 0); // A : +3
        play(tm, "B", "A", "B", 3, 2); // B gagne : A -1 => A total +2, B total -2
        auto table = tm.standings();
        check(table[0].points == 2 && table[1].points == 2, "A et B : 2 points chacun");
        check(table[0].name == "A" && table[1].name == "B", "A devant grace a la difference de frames");
        check(table[0].rank == 1 && table[1].rank == 2, "rangs 1 et 2");
        check(tm.champions() == QStringList{ "A" }, "A seul champion");
    }

    // --- Egalite points + frames, depart par le match direct (frames) ---
    {
        // A et B : 1 victoire chacun et meme difference de frames au total (-4),
        // mais A fait mieux dans leurs matchs directs (+2 contre -2). C bat tout
        // le monde. B est inscrit avant A : sans departage direct, l'ordre
        // d'inscription mettrait B devant A. Attendu : C, A, B.
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "B", "A", "C" }, 3);
        play(tm, "C", "B", "C", 3, 2);
        play(tm, "B", "C", "C", 3, 2);
        play(tm, "A", "B", "A", 3, 0);
        play(tm, "B", "A", "B", 3, 2);
        play(tm, "C", "A", "C", 3, 0);
        play(tm, "A", "C", "C", 3, 0);
        auto table = tm.standings();
        check(table[0].name == "C", "C premier (8 points)");
        check(table[1].name == "A" && table[2].name == "B", "A devant B grace au match direct");
        check(table[1].points == table[2].points, "A et B a egalite de points");
        check((table[1].framesFor - table[1].framesAgainst) == (table[2].framesFor - table[2].framesAgainst),
              "A et B a egalite de difference de frames");
        check(table[1].rank == 2 && table[2].rank == 3, "rangs 2 et 3 (departages)");
    }

    // --- Egalite parfaite a 2 : meme rang, deux co-champions ---
    {
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "A", "B" }, 1);
        play(tm, "A", "B", "A", 1, 0);
        play(tm, "B", "A", "B", 1, 0);
        auto table = tm.standings();
        check(table[0].rank == 1 && table[1].rank == 1, "egalite parfaite : rang 1 pour les deux");
        check(tm.champions().size() == 2, "deux co-champions");
        check(tm.champion().contains(" et "), "champion() joint les deux noms");
    }

    // --- Egalite parfaite a 3 (pierre-feuille-ciseaux) : meme rang pour les 3 ---
    {
        TM tm(TM::Kind::Championship);
        tm.create("Test", TM::Format::League, { "A", "B", "C" }, 2);
        play(tm, "A", "B", "A", 2, 0);
        play(tm, "B", "C", "B", 2, 0);
        play(tm, "A", "C", "C", 2, 0);
        play(tm, "B", "A", "B", 2, 0);
        play(tm, "C", "B", "C", 2, 0);
        play(tm, "C", "A", "A", 2, 0);
        auto table = tm.standings();
        check(table[0].points == 4 && table[1].points == 4 && table[2].points == 4, "3 joueurs a 4 points");
        check(table[0].rank == 1 && table[1].rank == 1 && table[2].rank == 1, "meme rang pour les 3");
        check(tm.champions().size() == 3, "3 co-champions");
    }

    // --- Tournoi (elimination) inchange : champion unique ---
    {
        TM tm;
        tm.create("Tournoi", TM::Format::Elimination, { "A", "B", "C", "D" });
        play(tm, "A", "B", "A", 2, 0);
        play(tm, "C", "D", "D", 2, 1);
        play(tm, "A", "D", "D", 2, 0);
        check(tm.isFinished(), "elimination terminee apres la finale");
        check(tm.champions() == QStringList{ "D" }, "D champion unique");
    }

    std::cout << g_checks << " controles, " << g_failures << " echec(s)" << std::endl;
    return g_failures == 0 ? 0 : 1;
}
