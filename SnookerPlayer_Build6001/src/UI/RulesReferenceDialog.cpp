#include "RulesReferenceDialog.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QFile>
#include <QTextStream>

// Chemin du texte du reglement officiel (extrait du PDF francais, voir
// docs/reglement_snooker/), injecte par CMakeLists.txt via
// target_compile_definitions (chemin absolu source, valable en
// developpement -- pas un chemin "installe", ce projet n'a pas encore
// d'etape de deploiement/packaging).
#ifndef RULES_TEXT_PATH
#define RULES_TEXT_PATH ""
#endif

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";
}

RulesReferenceDialog::RulesReferenceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Reglement officiel du snooker");
    resize(720, 560);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    QLabel* title = new QLabel(
        "Recherche dans le reglement officiel (traduction francaise, MAJ septembre 2024)", this
    );
    title->setStyleSheet("color: " + kGray + "; font-size: 12px;");
    title->setWordWrap(true);
    layout->addWidget(title);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Ex : free ball, faute, miss, couleur, respotee...");
    m_searchBox->setStyleSheet(
        "QLineEdit {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 5px;"
        "  padding: 8px; font-size: 14px;"
        "}"
    );
    layout->addWidget(m_searchBox);

    m_resultCountLabel = new QLabel(this);
    m_resultCountLabel->setStyleSheet("color: " + kGray + "; font-size: 11px;");
    layout->addWidget(m_resultCountLabel);

    m_resultsList = new QListWidget(this);
    m_resultsList->setStyleSheet(
        "QListWidget {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 5px;"
        "}"
        "QListWidget::item {"
        "  padding: 10px; border-bottom: 1px solid " + kBorder + ";"
        "}"
    );
    m_resultsList->setWordWrap(true);
    m_resultsList->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_resultsList, 1);

    connect(m_searchBox, &QLineEdit::textChanged, this, &RulesReferenceDialog::updateResults);

    loadParagraphs();
    updateResults(QString());
}

void RulesReferenceDialog::loadParagraphs()
{
    QFile file(RULES_TEXT_PATH);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_paragraphs.append(
            "Reglement introuvable (" + QString(RULES_TEXT_PATH) + "). "
            "Verifiez que docs/reglement_snooker/Regles_officielles_snooker_FR_sept2024.txt existe."
        );
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    // Un paragraphe = un bloc de lignes non vides consecutives dans le
    // texte extrait du PDF (les paragraphes du reglement sont separes
    // par une ligne vide). Les blocs trop courts (numeros de page, en-tetes
    // de section repetes comme "SECTION 3 - SNOOKER") sont ignores : ils ne
    // constituent pas un extrait de regle citable en cas de litige.
    QStringList currentLines;
    auto flushParagraph = [this, &currentLines]()
        {
            if (currentLines.isEmpty())
            {
                return;
            }
            QString paragraph = currentLines.join(" ").simplified();
            currentLines.clear();
            if (paragraph.length() < 20)
            {
                return;
            }
            m_paragraphs.append(paragraph);
        };

    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        if (line.trimmed().isEmpty())
        {
            flushParagraph();
        }
        else
        {
            currentLines.append(line.trimmed());
        }
    }
    flushParagraph();
}

void RulesReferenceDialog::updateResults(const QString& searchText)
{
    m_resultsList->clear();

    QString needle = searchText.trimmed();
    int shown = 0;
    const int maxResults = 200; // evite d'afficher les ~600 paragraphes d'un coup sans recherche

    for (const QString& paragraph : m_paragraphs)
    {
        bool matches = needle.isEmpty() || paragraph.contains(needle, Qt::CaseInsensitive);
        if (!matches)
        {
            continue;
        }
        m_resultsList->addItem(paragraph);
        ++shown;
        if (needle.isEmpty() && shown >= maxResults)
        {
            break;
        }
    }

    if (needle.isEmpty())
    {
        m_resultCountLabel->setText(
            QString::number(m_paragraphs.size()) + " extraits au total (les " + QString::number(maxResults) +
            " premiers affiches -- tapez un mot-cle pour filtrer)"
        );
    }
    else
    {
        m_resultCountLabel->setText(QString::number(shown) + " extrait(s) trouve(s)");
    }
}
