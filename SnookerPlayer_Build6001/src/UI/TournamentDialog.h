#pragma once

#include <QDialog>
#include "TournamentManager.h"

class QListWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class QStackedWidget;
class QPushButton;

// =====================================================================
// TournamentDialog
// ---------------------------------------------------------------------
// Ecran "Tournoi" (tuile de HomeScreen) : creation d'un tournoi
// (nom + format + joueurs), puis suivi du bracket/classement et
// lancement du prochain match (voir TournamentManager pour la logique).
//
// Ne demarre PAS le match elle-meme (cette fenetre ne connait rien au
// moteur de jeu reel) : emet matchRequested(p1, p2) et se ferme,
// MainWindow se charge de vraiment lancer le match et, une fois
// termine, d'enregistrer le resultat dans TournamentManager (voir
// MainWindow::handleTournamentMatchFinished()).
//
// Un tournoi de club peut avoir plusieurs matchs joues EN MEME TEMPS
// sur d'autres tables (chacune son propre PC/instance de l'appli, voir
// [[project_match_web_server]]) : cette fenetre ne peut donc pas
// supposer que "le prochain match" se joue forcement ici. Tout match en
// attente (pas seulement le tout premier) peut etre choisi dans
// m_pendingMatchCombo et soit lance sur CETTE table, soit avoir son
// resultat saisi manuellement (deja joue ailleurs) via enterResultManually().
// =====================================================================
class TournamentDialog : public QDialog
{
    Q_OBJECT

public:
    // `kind` : Tournoi (comportement historique) ou Championnat (round
    // robin aller-retour, nombre de frames choisi a la creation) -- meme
    // ecran, memes mecaniques, voir TournamentManager::Kind.
    explicit TournamentDialog(TournamentManager::Kind kind = TournamentManager::Kind::Tournament, QWidget* parent = nullptr);

signals:
    // framesToWin : longueur du match (2 pour un tournoi, choisie a la
    // creation pour un championnat).
    void matchRequested(const QString& player1, const QString& player2, int framesToWin);

private:
    void showCreationForm();
    void showActiveTournament();
    void refreshActiveView();
    void createTournament();
    void confirmNewTournament();
    void clearTournament();
    void launchSelectedMatch();
    void enterResultManually();
    // Apercu avant impression / export PDF / impression de la feuille
    // de la competition (voir CompetitionPrintout).
    void printCompetition();

    // Renvoie le Matchup selectionne dans m_pendingMatchCombo, ou
    // nullptr si aucun (liste vide -- tournoi fini ou round d'elimination
    // pas encore entierement determine).
    const TournamentManager::Matchup* selectedPendingMatch() const;

    QStackedWidget* m_stack = nullptr;

    // --- Page de creation ---
    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_formatCombo = nullptr;
    QComboBox* m_framesCombo = nullptr; // Championnat uniquement
    QListWidget* m_playerCheckList = nullptr;

    // --- Page tournoi actif ---
    QLabel* m_titleLabel = nullptr;
    QLabel* m_bracketLabel = nullptr;
    QComboBox* m_pendingMatchCombo = nullptr;
    QPushButton* m_launchButton = nullptr;
    QPushButton* m_manualResultButton = nullptr;

    bool m_isChampionship = false;
    TournamentManager m_tournament;
};
