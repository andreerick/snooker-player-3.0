#include "TournamentManager.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <algorithm>

namespace
{
    QString competitionFilePath(TournamentManager::Kind kind)
    {
        QString fileName = (kind == TournamentManager::Kind::Championship) ? "/championnat.json" : "/tournoi.json";
        return QCoreApplication::applicationDirPath() + fileName;
    }
}

void TournamentManager::create(const QString& name, Format format, const QStringList& players, int framesToWin)
{
    clear();
    m_name = name;
    m_format = format;
    m_framesToWin = framesToWin;

    if (format == Format::Elimination)
    {
        generateElimination(players);
    }
    else if (format == Format::League)
    {
        generateLeague(players);
    }
    else
    {
        generateRoundRobin(players);
    }
}

void TournamentManager::clear()
{
    m_name.clear();
    m_matchups.clear();
}

void TournamentManager::generateElimination(const QStringList& players)
{
    int paddedCount = 1;
    while (paddedCount < players.size())
    {
        paddedCount *= 2;
    }

    QStringList padded = players;
    while (padded.size() < paddedCount)
    {
        padded.append(QString()); // chaine vide = bye (adversaire absent)
    }

    int totalRounds = 0;
    for (int n = paddedCount; n > 1; n /= 2)
    {
        ++totalRounds;
    }

    // Round 1 : les vrais joueurs (+ byes eventuels), resolus tout de
    // suite (un bye ne se "joue" pas, le joueur passe directement au
    // round suivant).
    for (int i = 0; i < paddedCount / 2; ++i)
    {
        Matchup m;
        m.round = 1;
        m.player1 = padded[2 * i];
        m.player2 = padded[2 * i + 1];
        if (m.player1.isEmpty() || m.player2.isEmpty())
        {
            m.isBye = true;
            m.winner = m.player1.isEmpty() ? m.player2 : m.player1;
        }
        m_matchups.append(m);
    }

    // Rounds suivants : places vides, remplies au fur et a mesure par
    // advanceEliminationIfRoundComplete() une fois le round precedent
    // entierement joue.
    for (int round = 2; round <= totalRounds; ++round)
    {
        int matchCount = paddedCount / (1 << round);
        for (int i = 0; i < matchCount; ++i)
        {
            Matchup m;
            m.round = round;
            m_matchups.append(m);
        }
    }

    // Propage tout de suite les byes du round 1 (un round entierement
    // fait de byes, ex. un seul "vrai" round 1 mais un bye dedans, doit
    // immediatement remplir le round 2 pour que nextMatch() trouve un
    // match REEL des le debut plutot que de sembler bloque).
    advanceEliminationIfRoundComplete(1);
}

void TournamentManager::generateRoundRobin(const QStringList& players)
{
    for (int i = 0; i < players.size(); ++i)
    {
        for (int j = i + 1; j < players.size(); ++j)
        {
            Matchup m;
            m.round = 1;
            m.player1 = players[i];
            m.player2 = players[j];
            m_matchups.append(m);
        }
    }
}

void TournamentManager::generateLeague(const QStringList& players)
{
    // Phase aller puis phase retour : meme paires, roles inverses.
    for (int leg = 1; leg <= 2; ++leg)
    {
        for (int i = 0; i < players.size(); ++i)
        {
            for (int j = i + 1; j < players.size(); ++j)
            {
                Matchup m;
                m.round = leg;
                m.leg = leg;
                m.player1 = (leg == 1) ? players[i] : players[j];
                m.player2 = (leg == 1) ? players[j] : players[i];
                m_matchups.append(m);
            }
        }
    }
}

void TournamentManager::advanceEliminationIfRoundComplete(int completedRound)
{
    QVector<Matchup*> roundMatches;
    for (Matchup& m : m_matchups)
    {
        if (m.round == completedRound)
        {
            roundMatches.append(&m);
        }
    }
    if (roundMatches.isEmpty())
    {
        return;
    }
    for (Matchup* m : roundMatches)
    {
        if (!m->isPlayed())
        {
            return; // round pas encore entierement joue
        }
    }

    QVector<Matchup*> nextRoundMatches;
    for (Matchup& m : m_matchups)
    {
        if (m.round == completedRound + 1)
        {
            nextRoundMatches.append(&m);
        }
    }
    if (nextRoundMatches.isEmpty())
    {
        return; // completedRound etait la finale, rien a propager
    }

    for (int i = 0; i < nextRoundMatches.size(); ++i)
    {
        if (2 * i + 1 >= roundMatches.size())
        {
            break;
        }
        nextRoundMatches[i]->player1 = roundMatches[2 * i]->winner;
        nextRoundMatches[i]->player2 = roundMatches[2 * i + 1]->winner;
    }
}

const TournamentManager::Matchup* TournamentManager::nextMatch() const
{
    for (const Matchup& m : m_matchups)
    {
        if (!m.isPlayed() && !m.isBye && !m.player1.isEmpty() && !m.player2.isEmpty())
        {
            return &m;
        }
    }
    return nullptr;
}

QVector<const TournamentManager::Matchup*> TournamentManager::pendingMatches() const
{
    QVector<const Matchup*> result;
    for (const Matchup& m : m_matchups)
    {
        if (!m.isPlayed() && !m.isBye && !m.player1.isEmpty() && !m.player2.isEmpty())
        {
            result.append(&m);
        }
    }
    return result;
}

void TournamentManager::recordResult(const QString& player1, const QString& player2, const QString& winner, int scoreWinner, int scoreLoser)
{
    // Deux passes : d'abord l'ordre exact (player1, player2), puis
    // l'ordre inverse. Indispensable en Championnat ou la meme paire
    // apparait deux fois (aller A-B, retour B-A) : le match joue doit
    // etre rattache a la bonne manche.
    for (int pass = 0; pass < 2; ++pass)
    for (Matchup& m : m_matchups)
    {
        if (m.isPlayed() || m.isBye)
        {
            continue;
        }
        bool matches = (pass == 0)
            ? (m.player1 == player1 && m.player2 == player2)
            : (m.player1 == player2 && m.player2 == player1);
        if (!matches)
        {
            continue;
        }
        m.winner = winner;
        if (m.player1 == winner)
        {
            m.scorePlayer1 = scoreWinner;
            m.scorePlayer2 = scoreLoser;
        }
        else
        {
            m.scorePlayer1 = scoreLoser;
            m.scorePlayer2 = scoreWinner;
        }

        if (m_format == Format::Elimination)
        {
            advanceEliminationIfRoundComplete(m.round);
        }
        return;
    }
}

bool TournamentManager::isFinished() const
{
    if (m_matchups.isEmpty())
    {
        return false;
    }
    if (isRoundRobinLike())
    {
        return std::all_of(m_matchups.begin(), m_matchups.end(), [](const Matchup& m) { return m.isPlayed(); });
    }

    int maxRound = 0;
    for (const Matchup& m : m_matchups)
    {
        maxRound = std::max(maxRound, m.round);
    }
    for (const Matchup& m : m_matchups)
    {
        if (m.round == maxRound)
        {
            return m.isPlayed();
        }
    }
    return false;
}

QStringList TournamentManager::champions() const
{
    if (!isFinished())
    {
        return QStringList();
    }
    if (isRoundRobinLike())
    {
        QStringList result;
        for (const Standing& st : standings())
        {
            if (st.rank != 1)
            {
                break;
            }
            result.append(st.name);
        }
        return result;
    }

    int maxRound = 0;
    for (const Matchup& m : m_matchups)
    {
        maxRound = std::max(maxRound, m.round);
    }
    for (const Matchup& m : m_matchups)
    {
        if (m.round == maxRound)
        {
            return QStringList{ m.winner };
        }
    }
    return QStringList();
}

QString TournamentManager::champion() const
{
    return champions().join(" et ");
}

QVector<TournamentManager::Standing> TournamentManager::standings() const
{
    QVector<Standing> table;
    // Renvoie un INDICE, pas une reference : deux appels de suite (un
    // par joueur du match) peuvent chacun faire grossir `table`, et un
    // QVector::append() qui reagit vecteur invalide toute reference
    // obtenue par le premier appel -- l'incrementation faite ensuite via
    // cette reference perimee ecrirait dans une memoire deja liberee
    // (constate en test : le tout premier joueur ajoute se retrouvait
    // avec played=0 alors qu'il avait bien joue). Reindexer `table[i]`
    // a chaque usage reste valable meme apres un append().
    auto findOrCreateIndex = [&table](const QString& name) -> int
    {
        for (int i = 0; i < table.size(); ++i)
        {
            if (table[i].name == name)
            {
                return i;
            }
        }
        table.append(Standing{ name, 0, 0, 0, 0, 0 });
        return table.size() - 1;
    };

    for (const Matchup& m : m_matchups)
    {
        if (!m.isPlayed())
        {
            continue;
        }
        int i1 = findOrCreateIndex(m.player1);
        int i2 = findOrCreateIndex(m.player2);
        table[i1].played++;
        table[i2].played++;
        table[i1].framesFor += m.scorePlayer1;
        table[i1].framesAgainst += m.scorePlayer2;
        table[i2].framesFor += m.scorePlayer2;
        table[i2].framesAgainst += m.scorePlayer1;
        if (m.winner == m.player1)
        {
            table[i1].won++;
            table[i1].points += kPointsPerWin;
        }
        else
        {
            table[i2].won++;
            table[i2].points += kPointsPerWin;
        }
    }

    auto frameDiff = [](const Standing& st) { return st.framesFor - st.framesAgainst; };
    std::sort(table.begin(), table.end(), [&](const Standing& a, const Standing& b)
        {
            if (a.points != b.points)
            {
                return a.points > b.points;
            }
            return frameDiff(a) > frameDiff(b);
        });

    // Egalite de points ET de difference de frames : on regarde les
    // matchs directs entre les joueurs concernes seulement (mini-poule).
    // Une cle par joueur (et non une comparaison deux a deux) pour
    // rester un ordre total valable meme a 3 joueurs a egalite.
    struct Key
    {
        int wins = 0;
        int diff = 0;
        bool operator==(const Key& other) const { return wins == other.wins && diff == other.diff; }
    };
    QMap<QString, Key> keys;
    for (int start = 0; start < table.size();)
    {
        int end = start + 1;
        while (end < table.size() && table[end].points == table[start].points
               && frameDiff(table[end]) == frameDiff(table[start]))
        {
            ++end;
        }
        if (end - start > 1)
        {
            QStringList group;
            for (int i = start; i < end; ++i)
            {
                group.append(table[i].name);
            }
            for (const Matchup& m : m_matchups)
            {
                if (!m.isPlayed() || !group.contains(m.player1) || !group.contains(m.player2))
                {
                    continue;
                }
                keys[m.winner].wins++;
                keys[m.player1].diff += m.scorePlayer1 - m.scorePlayer2;
                keys[m.player2].diff += m.scorePlayer2 - m.scorePlayer1;
            }
            std::stable_sort(table.begin() + start, table.begin() + end, [&](const Standing& a, const Standing& b)
                {
                    const Key ka = keys.value(a.name);
                    const Key kb = keys.value(b.name);
                    if (ka.wins != kb.wins)
                    {
                        return ka.wins > kb.wins;
                    }
                    return ka.diff > kb.diff;
                });
        }
        start = end;
    }

    // Meme rang uniquement si TOUT est identique, matchs directs compris.
    for (int i = 0; i < table.size(); ++i)
    {
        bool sameAsPrevious = i > 0
            && table[i].points == table[i - 1].points
            && frameDiff(table[i]) == frameDiff(table[i - 1])
            && keys.value(table[i].name) == keys.value(table[i - 1].name);
        table[i].rank = sameAsPrevious ? table[i - 1].rank : i + 1;
    }
    return table;
}

void TournamentManager::save() const
{
    QJsonObject root;
    root["name"] = m_name;
    root["format"] = (m_format == Format::Elimination) ? "elimination"
                   : (m_format == Format::League) ? "league" : "roundrobin";
    root["framesToWin"] = m_framesToWin;

    QJsonArray matchArray;
    for (const Matchup& m : m_matchups)
    {
        QJsonObject obj;
        obj["round"] = m.round;
        obj["player1"] = m.player1;
        obj["player2"] = m.player2;
        obj["winner"] = m.winner;
        obj["scorePlayer1"] = m.scorePlayer1;
        obj["scorePlayer2"] = m.scorePlayer2;
        obj["isBye"] = m.isBye;
        obj["leg"] = m.leg;
        matchArray.append(obj);
    }
    root["matchups"] = matchArray;

    QFile file(competitionFilePath(m_kind));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

TournamentManager TournamentManager::load(Kind kind)
{
    TournamentManager tm(kind);

    QFile file(competitionFilePath(kind));
    if (!file.open(QIODevice::ReadOnly))
    {
        return tm;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
    {
        return tm;
    }

    QJsonObject root = doc.object();
    tm.m_name = root.value("name").toString();
    QString formatName = root.value("format").toString();
    tm.m_format = (formatName == "roundrobin") ? Format::RoundRobin
                : (formatName == "league") ? Format::League : Format::Elimination;
    tm.m_framesToWin = root.value("framesToWin").toInt(2);

    for (const QJsonValue& value : root.value("matchups").toArray())
    {
        QJsonObject obj = value.toObject();
        Matchup m;
        m.round = obj.value("round").toInt();
        m.player1 = obj.value("player1").toString();
        m.player2 = obj.value("player2").toString();
        m.winner = obj.value("winner").toString();
        m.scorePlayer1 = obj.value("scorePlayer1").toInt();
        m.scorePlayer2 = obj.value("scorePlayer2").toInt();
        m.isBye = obj.value("isBye").toBool();
        m.leg = obj.value("leg").toInt();
        tm.m_matchups.append(m);
    }
    return tm;
}
