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

void MatchStorage::saveMatch(Match& match)
{
    QJsonArray history = loadHistory();

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
    matchObj["date"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    matchObj["joueur1"] = QString::fromStdString(match.getPlayer1().getName());
    matchObj["joueur2"] = QString::fromStdString(match.getPlayer2().getName());
    matchObj["frames_joueur1"] = match.getFramesPlayer1();
    matchObj["frames_joueur2"] = match.getFramesPlayer2();
    matchObj["frames"] = frames;

    history.append(matchObj);

    QFile outFile(filePath());
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        outFile.write(QJsonDocument(history).toJson(QJsonDocument::Indented));
        outFile.close();
    }
}