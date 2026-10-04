#include "CompetitionPrintout.h"

#include <QDateTime>
#include <QStringList>
#include <algorithm>

namespace
{
    QString esc(const QString& text)
    {
        return text.toHtmlEscaped();
    }

    QString cell(const QString& content, const QString& attributes = QString())
    {
        return "<td " + attributes + ">" + content + "</td>";
    }

    const QString kTableAttributes = "border='1' cellspacing='0' cellpadding='6' width='100%'";
    const QString kHeaderBackground = "bgcolor='#e6e6e6'";

    QString roundTitle(const TournamentManager& competition, int round, int maxRound)
    {
        if (competition.format() == TournamentManager::Format::League)
        {
            return round == 1 ? "Matchs aller" : "Matchs retour";
        }
        if (competition.format() == TournamentManager::Format::RoundRobin)
        {
            return "Matchs";
        }
        if (round == maxRound)
        {
            return "Finale";
        }
        if (round == maxRound - 1)
        {
            return "Demi-finales";
        }
        if (round == maxRound - 2)
        {
            return "Quarts de finale";
        }
        return "Tour " + QString::number(round);
    }

    QString formatDescription(const TournamentManager& competition)
    {
        switch (competition.format())
        {
        case TournamentManager::Format::Elimination:
            return "Elimination directe";
        case TournamentManager::Format::RoundRobin:
            return "Round robin (tous contre tous)";
        case TournamentManager::Format::League:
            break;
        }
        QString text = "Championnat aller-retour";
        if (competition.framesToWin() == 1)
        {
            text += " - 1 frame par match";
        }
        else
        {
            text += " - meilleur des " + QString::number(2 * competition.framesToWin() - 1) + " frames";
        }
        return text;
    }

    QString matchesTable(const TournamentManager& competition, int round, int& matchNumber)
    {
        QString html = "<table " + kTableAttributes + ">";
        html += "<tr " + kHeaderBackground + ">"
              + cell("<b>N&deg;</b>", "width='6%' align='center'")
              + cell("<b>Joueur 1</b>", "width='27%'")
              + cell("<b>Joueur 2</b>", "width='27%'")
              + cell("<b>Score</b>", "width='15%' align='center'")
              + cell("<b>Vainqueur</b>", "width='25%'")
              + "</tr>";
        for (const TournamentManager::Matchup& m : competition.matchups())
        {
            if (m.round != round)
            {
                continue;
            }
            ++matchNumber;
            const QString p1 = m.player1.isEmpty() ? "&nbsp;" : esc(m.player1);
            const QString p2 = m.player2.isEmpty() ? "&nbsp;" : esc(m.player2);
            QString score;
            QString winner;
            if (m.isBye)
            {
                score = "exempt";
                winner = "<b>" + esc(m.winner) + "</b>";
            }
            else if (m.isPlayed())
            {
                score = QString::number(m.scorePlayer1) + " - " + QString::number(m.scorePlayer2);
                winner = "<b>" + esc(m.winner) + "</b>";
            }
            else
            {
                score = "__ - __"; // a remplir a la main
                winner = "&nbsp;";
            }
            html += "<tr>"
                  + cell(QString::number(matchNumber), "align='center'")
                  + cell(p1) + cell(p2)
                  + cell(score, "align='center'")
                  + cell(winner)
                  + "</tr>";
        }
        html += "</table>";
        return html;
    }

    QString standingsTable(const TournamentManager& competition)
    {
        const bool withPoints = (competition.format() == TournamentManager::Format::League);
        QString html = "<table " + kTableAttributes + ">";
        html += "<tr " + kHeaderBackground + ">"
              + cell("<b>Rang</b>", "align='center'")
              + cell("<b>Joueur</b>", "width='34%'")
              + (withPoints ? cell("<b>Pts</b>", "align='center'") : QString())
              + cell("<b>V</b>", "align='center'")
              + cell("<b>J</b>", "align='center'")
              + cell("<b>Frames</b>", "align='center'")
              + cell("<b>Diff.</b>", "align='center'")
              + "</tr>";
        for (const TournamentManager::Standing& s : competition.standings())
        {
            const int diff = s.framesFor - s.framesAgainst;
            html += "<tr>"
                  + cell(QString::number(s.rank), "align='center'")
                  + cell(esc(s.name))
                  + (withPoints ? cell("<b>" + QString::number(s.points) + "</b>", "align='center'") : QString())
                  + cell(QString::number(s.won), "align='center'")
                  + cell(QString::number(s.played), "align='center'")
                  + cell(QString::number(s.framesFor) + " - " + QString::number(s.framesAgainst), "align='center'")
                  + cell((diff > 0 ? "+" : "") + QString::number(diff), "align='center'")
                  + "</tr>";
        }
        html += "</table>";
        return html;
    }
}

QString competitionPrintoutHtml(const TournamentManager& competition)
{
    const bool isChampionship = (competition.kind() == TournamentManager::Kind::Championship);

    QString html = "<html><body style=\"font-family: Helvetica, Arial, sans-serif; font-size: 11pt; color: #000000;\">";
    html += "<h1 align='center' style='font-size: 22pt; margin-bottom: 2px;'>"
          + esc(competition.name().isEmpty() ? (isChampionship ? "Championnat" : "Tournoi") : competition.name())
          + "</h1>";
    html += "<p align='center' style='font-size: 11pt; margin-top: 0px;'>"
          + esc(formatDescription(competition))
          + " &nbsp;&bull;&nbsp; " + QString::number(competition.matchups().size()) + " matchs</p>";

    int maxRound = 0;
    for (const TournamentManager::Matchup& m : competition.matchups())
    {
        maxRound = std::max(maxRound, m.round);
    }
    int matchNumber = 0;
    const int firstRound = (competition.format() == TournamentManager::Format::RoundRobin) ? 1 : 1;
    for (int round = firstRound; round <= std::max(maxRound, 1); ++round)
    {
        html += "<h3 style='margin-top: 16px; margin-bottom: 4px;'>"
              + esc(roundTitle(competition, round, maxRound)) + "</h3>";
        html += matchesTable(competition, round, matchNumber);
    }

    if (competition.isRoundRobinLike())
    {
        html += "<h3 style='margin-top: 20px; margin-bottom: 4px;'>Classement</h3>";
        html += standingsTable(competition);
    }

    if (competition.isFinished())
    {
        const QStringList winners = competition.champions();
        html += "<h2 align='center' style='margin-top: 22px;'>"
              + QString(winners.size() > 1 ? "Egalite parfaite en tete : " : "Champion : ")
              + esc(winners.join(" et ")) + "</h2>";
    }

    html += "<p align='center' style='font-size: 8pt; color: #555555; margin-top: 26px;'>Snooker Player &mdash; imprime le "
          + QDateTime::currentDateTime().toString("dd/MM/yyyy 'a' HH:mm") + "</p>";
    html += "</body></html>";
    return html;
}
