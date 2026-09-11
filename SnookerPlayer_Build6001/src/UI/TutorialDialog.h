#pragma once

#include <QDialog>

class QListWidget;
class QLabel;

// =====================================================================
// TutorialDialog
// ---------------------------------------------------------------------
// Ecran "Tutoriels" (barre du bas de HomeScreen) : guide de prise en
// main de L'APPLI elle-meme (comment demarrer un match, connecter un
// telephone, creer un tournoi...) -- distinct du "Reglement" (tuile
// Aide), qui couvre les regles officielles du snooker, pas
// l'utilisation du logiciel. Contenu statique (pas de dependance a
// l'etat reel de l'appli), simple liste de sujets a gauche + texte
// explicatif a droite (meme structure que PlayersDialog).
// =====================================================================
class TutorialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TutorialDialog(QWidget* parent = nullptr);

private:
    void showTopic(int index);

    QListWidget* m_topicList = nullptr;
    QLabel* m_contentLabel = nullptr;
};
