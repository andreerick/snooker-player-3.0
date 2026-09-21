#include "ScenarioReplay.h"

#include "Ball.h"
#include "Frame.h"
#include "Referee.h"

#include <cctype>
#include <cstdlib>

namespace
{
    std::string trim(const std::string& s)
    {
        size_t begin = 0;
        size_t end = s.size();
        while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) { ++begin; }
        while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) { --end; }
        return s.substr(begin, end - begin);
    }

    bool contains(const std::string& s, const std::string& part)
    {
        return s.find(part) != std::string::npos;
    }

    bool startsWith(const std::string& s, const std::string& prefix)
    {
        return s.compare(0, prefix.size(), prefix) == 0;
    }

    // Texte compris entre la fin de `marker` et `stop` (ou la fin de la ligne).
    std::string afterMarker(const std::string& s, const std::string& marker, char stop, bool& found)
    {
        size_t idx = s.find(marker);
        found = (idx != std::string::npos);
        if (!found)
        {
            return std::string();
        }
        std::string remainder = s.substr(idx + marker.size());
        size_t end = remainder.find(stop);
        return trim(end != std::string::npos ? remainder.substr(0, end) : remainder);
    }
}

int standardBallValueOf(const std::string& ballName)
{
    if (ballName == "Rouge") return 1;
    if (ballName == "Jaune") return 2;
    if (ballName == "Verte") return 3;
    if (ballName == "Marron") return 4;
    if (ballName == "Bleue") return 5;
    if (ballName == "Rose") return 6;
    if (ballName == "Noire") return 7;
    return 0;
}

std::vector<ReplayAction> parseScenarioText(const std::string& text)
{
    std::vector<ReplayAction> actions;

    size_t pos = 0;
    while (pos <= text.size())
    {
        size_t eol = text.find('\n', pos);
        std::string line = text.substr(pos, (eol == std::string::npos) ? std::string::npos : eol - pos);
        pos = (eol == std::string::npos) ? text.size() + 1 : eol + 1;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        // Ne garde que les lignes numerotees du journal ("N. ...").
        size_t dotIdx = line.find(". ");
        if (dotIdx == std::string::npos || dotIdx == 0)
        {
            continue;
        }
        bool allDigits = true;
        for (size_t i = 0; i < dotIdx; ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(line[i]))) { allDigits = false; break; }
        }
        if (!allDigits)
        {
            continue;
        }
        std::string rest = line.substr(dotIdx + 2);

        // Trace d'une correction d'arbitre : n'est pas un evenement de jeu
        // (le coup corrige, deja present sur sa propre ligne, est rejoue
        // normalement) ; son texte peut contenir " -- adverse +N" qu'il ne
        // faut surtout pas lire comme un coup.
        if (startsWith(rest, "CORRECTION ARBITRE"))
        {
            continue;
        }

        if (contains(rest, " -- MISS"))
        {
            actions.push_back({ ReplayAction::Kind::Miss, "", "" });
            continue;
        }
        if (contains(rest, " -- BILLE TOUCHANTE"))
        {
            actions.push_back({ ReplayAction::Kind::TouchingBall, "", "" });
            continue;
        }
        if (contains(rest, " -- REMETTRE EN PLACE"))
        {
            actions.push_back({ ReplayAction::Kind::Replay, "", "" });
            continue;
        }

        if (startsWith(rest, "FAUTE -- "))
        {
            bool foundPlayed = false;
            std::string ballName = afterMarker(rest, "bille jouee : ", ')', foundPlayed);
            if (!foundPlayed)
            {
                continue;
            }

            bool foundRequired = false;
            std::string requiredName = afterMarker(rest, "bille demandee : ", ',', foundRequired);

            bool isFreeBallFoul = contains(rest, ": Faute pendant un Free Ball (");

            if (isFreeBallFoul && !requiredName.empty())
            {
                actions.push_back({ ReplayAction::Kind::FreeBallFoul, requiredName, ballName });
            }
            else if (ballName == "Blanche")
            {
                actions.push_back({ ReplayAction::Kind::BlancheOffTable, "", "" });
            }
            else if (!requiredName.empty() && requiredName == ballName)
            {
                actions.push_back({ ReplayAction::Kind::DirectFoul, ballName, "" });
            }
            else
            {
                actions.push_back({ ReplayAction::Kind::Shot, ballName, "" });
            }
            continue;
        }

        // Ligne normale : "PlayerName -- BallName (+X)".
        size_t sep = rest.find(" -- ");
        if (sep == std::string::npos)
        {
            continue;
        }
        std::string afterSep = rest.substr(sep + 4);
        size_t parenIdx = afterSep.find(" (+");
        std::string ballName = trim(parenIdx != std::string::npos ? afterSep.substr(0, parenIdx) : afterSep);

        // Valeur reellement comptee : differente de la valeur standard = Free Ball.
        int loggedPoints = -1;
        if (parenIdx != std::string::npos)
        {
            size_t closeParen = afterSep.find(')', parenIdx);
            std::string pointsStr = afterSep.substr(
                parenIdx + 3,
                (closeParen != std::string::npos ? closeParen : afterSep.size()) - (parenIdx + 3));
            pointsStr = trim(pointsStr);
            if (!pointsStr.empty() && pointsStr.find_first_not_of("0123456789") == std::string::npos)
            {
                loggedPoints = std::atoi(pointsStr.c_str());
            }
        }

        if (loggedPoints >= 0 && loggedPoints != standardBallValueOf(ballName))
        {
            actions.push_back({ ReplayAction::Kind::FreeBallShot, ballName, "" });
        }
        else
        {
            actions.push_back({ ReplayAction::Kind::Shot, ballName, "" });
        }
    }

    return actions;
}

void applyReplayAction(Frame& frame, const ReplayAction& action)
{
    switch (action.kind)
    {
    case ReplayAction::Kind::Miss:
        frame.missShot();
        break;

    case ReplayAction::Kind::TouchingBall:
        frame.setTouchingBall(true);
        break;

    case ReplayAction::Kind::Replay:
        frame.requestReplay();
        break;

    case ReplayAction::Kind::BlancheOffTable:
    {
        Ball required = frame.getRequiredBall();
        Ball blanche("Blanche", 0);
        Referee referee;
        frame.foul(required, blanche, referee.calculateFoul(required, blanche), "Blanche sortie de la table");
        break;
    }

    case ReplayAction::Kind::DirectFoul:
    {
        Ball required = frame.getRequiredBall();
        Ball touched(action.ballName, standardBallValueOf(action.ballName));
        Referee referee;
        frame.foul(required, touched, referee.calculateFoul(required, touched), Frame::kMissFoulReason);
        break;
    }

    case ReplayAction::Kind::FreeBallShot:
    {
        // Arme le Free Ball juste avant de le jouer, comme le bouton "Choisir
        // la bille de depart" en direct. Ne gere pas le cas ambigu
        // ("n'importe quelle couleur" due, voir Frame::isFreeBallValueAmbiguous()).
        Ball designated(action.ballName, standardBallValueOf(action.ballName));
        frame.setFreeBall(true);
        frame.setFreeBallColor(designated);
        frame.playFreeBall(designated);
        break;
    }

    case ReplayAction::Kind::FreeBallFoul:
    {
        // Arme le Free Ball avec la bille DESIGNEE, puis "joue" la bille
        // reellement touchee : playFreeBall() reconnait le mismatch et
        // declenche la faute (meme penalite, meme desarmement propre).
        Ball designated(action.ballName, standardBallValueOf(action.ballName));
        Ball touched(action.touchedBallName, standardBallValueOf(action.touchedBallName));
        frame.setFreeBall(true);
        frame.setFreeBallColor(designated);
        frame.playFreeBall(touched);
        break;
    }

    case ReplayAction::Kind::Shot:
    default:
        frame.playShot(Ball(action.ballName, standardBallValueOf(action.ballName)));
        break;
    }
}
