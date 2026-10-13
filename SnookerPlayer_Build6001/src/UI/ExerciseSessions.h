#pragma once

#include <QString>
#include <QVector>

// =====================================================================
// ExerciseSessions
// ---------------------------------------------------------------------
// Historique des series d'entrainement comptees (Line-Up, The T) :
// fichier exercices.json a cote de l'executable (meme principe que
// players.json / matchs.json). Un enregistrement par serie enregistree.
// =====================================================================
struct ExerciseSession
{
    QString date;       // ISO, heure locale
    QString exercise;   // nom sans numero, ex. "Line-Up"
    QString player;
    int points = 0;
    int reds = 0;
    QString sequence;   // valeurs des billes empochees, ex. "1,7,1,5"
};

struct ExerciseSummary
{
    int count = 0;
    int best = 0;
    double average = 0.0;
};

namespace ExerciseSessions
{
    QVector<ExerciseSession> loadAll();
    void append(const ExerciseSession& session);

    // Les plus recentes d'abord ; `exercise` vide = tous les exercices.
    QVector<ExerciseSession> forPlayer(const QString& player, const QString& exercise = QString());
    ExerciseSummary summaryFor(const QString& player, const QString& exercise);
}
