#pragma once

#include <QDialog>

// =====================================================================
// CompetitionChoiceDialog
// ---------------------------------------------------------------------
// Ouverte par la tuile "CHAMPIONNAT ET TOURNOI" de HomeScreen (voir
// HomeScreen::tournamentRequested) : propose 2 choix independants --
// "Tournoi" (tout ce qui existait deja : TournamentDialog) et
// "Championnat" (nouveau) -- au lieu d'ouvrir directement le tournoi
// comme avant. Meme principe que TrainingChoiceDialog.
// =====================================================================
class CompetitionChoiceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CompetitionChoiceDialog(QWidget* parent = nullptr);

signals:
    void championshipRequested();
    void tournamentRequested();
};
