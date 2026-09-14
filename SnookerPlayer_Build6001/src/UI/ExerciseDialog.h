#pragma once

#include <QDialog>

class QListWidget;
class QLabel;

// =====================================================================
// ExerciseDialog
// ---------------------------------------------------------------------
// Catalogue d'exercices d'entrainement NATIF (pas de capteur requis,
// contrairement a CueSense) -- ouvert depuis TrainingChoiceDialog. Meme
// structure que TutorialDialog (liste de sujets a gauche, contenu a
// droite), plus un diagramme de mise en place dessine en vectoriel pour
// l'exercice courant.
// =====================================================================
class ExerciseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExerciseDialog(QWidget* parent = nullptr);

private:
    void showExercise(int index);

    QListWidget* m_exerciseList = nullptr;
    QLabel* m_diagramLabel = nullptr;
    QLabel* m_contentLabel = nullptr;
};
