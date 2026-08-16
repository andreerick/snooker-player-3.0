#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QFrame>
#include "GameManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void refreshDisplay();

    GameManager m_gameManager;

    QFrame* m_player1Frame = nullptr;
    QFrame* m_player2Frame = nullptr;

    QLabel* m_player1Label = nullptr;
    QLabel* m_player2Label = nullptr;
    QLabel* m_redsLabel = nullptr;
    QLabel* m_phaseLabel = nullptr;
};
