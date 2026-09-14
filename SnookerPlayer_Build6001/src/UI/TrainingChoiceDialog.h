#pragma once

#include <QDialog>

// =====================================================================
// TrainingChoiceDialog
// ---------------------------------------------------------------------
// Ouverte par la tuile "ENTRAINEMENT" de HomeScreen (voir
// HomeScreen::trainingRequested) : propose 2 choix independants --
// "CueSense" (outil externe d'analyse du geste, capteur requis) et
// "Exercice" (catalogue d'exercices natif, sans capteur) -- au lieu de
// lancer CueSense directement comme avant.
// =====================================================================
class TrainingChoiceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrainingChoiceDialog(QWidget* parent = nullptr);

signals:
    void cueSenseRequested();
    void exerciseRequested();
};
