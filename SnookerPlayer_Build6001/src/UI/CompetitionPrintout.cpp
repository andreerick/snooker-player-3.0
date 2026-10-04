#include "CompetitionPrintout.h"

#include <QDateTime>
#include <QMap>
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

    QString formatDescriptionForBlank(TournamentManager::Format format)
    {
        switch (format)
        {
        case TournamentManager::Format::Elimination:
            return "Elimination directe";
        case TournamentManager::Format::RoundRobin:
            return "Round robin (tous contre tous)";
        case TournamentManager::Format::League:
            break;
        }
        return "Championnat aller-retour";
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

namespace
{
    QString blankLine(const QString& label, int widthPercent)
    {
        return "<td width='" + QString::number(widthPercent) + "%'>" + label + " ______________________</td>";
    }

    QString blankMatchesTable(const TournamentManager& structure, int round, int& matchNumber, QMap<int, int>& firstNumberOfRound)
    {
        firstNumberOfRound[round] = matchNumber + 1;
        QString html = "<table " + kTableAttributes + ">";
        html += "<tr " + kHeaderBackground + ">"
              + cell("<b>N&deg;</b>", "width='8%' align='center'")
              + cell("<b>Joueur 1</b>", "width='26%'")
              + cell("<b>Joueur 2</b>", "width='26%'")
              + cell("<b>Score</b>", "width='15%' align='center'")
              + cell("<b>Vainqueur</b>", "width='25%'")
              + "</tr>";
        int indexInRound = 0;
        for (const TournamentManager::Matchup& m : structure.matchups())
        {
            if (m.round != round)
            {
                continue;
            }
            ++matchNumber;
            auto side = [&](const QString& player, int feeder) -> QString
            {
                if (!player.isEmpty())
                {
                    return esc(player);
                }
                if (round > 1 && firstNumberOfRound.contains(round - 1))
                {
                    return "<i>Vainq. match " + QString::number(firstNumberOfRound[round - 1] + 2 * indexInRound + feeder) + "</i>";
                }
                return "&nbsp;";
            };
            const bool bye = m.isBye;
            html += "<tr height='27'>"
                  + cell(QString::number(matchNumber), "align='center'")
                  + cell(side(m.player1, 0))
                  + cell(bye ? "<i>exempt</i>" : side(m.player2, 1))
                  + cell(bye ? "&mdash;" : "&nbsp;", "align='center'")
                  + cell(bye ? "<b>" + esc(m.winner) + "</b>" : "&nbsp;")
                  + "</tr>";
            ++indexInRound;
        }
        html += "</table>";
        return html;
    }
}

QString blankCompetitionPrintoutHtml(TournamentManager::Format format, int playerCount)
{
    playerCount = std::max(2, playerCount);
    QStringList names;
    for (int i = 1; i <= playerCount; ++i)
    {
        names.append("J" + QString::number(i));
    }
    TournamentManager structure;
    structure.create(QString(), format, names);

    const bool isLeague = (format == TournamentManager::Format::League);
    const bool withPoints = isLeague;
    const QString kind = isLeague ? "Championnat" : "Tournoi";

    QString html = "<html><body style=\"font-family: Helvetica, Arial, sans-serif; font-size: 11pt; color: #000000;\">";
    html += "<h1 align='center' style='font-size: 22pt; margin-bottom: 2px;'>" + kind + "</h1>";
    html += "<p align='center' style='font-size: 11pt;'>" + esc(formatDescriptionForBlank(format))
          + " &nbsp;&bull;&nbsp; " + QString::number(playerCount) + " joueurs</p>";
    html += "<table width='100%' cellspacing='0' cellpadding='6'><tr>"
          + blankLine("Nom :", 60) + blankLine("Date :", 40) + "</tr><tr>"
          + blankLine("Frames par match :", 60) + "<td>&nbsp;</td></tr></table>";

    // Joueurs : J1..Jn a nommer au stylo (deux colonnes si beaucoup de joueurs).
    html += "<h3 style='margin-top: 8px; margin-bottom: 3px;'>Joueurs</h3>";
    html += "<table " + kTableAttributes + ">";
    html += "<tr " + kHeaderBackground + ">" + cell("<b>N&deg;</b>", "width='10%' align='center'") + cell("<b>Nom du joueur</b>") + "</tr>";
    for (int i = 1; i <= playerCount; ++i)
    {
        html += "<tr height='25'>" + cell("J" + QString::number(i), "align='center'") + cell("&nbsp;") + "</tr>";
    }
    html += "</table>";

    int maxRound = 0;
    for (const TournamentManager::Matchup& m : structure.matchups())
    {
        maxRound = std::max(maxRound, m.round);
    }
    int matchNumber = 0;
    QMap<int, int> firstNumberOfRound;
    for (int round = 1; round <= std::max(maxRound, 1); ++round)
    {
        html += "<h3 style='margin-top: 10px; margin-bottom: 3px;'>"
              + esc(roundTitle(structure, round, maxRound)) + "</h3>";
        html += blankMatchesTable(structure, round, matchNumber, firstNumberOfRound);
    }

    if (structure.isRoundRobinLike())
    {
        html += "<h3 style='margin-top: 20px; margin-bottom: 4px;'>Classement</h3>";
        html += "<table " + kTableAttributes + ">";
        html += "<tr " + kHeaderBackground + ">"
              + cell("<b>Rang</b>", "align='center'")
              + cell("<b>Joueur</b>", "width='34%'")
              + (withPoints ? cell("<b>Pts</b>", "align='center'") : QString())
              + cell("<b>V</b>", "align='center'")
              + cell("<b>J</b>", "align='center'")
              + cell("<b>Frames</b>", "align='center'")
              + cell("<b>Diff.</b>", "align='center'")
              + "</tr>";
        for (int rank = 1; rank <= playerCount; ++rank)
        {
            html += "<tr height='25'>" + cell(QString::number(rank), "align='center'") + cell("&nbsp;")
                  + (withPoints ? cell("&nbsp;") : QString()) + cell("&nbsp;") + cell("&nbsp;") + cell("&nbsp;") + cell("&nbsp;") + "</tr>";
        }
        html += "</table>";
        if (withPoints)
        {
            html += "<p style='font-size: 9pt; color: #333333;'>Points : 2 par victoire, 0 par defaite. "
                    "Departage : difference de frames, puis matchs directs.</p>";
        }
    }
    else
    {
        html += "<h3 align='center' style='margin-top: 8px; margin-bottom: 2px;'>Champion : ______________________</h3>";
    }

    html += "</body></html>";
    return html;
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
