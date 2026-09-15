#include "MoveLogWidget.h"

#include "ShotHistory.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QString>
#include <QColor>

namespace
{
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#22c55e";
    const QString kOrange = "#f5a623";
    const QString kRed = "#ef4444";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";
}

MoveLogWidget::MoveLogWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    QLabel* title = new QLabel("JOURNAL DES COUPS", this);
    title->setStyleSheet(
        "color: " + kGray + "; font-size: 11px; letter-spacing: 1px;"
    );
    layout->addWidget(title);

    m_list = new QListWidget(this);
    m_list->setStyleSheet(
        "QListWidget {"
        "  background-color: " + kPanel + ";"
        "  border: 1px solid " + kBorder + ";"
        "  border-radius: 5px;"
        "  color: " + kWhite + ";"
        "  font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 4px 8px;"
        "  border-bottom: 1px solid " + kBorder + ";"
        "}"
    );
    m_list->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_list);
}

void MoveLogWidget::refresh(const ShotHistory& history)
{
    m_list->clear();

    const std::vector<LogEntry>& log = history.getLog();

    for (size_t i = 0; i < log.size(); i++)
    {
        const LogEntry& entry = log[i];

        QString text;
        QString color;

        if (entry.type == LogEntry::Type::Shot)
        {
            text =
                QString::number(i + 1) + ". "
                + QString::fromStdString(entry.playerName)
                + " — "
                + QString::fromStdString(entry.ballName)
                + " (+" + QString::number(entry.points) + ")";
            color = kGreen;
        }
        else if (entry.type == LogEntry::Type::Miss)
        {
            text =
                QString::number(i + 1) + ". "
                + QString::fromStdString(entry.playerName)
                + " — Fin de break (aucune bille jouee)";
            color = kGray;
        }
        else if (entry.type == LogEntry::Type::TouchingBall)
        {
            text =
                QString::number(i + 1) + ". "
                + QString::fromStdString(entry.playerName)
                + " — BILLE TOUCHANTE (premier contact deja valide pour le prochain coup)";
            color = kGray;
        }
        else if (entry.type == LogEntry::Type::Replay)
        {
            text =
                QString::number(i + 1) + ". "
                + QString::fromStdString(entry.playerName)
                + " — REMETTRE EN PLACE (rejoue depuis la position)";
            color = kGray;
        }
        else
        {
            text =
                QString::number(i + 1) + ". FAUTE — "
                + QString::fromStdString(entry.playerName)
                + " : "
                + QString::fromStdString(entry.reason)
                + " (bille demandee : "
                + QString::fromStdString(entry.requiredBall)
                + ", bille jouee : "
                + QString::fromStdString(entry.touchedBall)
                + ") — adverse +"
                + QString::number(entry.foulPoints);
            color = kRed;
        }

        QListWidgetItem* item = new QListWidgetItem(text);
        item->setForeground(QColor(color));
        m_list->addItem(item);
    }

    m_list->scrollToBottom();
}