#pragma once

#include "ExerciseSeries.h"

#include <QDialog>
#include <QString>

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;

// =====================================================================
// ExerciseScoreDialog
// ---------------------------------------------------------------------
// Compteur de serie pour les exercices a 15 rouges (Line-Up, The T) :
// l'utilisateur clique chaque bille empochee, la fenetre impose l'ordre
// rouge / couleur puis couleurs finales (voir ExerciseSeries), "Rate"
// termine la serie, et le score est enregistre dans l'historique du
// joueur choisi (voir ExerciseSessions).
// =====================================================================
class ExerciseScoreDialog : public QDialog
{
    Q_OBJECT

public:
    // redsOnly : exercice a 15 rouges sans couleur (boutons couleurs masques).
    ExerciseScoreDialog(const QString& exerciseName, bool redsOnly = false, QWidget* parent = nullptr);

private:
    void potBall(int value);
    void undoBall();
    void endSeries();
    void saveSeries();
    void newSeries();
    void refreshSeries();
    void refreshHistory();

    QString m_exerciseName;
    bool m_redsOnly = false;
    ExerciseSeries m_series;
    bool m_ended = false;
    bool m_saved = false;

    QComboBox* m_playerCombo = nullptr;
    QLabel* m_scoreLabel = nullptr;
    QLabel* m_nextLabel = nullptr;
    QLabel* m_sequenceLabel = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QListWidget* m_historyList = nullptr;
    QPushButton* m_ballButtons[7] = {};
    QPushButton* m_undoButton = nullptr;
    QPushButton* m_endButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_newButton = nullptr;
};
