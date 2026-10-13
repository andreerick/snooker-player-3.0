#include "ExerciseScoreDialog.h"
#include "ExerciseSessions.h"
#include "PlayersDialog.h"
#include "UiUtils.h"

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";

    struct BallInfo
    {
        int value;
        const char* name;
        const char* color;
        const char* textColor;
    };

    const BallInfo kBalls[7] = {
        { 1, "Rouge", "#ff3b30", "#ffffff" },
        { 2, "Jaune", "#ffcc00", "#000000" },
        { 3, "Verte", "#00c853", "#000000" },
        { 4, "Marron", "#8b4513", "#ffffff" },
        { 5, "Bleue", "#1565ff", "#ffffff" },
        { 6, "Rose", "#ff4fa3", "#000000" },
        { 7, "Noire", "#161616", "#ffffff" },
    };

    QString ballButtonStyle(const BallInfo& ball)
    {
        return QString(
            "QPushButton { background-color: %1; color: %2; border: 2px solid %3;"
            "border-radius: 8px; padding: 10px 6px; font-size: 13px; font-weight: bold; }"
            "QPushButton:disabled { background-color: #1a1c1f; color: #4a4d52; border-color: #2a2d31; }"
        ).arg(ball.color, ball.textColor, kBorder);
    }

    QString plainButtonStyle()
    {
        return "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
               "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 9px 14px; }"
               "QPushButton:hover { border-color: " + kGreen + "; }"
               "QPushButton:disabled { color: #4a4d52; }";
    }
}

ExerciseScoreDialog::ExerciseScoreDialog(const QString& exerciseName, QWidget* parent)
    : QDialog(parent)
    , m_exerciseName(exerciseName)
{
    setWindowTitle("Compter une serie - " + exerciseName);
    applyDarkTitleBar(this);
    resize(640, 600);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    QHBoxLayout* playerRow = new QHBoxLayout();
    playerRow->addWidget(new QLabel("Joueur :", this));
    m_playerCombo = new QComboBox(this);
    m_playerCombo->setStyleSheet(
        "QComboBox { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 6px 10px; }"
        "QComboBox QAbstractItemView { background-color: " + kPanel + "; color: " + kWhite + "; }"
    );
    QStringList players = knownPlayerNames();
    if (players.isEmpty())
    {
        players << "Invite";
    }
    m_playerCombo->addItems(players);
    playerRow->addWidget(m_playerCombo, 1);
    root->addLayout(playerRow);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("color: " + kGray + "; font-size: 12px;");
    root->addWidget(m_summaryLabel);

    m_scoreLabel = new QLabel(this);
    m_scoreLabel->setAlignment(Qt::AlignCenter);
    m_scoreLabel->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + ";"
        "border-radius: 8px; padding: 14px; font-size: 22px; font-weight: bold;"
    );
    root->addWidget(m_scoreLabel);

    m_nextLabel = new QLabel(this);
    m_nextLabel->setAlignment(Qt::AlignCenter);
    m_nextLabel->setStyleSheet("font-size: 15px; color: " + kWhite + ";");
    root->addWidget(m_nextLabel);

    QHBoxLayout* ballRow = new QHBoxLayout();
    ballRow->setSpacing(8);
    for (int i = 0; i < 7; ++i)
    {
        QPushButton* button = new QPushButton(kBalls[i].name, this);
        button->setStyleSheet(ballButtonStyle(kBalls[i]));
        button->setMinimumHeight(52);
        int value = kBalls[i].value;
        connect(button, &QPushButton::clicked, this, [this, value]() { potBall(value); });
        m_ballButtons[i] = button;
        ballRow->addWidget(button);
    }
    root->addLayout(ballRow);

    m_sequenceLabel = new QLabel(this);
    m_sequenceLabel->setWordWrap(true);
    m_sequenceLabel->setTextFormat(Qt::RichText);
    m_sequenceLabel->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + ";"
        "border-radius: 6px; padding: 8px; font-size: 16px;"
    );
    m_sequenceLabel->setMinimumHeight(60);
    m_sequenceLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    root->addWidget(m_sequenceLabel);

    QHBoxLayout* actionRow = new QHBoxLayout();
    m_undoButton = new QPushButton("Annuler la derniere bille", this);
    m_undoButton->setStyleSheet(plainButtonStyle());
    connect(m_undoButton, &QPushButton::clicked, this, &ExerciseScoreDialog::undoBall);
    actionRow->addWidget(m_undoButton);

    m_endButton = new QPushButton("Rate - terminer la serie", this);
    m_endButton->setStyleSheet(plainButtonStyle());
    connect(m_endButton, &QPushButton::clicked, this, &ExerciseScoreDialog::endSeries);
    actionRow->addWidget(m_endButton);
    root->addLayout(actionRow);

    QHBoxLayout* saveRow = new QHBoxLayout();
    m_saveButton = new QPushButton("Enregistrer la serie", this);
    m_saveButton->setStyleSheet(plainButtonStyle());
    connect(m_saveButton, &QPushButton::clicked, this, &ExerciseScoreDialog::saveSeries);
    saveRow->addWidget(m_saveButton);

    m_newButton = new QPushButton("Nouvelle serie", this);
    m_newButton->setStyleSheet(plainButtonStyle());
    connect(m_newButton, &QPushButton::clicked, this, &ExerciseScoreDialog::newSeries);
    saveRow->addWidget(m_newButton);
    root->addLayout(saveRow);

    m_historyList = new QListWidget(this);
    m_historyList->setStyleSheet(
        "QListWidget { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px; }"
    );
    root->addWidget(m_historyList, 1);

    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(plainButtonStyle());
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    root->addWidget(closeButton, 0, Qt::AlignRight);

    connect(m_playerCombo, &QComboBox::currentTextChanged, this, [this]() { refreshHistory(); });

    refreshSeries();
    refreshHistory();
}

void ExerciseScoreDialog::potBall(int value)
{
    if (m_ended)
    {
        return;
    }
    m_series.pot(value);
    if (m_series.isDone())
    {
        m_ended = true;
    }
    refreshSeries();
}

void ExerciseScoreDialog::undoBall()
{
    if (m_saved)
    {
        return;
    }
    if (m_series.undo())
    {
        m_ended = m_series.isDone();
        refreshSeries();
    }
}

void ExerciseScoreDialog::endSeries()
{
    m_ended = true;
    refreshSeries();
}

void ExerciseScoreDialog::saveSeries()
{
    if (!m_ended || m_saved)
    {
        return;
    }
    QStringList values;
    for (int value : m_series.sequence())
    {
        values << QString::number(value);
    }

    ExerciseSession session;
    session.date = QDateTime::currentDateTime().toString(Qt::ISODate);
    session.exercise = m_exerciseName;
    session.player = m_playerCombo->currentText();
    session.points = m_series.points();
    session.reds = m_series.redsPotted();
    session.sequence = values.join(",");
    ExerciseSessions::append(session);

    m_saved = true;
    refreshSeries();
    refreshHistory();
}

void ExerciseScoreDialog::newSeries()
{
    m_series = ExerciseSeries();
    m_ended = false;
    m_saved = false;
    refreshSeries();
}

void ExerciseScoreDialog::refreshSeries()
{
    m_scoreLabel->setText(
        QString("Points : %1      Rouges : %2 / %3")
            .arg(m_series.points()).arg(m_series.redsPotted()).arg(m_series.redCount())
    );

    QString nextText;
    switch (m_series.next())
    {
    case ExerciseSeries::Next::Red:
        nextText = "Prochaine bille : une ROUGE";
        break;
    case ExerciseSeries::Next::AnyColor:
        nextText = "Prochaine bille : une COULEUR";
        break;
    case ExerciseSeries::Next::SpecificColor:
        nextText = QString("Prochaine bille : la %1")
                       .arg(QString(kBalls[m_series.nextColorValue() - 1].name).toUpper());
        break;
    case ExerciseSeries::Next::Done:
        nextText = "Serie complete !";
        break;
    }
    if (m_ended && m_series.next() != ExerciseSeries::Next::Done)
    {
        nextText = "Serie terminee";
    }
    m_nextLabel->setText(nextText);

    for (int i = 0; i < 7; ++i)
    {
        ExerciseSeries probe = m_series;
        m_ballButtons[i]->setEnabled(!m_ended && probe.pot(kBalls[i].value));
    }

    const std::vector<int>& sequence = m_series.sequence();
    if (sequence.empty())
    {
        m_sequenceLabel->setText("Aucune bille pour l'instant.");
    }
    else
    {
        // Pastilles dessinees (une noire reste visible sur fond sombre).
        const int diameter = 24;
        const int spacing = 30;
        const int perRow = 18;
        int rows = (static_cast<int>(sequence.size()) + perRow - 1) / perRow;
        QPixmap strip(perRow * spacing, rows * spacing);
        strip.fill(Qt::transparent);
        QPainter painter(&strip);
        painter.setRenderHint(QPainter::Antialiasing);
        for (size_t i = 0; i < sequence.size(); ++i)
        {
            int col = static_cast<int>(i) % perRow;
            int row = static_cast<int>(i) / perRow;
            painter.setPen(QPen(QColor("#9a9a9a"), 1.5));
            painter.setBrush(QColor(kBalls[sequence[i] - 1].color));
            painter.drawEllipse(col * spacing + 2, row * spacing + 2, diameter, diameter);
        }
        painter.end();
        m_sequenceLabel->setPixmap(strip);
    }

    m_undoButton->setEnabled(!m_saved && !m_series.sequence().empty());
    m_endButton->setEnabled(!m_ended);
    m_saveButton->setEnabled(m_ended && !m_saved);
    m_saveButton->setText(m_saved ? "Serie enregistree" : "Enregistrer la serie");
    m_newButton->setEnabled(m_ended);
}

void ExerciseScoreDialog::refreshHistory()
{
    QString player = m_playerCombo->currentText();
    ExerciseSummary summary = ExerciseSessions::summaryFor(player, m_exerciseName);
    if (summary.count == 0)
    {
        m_summaryLabel->setText("Aucune serie enregistree pour " + player + " sur cet exercice.");
    }
    else
    {
        m_summaryLabel->setText(
            QString("%1 serie(s)    Meilleur : %2    Moyenne : %3")
                .arg(summary.count).arg(summary.best).arg(summary.average, 0, 'f', 1)
        );
    }

    m_historyList->clear();
    int shown = 0;
    for (const ExerciseSession& s : ExerciseSessions::forPlayer(player, m_exerciseName))
    {
        if (++shown > 10)
        {
            break;
        }
        QString dateText = s.date;
        dateText.replace('T', ' ');
        m_historyList->addItem(
            QString("%1  -  %2 points  (%3 rouges)").arg(dateText).arg(s.points).arg(s.reds)
        );
    }
}
