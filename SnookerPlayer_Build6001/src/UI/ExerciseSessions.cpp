#include "ExerciseSessions.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace
{
    QString sessionsPath()
    {
        return QCoreApplication::applicationDirPath() + "/exercices.json";
    }
}

namespace ExerciseSessions
{
    QVector<ExerciseSession> loadAll()
    {
        QVector<ExerciseSession> result;
        QFile file(sessionsPath());
        if (!file.open(QIODevice::ReadOnly))
        {
            return result;
        }
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (!doc.isArray())
        {
            return result;
        }
        for (const QJsonValue& value : doc.array())
        {
            QJsonObject o = value.toObject();
            ExerciseSession s;
            s.date = o.value("date").toString();
            s.exercise = o.value("exercice").toString();
            s.player = o.value("joueur").toString();
            s.points = o.value("points").toInt();
            s.reds = o.value("rouges").toInt();
            s.sequence = o.value("billes").toString();
            result.append(s);
        }
        return result;
    }

    void append(const ExerciseSession& session)
    {
        QJsonArray array;
        QFile in(sessionsPath());
        if (in.open(QIODevice::ReadOnly))
        {
            QJsonDocument doc = QJsonDocument::fromJson(in.readAll());
            in.close();
            if (doc.isArray())
            {
                array = doc.array();
            }
        }

        QJsonObject o;
        o["date"] = session.date;
        o["exercice"] = session.exercise;
        o["joueur"] = session.player;
        o["points"] = session.points;
        o["rouges"] = session.reds;
        o["billes"] = session.sequence;
        array.append(o);

        QFile out(sessionsPath());
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            out.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
            out.close();
        }
    }

    QVector<ExerciseSession> forPlayer(const QString& player, const QString& exercise)
    {
        QVector<ExerciseSession> result;
        for (const ExerciseSession& s : loadAll())
        {
            if (s.player == player && (exercise.isEmpty() || s.exercise == exercise))
            {
                result.append(s);
            }
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    ExerciseSummary summaryFor(const QString& player, const QString& exercise)
    {
        ExerciseSummary summary;
        int total = 0;
        for (const ExerciseSession& s : forPlayer(player, exercise))
        {
            ++summary.count;
            total += s.points;
            summary.best = std::max(summary.best, s.points);
        }
        if (summary.count > 0)
        {
            summary.average = static_cast<double>(total) / summary.count;
        }
        return summary;
    }
}
