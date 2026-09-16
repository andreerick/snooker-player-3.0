#include "MatchStorage.h"

#include "Match.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QString MatchStorage::filePath()
{
    return QCoreApplication::applicationDirPath() + "/matchs.json";
}

QJsonArray MatchStorage::loadHistory()
{
    QFile existingFile(filePath());
    if (!existingFile.open(QIODevice::ReadOnly))
    {
        return QJsonArray();
    }

    QJsonDocument existingDoc = QJsonDocument::fromJson(existingFile.readAll());
    existingFile.close();

    return existingDoc.isArray() ? existingDoc.array() : QJsonArray();
}

void MatchStorage::saveMatch(Match& match, int& historyIndex)
{
    QJsonArray history = loadHistory();

    // Date FIGEE au premier enregistrement (debut du match) : sans ca,
    // chaque mise a jour intermediaire (voir historyIndex) avancerait la
    // date affichee jusqu'a celle de la derniere frame jouee au lieu du
    // debut du match.
    QString date;
    bool hasExistingEntry = historyIndex >= 0 && historyIndex < history.size();
    if (hasExistingEntry)
    {
        date = history[historyIndex].toObject()["date"].toString();
    }
    if (date.isEmpty())
    {
        date = QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    QJsonArray frames;
    for (const FrameResult& frame : match.getFrameResults())
    {
        QJsonObject frameObj;
        frameObj["vainqueur"] = QString::fromStdString(frame.winnerName);
        frameObj["score_joueur1"] = frame.scorePlayer1;
        frameObj["score_joueur2"] = frame.scorePlayer2;
        frames.append(frameObj);
    }

    QJsonObject matchObj;
    matchObj["date"] = date;
    matchObj["joueur1"] = QString::fromStdString(match.getPlayer1().getName());
    matchObj["joueur2"] = QString::fromStdString(match.getPlayer2().getName());
    matchObj["frames_joueur1"] = match.getFramesPlayer1();
    matchObj["frames_joueur2"] = match.getFramesPlayer2();
    matchObj["frames"] = frames;
    // Permet de distinguer plus tard un match interrompu (plantage,
    // fermeture) d'un match reellement termine, sans rien casser pour le
    // code existant qui lit ce fichier (nouveau champ, ignore si absent).
    matchObj["enCours"] = !match.isMatchFinished();

    if (hasExistingEntry)
    {
        history.replace(historyIndex, matchObj);
    }
    else
    {
        historyIndex = history.size();
        history.append(matchObj);
    }

    QFile outFile(filePath());
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        outFile.write(QJsonDocument(history).toJson(QJsonDocument::Indented));
        outFile.close();
    }
}