#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;

// =====================================================================
// PlayersDialog
// ---------------------------------------------------------------------
// Ecran "Joueurs" (tuile de HomeScreen) : liste des joueurs connus,
// photo optionnelle par joueur, statistiques et historique des matchs
// joues (calcules a la volee depuis matchs.json, voir MatchStorage --
// aucune statistique n'est stockee separement, pour eviter tout risque
// de desynchronisation avec l'historique reel).
//
// La liste des noms reutilise le meme fichier que la boite de dialogue
// de saisie des noms (joueurs.ini, cle "joueurs/noms", voir
// MainWindow::loadSavedPlayers()/savePlayerList()) : ajouter/supprimer
// un joueur ici se repercute donc aussi dans le menu deroulant au
// demarrage d'un match, et inversement.
// =====================================================================
class PlayersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlayersDialog(QWidget* parent = nullptr);

private:
    void refreshPlayerList();
    void showPlayer(const QString& name);
    void addPlayer();
    void removeSelectedPlayer();
    void clearAllPlayers();
    void changePhoto();

    QListWidget* m_playerList = nullptr;
    QPushButton* m_photoButton = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_statsLabel = nullptr;
    QListWidget* m_historyList = nullptr;

    QString m_currentPlayer;
};
