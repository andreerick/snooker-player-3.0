#pragma once

#include <QObject>
#include <QTimer>
#include <vector>

#include "Ball.h"

class GameManager;

// Rejoue automatiquement une serie de coups predefinie sur le vrai moteur
// de jeu (GameManager/Frame), un coup a la fois, avec un delai reglable
// entre chaque coup. Utilise pour tester visuellement l'affichage Qt
// (scores, phase, journal des coups) sans avoir a cliquer coup par coup.
class TestScenarioRunner : public QObject
{
    Q_OBJECT

public:
    explicit TestScenarioRunner(GameManager& gameManager, QObject* parent = nullptr);

    // Definit la liste des billes a jouer dans l'ordre. Chaque bille est
    // tentee via Frame::playShot ; si elle est illegale a ce moment,
    // le moteur la traite automatiquement comme une faute.
    void setScenario(const std::vector<Ball>& balls);

    // Demarre (ou relance depuis le debut) le rejeu du scenario courant,
    // sur une frame fraichement reinitialisee.
    void start();

    // Arrete le rejeu en cours, sans reinitialiser la frame.
    void stop();

    // Regle le delai entre deux coups, en millisecondes.
    void setStepDelayMs(int delayMs);

signals:
    // Emis apres chaque coup joue (legal ou faute), pour que l'UI
    // puisse rafraichir l'affichage.
    void stepPlayed();

    // Emis quand tous les coups du scenario ont ete joues.
    void scenarioFinished();

private slots:
    void playNextStep();

private:
    GameManager& m_gameManager;
    QTimer m_timer;
    std::vector<Ball> m_scenario;
    size_t m_currentStep = 0;
};