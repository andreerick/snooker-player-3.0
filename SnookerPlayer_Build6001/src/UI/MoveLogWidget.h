#pragma once

#include <QWidget>
#include <QListWidget>

class ShotHistory;

// Panneau "Journal des coups" en pleine largeur, sous le tableau de score.
// Affiche l'historique chronologique (coups + fautes) de la ShotHistory
// du moteur de jeu reel. Thème sombre cohérent avec MainWindow.
class MoveLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MoveLogWidget(QWidget* parent = nullptr);

    // Reconstruit entierement la liste affichee a partir de l'historique reel.
    void refresh(const ShotHistory& history);

private:
    QListWidget* m_list = nullptr;
};