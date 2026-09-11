#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class QLineEdit;
class QListWidget;
class QLabel;

// =====================================================================
// RulesReferenceDialog
// ---------------------------------------------------------------------
// Reference rapide au reglement officiel du snooker (texte francais,
// voir docs/reglement_snooker/), pour pouvoir sortir l'extrait exact
// d'une regle en cas de litige pendant un match, sans quitter l'appli
// ni chercher a la main dans le PDF complet.
//
// Charge le texte une seule fois (voir loadParagraphs()), le decoupe en
// paragraphes (blocs separes par une ligne vide dans le texte extrait
// du PDF), et filtre ces paragraphes en direct au fur et a mesure de la
// saisie dans la barre de recherche.
// =====================================================================
class RulesReferenceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RulesReferenceDialog(QWidget* parent = nullptr);

private slots:
    void updateResults(const QString& searchText);

private:
    void loadParagraphs();

    QLineEdit* m_searchBox = nullptr;
    QListWidget* m_resultsList = nullptr;
    QLabel* m_resultCountLabel = nullptr;

    // Paragraphes extraits du reglement (texte francais officiel),
    // dans l'ordre du document. Charges une seule fois a la construction.
    QStringList m_paragraphs;
};
