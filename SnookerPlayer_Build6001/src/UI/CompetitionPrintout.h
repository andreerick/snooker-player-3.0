#pragma once

#include <QString>

#include "TournamentManager.h"

// =====================================================================
// CompetitionPrintout
// ---------------------------------------------------------------------
// Construit la page imprimable (HTML pour QTextDocument, A4, noir sur
// blanc pour economiser l'encre) d'un championnat ou d'un tournoi :
// titre, matchs (par tour / aller-retour) avec scores, classement et
// champion. Les matchs pas encore joues ont une case "__ - __" a
// remplir au stylo, pour suivre la competition sur papier.
//
// Aucune dependance a l'interface : testable seule (voir
// tests/CompetitionTests.cpp). L'impression elle-meme (apercu, PDF,
// imprimante) est faite par TournamentDialog::printCompetition().
// =====================================================================
QString competitionPrintoutHtml(const TournamentManager& competition);

// Feuille VIERGE a remplir a la main (avant ou sans l'appli) : liste de
// joueurs numerotes J1..Jn a nommer au stylo, matchs deja numerotes
// ("J1" vs "J2", ou "Vainqueur match 3" en elimination), cases score et
// vainqueur vides, tableau de classement vide. `format` : Elimination,
// RoundRobin ou League (championnat aller-retour). Meme mise en page que
// competitionPrintoutHtml().
QString blankCompetitionPrintoutHtml(TournamentManager::Format format, int playerCount);
