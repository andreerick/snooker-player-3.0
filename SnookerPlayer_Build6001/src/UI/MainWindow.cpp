#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QFont>

namespace
{
    const QString kBg = "#0b0c0e";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#22c55e";
    const QString kOrange = "#f5a623";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Snooker Player");
    resize(1200, 700);

    setStyleSheet("background-color: " + kBg + ";");

    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setSpacing(14);
    layout->setContentsMargins(24, 24, 24, 24);

    QFont scoreFont;
    scoreFont.setPointSize(22);
    scoreFont.setBold(true);

    m_player1Frame = new QFrame(central);
    m_player2Frame = new QFrame(central);

    QVBoxLayout* p1Layout = new QVBoxLayout(m_player1Frame);
    QVBoxLayout* p2Layout = new QVBoxLayout(m_player2Frame);

    m_player1Label = new QLabel(m_player1Frame);
    m_player1Label->setFont(scoreFont);
    m_player1Label->setStyleSheet("color: " + kGreen + ";");
    p1Layout->addWidget(m_player1Label);

    m_player2Label = new QLabel(m_player2Frame);
    m_player2Label->setFont(scoreFont);
    m_player2Label->setStyleSheet("color: " + kOrange + ";");
    p2Layout->addWidget(m_player2Label);

    m_redsLabel = new QLabel(central);
    m_redsLabel->setStyleSheet("color: " + kWhite + "; font-size: 13px;");

    m_phaseLabel = new QLabel(central);
    m_phaseLabel->setStyleSheet("color: " + kGray + "; font-size: 11px; letter-spacing: 1px;");

    layout->addWidget(m_player1Frame);
    layout->addWidget(m_player2Frame);
    layout->addWidget(m_redsLabel);
    layout->addWidget(m_phaseLabel);
    layout->addStretch();

    setCentralWidget(central);

    m_gameManager.startMatch();
    refreshDisplay();
}

void MainWindow::refreshDisplay()
{
    Frame& frame = m_gameManager.getMatch().getCurrentFrame();

    bool p1Active = (&frame.currentPlayer() == &frame.getPlayer1());

    QString activeStyle1 =
        p1Active
        ? "background-color: rgba(34,197,94,0.12); border: 1px solid " + kGreen + "; border-radius: 5px; padding: 10px;"
        : "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 5px; padding: 10px;";

    QString activeStyle2 =
        !p1Active
        ? "background-color: rgba(245,166,35,0.12); border: 1px solid " + kOrange + "; border-radius: 5px; padding: 10px;"
        : "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 5px; padding: 10px;";

    m_player1Frame->setStyleSheet(activeStyle1);
    m_player2Frame->setStyleSheet(activeStyle2);

    m_player1Label->setText(
        QString::fromStdString(frame.getPlayer1().getName())
        + " : "
        + QString::number(frame.getPlayer1().getScore())
    );

    m_player2Label->setText(
        QString::fromStdString(frame.getPlayer2().getName())
        + " : "
        + QString::number(frame.getPlayer2().getScore())
    );

    m_redsLabel->setText(
        "Rouges restantes : " + QString::number(frame.redsRemaining())
    );

    QString phaseText;
    switch (frame.getPhase())
    {
    case FramePhase::Reds:
        phaseText = "ROUGES";
        break;
    case FramePhase::LastRedColor:
        phaseText = "DERNIERE COULEUR APRES ROUGE";
        break;
    case FramePhase::FinalColors:
        phaseText = "COULEURS FINALES";
        break;
    case FramePhase::Finished:
        phaseText = "FRAME TERMINEE";
        break;
    }
    m_phaseLabel->setText("PHASE : " + phaseText);
}
