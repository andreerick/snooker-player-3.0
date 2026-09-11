#include "TournamentDialog.h"
#include "../Storage/MatchStorage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QAbstractItemView>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QScrollArea>
#include <QSettings>
#include <QCoreApplication>
#include <QSet>
#include <QDate>
#include <QListWidgetItem>
#include <QSpinBox>
#include <algorithm>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kOrange = "#f5a623";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";

    // Meme union que PlayersDialog::allKnownPlayerNames() (dupliquee ici
    // plutot que partagee, voir la convention deja en place dans ce
    // projet pour ce genre de petit helper) : joueurs.ini + tout nom
    // deja rencontre dans l'historique des matchs.
    QStringList allKnownPlayerNames()
    {
        QSet<QString> names;
        QSettings settings(QCoreApplication::applicationDirPath() + "/joueurs.ini", QSettings::IniFormat);
        for (const QString& name : settings.value("joueurs/noms").toStringList())
        {
            names.insert(name);
        }
        for (const QJsonValue& value : MatchStorage::loadHistory())
        {
            QJsonObject match = value.toObject();
            names.insert(match.value("joueur1").toString());
            names.insert(match.value("joueur2").toString());
        }
        names.remove(QString());
        QStringList sorted = names.values();
        sorted.sort(Qt::CaseInsensitive);
        return sorted;
    }

    QString matchupLine(const TournamentManager::Matchup& m)
    {
        QString p1 = m.player1.isEmpty() ? "?" : m.player1;
        QString p2 = m.player2.isEmpty() ? "?" : m.player2;

        if (m.isBye)
        {
            return "<span style='color:" + kGray + ";'>" + m.winner + " (exempt ce tour)</span>";
        }
        if (!m.isPlayed())
        {
            return p1 + " vs " + p2 + " <span style='color:" + kGray + ";'>(a jouer)</span>";
        }
        QString winnerP1 = "<span style='color:" + kGreen + "; font-weight:bold;'>" + p1 + " (" + QString::number(m.scorePlayer1) + ")</span>";
        QString winnerP2 = "<span style='color:" + kGreen + "; font-weight:bold;'>" + p2 + " (" + QString::number(m.scorePlayer2) + ")</span>";
        QString loserP1 = "<span style='color:" + kGray + ";'>" + p1 + " (" + QString::number(m.scorePlayer1) + ")</span>";
        QString loserP2 = "<span style='color:" + kGray + ";'>" + p2 + " (" + QString::number(m.scorePlayer2) + ")</span>";
        return (m.winner == m.player1 ? winnerP1 : loserP1) + " vs " + (m.winner == m.player2 ? winnerP2 : loserP2);
    }
}

TournamentDialog::TournamentDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Tournoi");
    resize(760, 600);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    m_stack = new QStackedWidget(this);
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->addWidget(m_stack);

    // --- Page 0 : creation ---
    QWidget* creationPage = new QWidget(this);
    QVBoxLayout* creationLayout = new QVBoxLayout(creationPage);
    creationLayout->setSpacing(14);

    QLabel* creationTitle = new QLabel("NOUVEAU TOURNOI", creationPage);
    creationTitle->setStyleSheet("color: " + kWhite + "; font-size: 16px; font-weight: bold; letter-spacing: 1px;");
    creationLayout->addWidget(creationTitle);

    QString fieldStyle =
        "background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px;";

    QLabel* nameHint = new QLabel("Nom du tournoi", creationPage);
    nameHint->setStyleSheet("color: " + kGray + "; font-size: 11px;");
    creationLayout->addWidget(nameHint);
    m_nameEdit = new QLineEdit(creationPage);
    m_nameEdit->setStyleSheet(fieldStyle);
    m_nameEdit->setPlaceholderText("Ex. Tournoi du club - Septembre 2026");
    creationLayout->addWidget(m_nameEdit);

    QLabel* formatHint = new QLabel("Format", creationPage);
    formatHint->setStyleSheet("color: " + kGray + "; font-size: 11px; margin-top: 8px;");
    creationLayout->addWidget(formatHint);
    m_formatCombo = new QComboBox(creationPage);
    m_formatCombo->setStyleSheet(
        "QComboBox { " + fieldStyle + " }"
    );
    m_formatCombo->addItem("Elimination directe");
    m_formatCombo->addItem("Round robin (tout le monde affronte tout le monde)");
    creationLayout->addWidget(m_formatCombo);

    QLabel* playersHint = new QLabel("Joueurs participants (2 minimum)", creationPage);
    playersHint->setStyleSheet("color: " + kGray + "; font-size: 11px; margin-top: 8px;");
    creationLayout->addWidget(playersHint);

    QLabel* playersSubHint = new QLabel("Cliquez sur un ou plusieurs noms pour les inclure (surlignes en vert)", creationPage);
    playersSubHint->setStyleSheet("color: " + kGray + "; font-size: 10px;");
    creationLayout->addWidget(playersSubHint);

    // Selection multiple plutot que des cases a cocher : une case a
    // cocher Qt ne se coche qu'en cliquant tres precisement sur la
    // petite icone (peu decouvrable, source de confusion constatee a
    // l'usage) -- en mode MultiSelection, cliquer n'importe ou sur la
    // ligne bascule fiablement son inclusion, et le surlignage
    // (QListWidget::item:selected, deja stylise) rend l'etat evident.
    m_playerCheckList = new QListWidget(creationPage);
    m_playerCheckList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_playerCheckList->setStyleSheet(
        "QListWidget { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px; }"
        "QListWidget::item { padding: 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: " + kGreen + "; color: " + kWhite + "; }"
    );
    for (const QString& name : allKnownPlayerNames())
    {
        new QListWidgetItem(name, m_playerCheckList);
    }
    creationLayout->addWidget(m_playerCheckList, 1);

    QPushButton* createButton = new QPushButton("Creer le tournoi", creationPage);
    createButton->setStyleSheet(
        "QPushButton { background-color: " + kGreen + "; color: " + kWhite + ";"
        "border: none; border-radius: 6px; padding: 12px; font-weight: bold; }"
    );
    connect(createButton, &QPushButton::clicked, this, &TournamentDialog::createTournament);
    creationLayout->addWidget(createButton);

    m_stack->addWidget(creationPage);

    // --- Page 1 : tournoi actif ---
    QWidget* activePage = new QWidget(this);
    QVBoxLayout* activeLayout = new QVBoxLayout(activePage);
    activeLayout->setSpacing(10);

    m_titleLabel = new QLabel(activePage);
    m_titleLabel->setStyleSheet("color: " + kWhite + "; font-size: 16px; font-weight: bold; letter-spacing: 1px;");
    activeLayout->addWidget(m_titleLabel);

    QScrollArea* scrollArea = new QScrollArea(activePage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 6px; }"
    );
    m_bracketLabel = new QLabel(activePage);
    m_bracketLabel->setWordWrap(true);
    m_bracketLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_bracketLabel->setStyleSheet("color: " + kWhite + "; padding: 14px; font-size: 13px;");
    m_bracketLabel->setTextFormat(Qt::RichText);
    scrollArea->setWidget(m_bracketLabel);
    activeLayout->addWidget(scrollArea, 1);

    // Un tournoi de club peut avoir plusieurs matchs en attente EN MEME
    // TEMPS, potentiellement joues sur d'autres tables : m_pendingMatchCombo
    // liste TOUS les matchs en attente (pas seulement "le premier"), et
    // l'utilisateur choisit soit de le jouer sur cette table, soit de
    // juste saisir son resultat (deja joue ailleurs) -- voir
    // launchSelectedMatch()/enterResultManually().
    QLabel* pendingLabel = new QLabel("Match en attente :", activePage);
    pendingLabel->setStyleSheet("color: " + kGray + "; font-size: 11px;");
    activeLayout->addWidget(pendingLabel);

    m_pendingMatchCombo = new QComboBox(activePage);
    m_pendingMatchCombo->setStyleSheet(
        "QComboBox { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 4px; padding: 6px; }"
    );
    activeLayout->addWidget(m_pendingMatchCombo);

    QHBoxLayout* nextRow = new QHBoxLayout();
    m_launchButton = new QPushButton("Lancer sur cette table", activePage);
    m_launchButton->setStyleSheet(
        "QPushButton { background-color: " + kGreen + "; color: " + kWhite + ";"
        "border: none; border-radius: 6px; padding: 10px 18px; font-weight: bold; }"
        "QPushButton:disabled { background-color: " + kPanel + "; color: " + kGray + "; }"
    );
    connect(m_launchButton, &QPushButton::clicked, this, &TournamentDialog::launchSelectedMatch);
    nextRow->addWidget(m_launchButton);

    m_manualResultButton = new QPushButton("Saisir un resultat...", activePage);
    m_manualResultButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 10px 18px; }"
        "QPushButton:hover { border-color: " + kGray + "; }"
        "QPushButton:disabled { color: " + kGray + "; }"
    );
    connect(m_manualResultButton, &QPushButton::clicked, this, &TournamentDialog::enterResultManually);
    nextRow->addWidget(m_manualResultButton);
    activeLayout->addLayout(nextRow);

    QLabel* manualHint = new QLabel(
        "\"Saisir un resultat\" sert pour un match deja joue sur une autre table "
        "(chaque table a sa propre instance de l'application) : entre le score final "
        "sans rien lancer ici.",
        activePage
    );
    manualHint->setWordWrap(true);
    manualHint->setStyleSheet("color: " + kGray + "; font-size: 10px;");
    activeLayout->addWidget(manualHint);

    QHBoxLayout* bottomRow = new QHBoxLayout();
    QString smallButtonStyle =
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 4px; padding: 8px 14px;"
        "}"
        "QPushButton:hover { border-color: " + kGray + "; }";
    QPushButton* newTournamentButton = new QPushButton("Nouveau tournoi", activePage);
    newTournamentButton->setStyleSheet(smallButtonStyle);
    connect(newTournamentButton, &QPushButton::clicked, this, &TournamentDialog::confirmNewTournament);
    bottomRow->addWidget(newTournamentButton);

    // Distinct de "Nouveau tournoi" (qui invite a en recreer un) : sert
    // juste a effacer sans rien recreer. Reste ici (pas dans Parametres)
    // puisque c'est l'ecran du tournoi, sur demande de l'utilisateur.
    QPushButton* clearTournamentButton = new QPushButton("Effacer le tournoi en cours", activePage);
    clearTournamentButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + "; color: #e74c3c;"
        "  border: 1px solid " + kBorder + "; border-radius: 4px; padding: 8px 14px;"
        "}"
        "QPushButton:hover { border-color: #e74c3c; }"
    );
    connect(clearTournamentButton, &QPushButton::clicked, this, &TournamentDialog::clearTournament);
    bottomRow->addWidget(clearTournamentButton);

    bottomRow->addStretch();
    QPushButton* closeButton = new QPushButton("Fermer", activePage);
    closeButton->setStyleSheet(smallButtonStyle);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    bottomRow->addWidget(closeButton);
    activeLayout->addLayout(bottomRow);

    m_stack->addWidget(activePage);

    m_tournament = TournamentManager::load();
    if (m_tournament.isActive())
    {
        showActiveTournament();
    }
    else
    {
        showCreationForm();
    }
}

void TournamentDialog::showCreationForm()
{
    m_stack->setCurrentIndex(0);
}

void TournamentDialog::showActiveTournament()
{
    m_stack->setCurrentIndex(1);
    refreshActiveView();
}

void TournamentDialog::refreshActiveView()
{
    QString formatText = (m_tournament.format() == TournamentManager::Format::Elimination)
        ? "Elimination directe" : "Round robin";
    m_titleLabel->setText(m_tournament.name() + " -- " + formatText);

    QString html;
    int currentRound = -1;
    for (const TournamentManager::Matchup& m : m_tournament.matchups())
    {
        if (m_tournament.format() == TournamentManager::Format::Elimination && m.round != currentRound)
        {
            currentRound = m.round;
            html += "<p style='color:" + kGray + "; font-size:11px; letter-spacing:1px; margin-top:14px;'>ROUND " + QString::number(currentRound) + "</p>";
        }
        html += "<p style='margin:4px 0;'>" + matchupLine(m) + "</p>";
    }

    if (m_tournament.format() == TournamentManager::Format::RoundRobin)
    {
        html += "<p style='color:" + kGray + "; font-size:11px; letter-spacing:1px; margin-top:18px;'>CLASSEMENT</p>";
        html += "<table cellspacing='0' cellpadding='4' style='color:" + kWhite + ";'>";
        html += "<tr style='color:" + kGray + ";'><td>Joueur</td><td>V</td><td>J</td><td>Frames</td></tr>";
        for (const TournamentManager::Standing& s : m_tournament.standings())
        {
            html += "<tr><td>" + s.name + "</td><td>" + QString::number(s.won) + "</td><td>"
                  + QString::number(s.played) + "</td><td>" + QString::number(s.framesFor) + "-" + QString::number(s.framesAgainst) + "</td></tr>";
        }
        html += "</table>";
    }

    if (m_tournament.isFinished())
    {
        html += "<p style='color:" + kOrange + "; font-size:15px; font-weight:bold; margin-top:18px;'>CHAMPION : " + m_tournament.champion() + "</p>";
    }
    m_bracketLabel->setText(html);

    QVector<const TournamentManager::Matchup*> pending = m_tournament.pendingMatches();
    m_pendingMatchCombo->clear();
    for (const TournamentManager::Matchup* m : pending)
    {
        QString label = m->player1 + " vs " + m->player2;
        if (m_tournament.format() == TournamentManager::Format::Elimination)
        {
            label = "Round " + QString::number(m->round) + " : " + label;
        }
        // itemData porte les deux noms (pas un pointeur -- invalide des
        // que m_tournament est reassigne/rechargee) : identifie le
        // Matchup a nouveau au moment du clic, voir selectedPendingMatch().
        m_pendingMatchCombo->addItem(label, QStringList{ m->player1, m->player2 });
    }

    bool hasPending = !pending.isEmpty();
    m_pendingMatchCombo->setEnabled(hasPending);
    m_launchButton->setEnabled(hasPending);
    m_manualResultButton->setEnabled(hasPending);
    if (!hasPending)
    {
        m_pendingMatchCombo->addItem(m_tournament.isFinished() ? "Tournoi termine" : "Aucun match determine pour l'instant");
    }
}

const TournamentManager::Matchup* TournamentDialog::selectedPendingMatch() const
{
    QStringList names = m_pendingMatchCombo->currentData().toStringList();
    if (names.size() != 2)
    {
        return nullptr;
    }
    for (const TournamentManager::Matchup& m : m_tournament.matchups())
    {
        if (m.isPlayed() || m.isBye)
        {
            continue;
        }
        if (m.player1 == names[0] && m.player2 == names[1])
        {
            return &m;
        }
    }
    return nullptr;
}

void TournamentDialog::launchSelectedMatch()
{
    const TournamentManager::Matchup* match = selectedPendingMatch();
    if (!match)
    {
        return;
    }
    emit matchRequested(match->player1, match->player2);
    accept();
}

void TournamentDialog::enterResultManually()
{
    const TournamentManager::Matchup* match = selectedPendingMatch();
    if (!match)
    {
        return;
    }
    QString player1 = match->player1;
    QString player2 = match->player2;

    QDialog dialog(this);
    dialog.setWindowTitle("Saisir un resultat");
    dialog.setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    QLabel* info = new QLabel(
        "Score final de \"" + player1 + "\" vs \"" + player2 + "\" (deja joue sur une autre table) :",
        &dialog
    );
    info->setWordWrap(true);
    layout->addWidget(info);

    QString spinStyle =
        "QSpinBox { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 4px; padding: 6px; }";

    QHBoxLayout* row1 = new QHBoxLayout();
    QLabel* label1 = new QLabel(player1 + " :", &dialog);
    QSpinBox* score1 = new QSpinBox(&dialog);
    score1->setRange(0, 99);
    score1->setStyleSheet(spinStyle);
    row1->addWidget(label1, 1);
    row1->addWidget(score1);
    layout->addLayout(row1);

    QHBoxLayout* row2 = new QHBoxLayout();
    QLabel* label2 = new QLabel(player2 + " :", &dialog);
    QSpinBox* score2 = new QSpinBox(&dialog);
    score2->setRange(0, 99);
    score2->setStyleSheet(spinStyle);
    row2->addWidget(label2, 1);
    row2->addWidget(score2);
    layout->addLayout(row2);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    QPushButton* cancelButton = new QPushButton("Annuler", &dialog);
    QPushButton* validateButton = new QPushButton("Valider", &dialog);
    QString dialogButtonStyle =
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }";
    cancelButton->setStyleSheet(dialogButtonStyle);
    validateButton->setStyleSheet(dialogButtonStyle);
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(validateButton);
    layout->addLayout(buttonRow);

    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(validateButton, &QPushButton::clicked, &dialog, [&]()
        {
            if (score1->value() == score2->value())
            {
                QMessageBox box(QMessageBox::Warning, "Score invalide",
                    "Les deux scores sont egaux : il faut un vainqueur (pas d'egalite possible en snooker).",
                    QMessageBox::Ok, &dialog);
                box.setStyleSheet(
                    "QMessageBox { background-color: " + kBg + "; }"
                    "QLabel { color: " + kWhite + "; background: transparent; }"
                    "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
                    "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
                );
                box.exec();
                return;
            }
            dialog.accept();
        });

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QString winner = (score1->value() > score2->value()) ? player1 : player2;
    int winnerScore = std::max(score1->value(), score2->value());
    int loserScore = std::min(score1->value(), score2->value());
    m_tournament.recordResult(player1, player2, winner, winnerScore, loserScore);
    m_tournament.save();
    refreshActiveView();
}

void TournamentDialog::createTournament()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
    {
        name = "Tournoi du " + QDate::currentDate().toString("dd/MM/yyyy");
    }

    QStringList selected;
    for (QListWidgetItem* item : m_playerCheckList->selectedItems())
    {
        selected.append(item->text());
    }

    if (selected.size() < 2)
    {
        QMessageBox box(QMessageBox::Warning, "Tournoi", "Selectionnez au moins 2 joueurs.", QMessageBox::Ok, this);
        box.setStyleSheet(
            "QMessageBox { background-color: " + kBg + "; }"
            "QLabel { color: " + kWhite + "; background: transparent; }"
            "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
        );
        box.exec();
        return;
    }

    TournamentManager::Format format = (m_formatCombo->currentIndex() == 0)
        ? TournamentManager::Format::Elimination : TournamentManager::Format::RoundRobin;

    m_tournament.create(name, format, selected);
    m_tournament.save();
    showActiveTournament();
}

void TournamentDialog::confirmNewTournament()
{
    if (!m_tournament.isFinished())
    {
        QMessageBox box(QMessageBox::Question, "Nouveau tournoi",
            "Le tournoi en cours n'est pas termine. Le remplacer par un nouveau tournoi effacera sa progression. Continuer ?",
            QMessageBox::Yes | QMessageBox::No, this);
        box.setStyleSheet(
            "QMessageBox { background-color: " + kBg + "; }"
            "QLabel { color: " + kWhite + "; background: transparent; }"
            "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
        );
        if (box.exec() != QMessageBox::Yes)
        {
            return;
        }
    }
    m_tournament.clear();
    showCreationForm();
}

void TournamentDialog::clearTournament()
{
    QMessageBox box(QMessageBox::Warning, "Effacer le tournoi",
        "Effacer definitivement le tournoi en cours (progression comprise) ?",
        QMessageBox::Yes | QMessageBox::No, this);
    box.setStyleSheet(
        "QMessageBox { background-color: " + kBg + "; }"
        "QLabel { color: " + kWhite + "; background: transparent; }"
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
    );
    if (box.exec() != QMessageBox::Yes)
    {
        return;
    }

    // A la difference de confirmNewTournament() (qui vide juste l'etat
    // en memoire avant d'en recreer un), efface aussi le fichier :
    // "Effacer" doit rester efface meme si on rouvre cette fenetre sans
    // avoir cree de nouveau tournoi entre-temps.
    m_tournament.clear();
    m_tournament.save();
    showCreationForm();
}
