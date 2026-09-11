#include "TestScenarioRunner.h"

#include "GameManager.h"
#include "Match.h"
#include "Frame.h"

TestScenarioRunner::TestScenarioRunner(GameManager& gameManager, QObject* parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    m_timer.setSingleShot(false);
    m_timer.setInterval(800);
    connect(&m_timer, &QTimer::timeout, this, &TestScenarioRunner::playNextStep);
}

void TestScenarioRunner::setScenario(const std::vector<Ball>& balls)
{
    m_scenario = balls;
}

void TestScenarioRunner::start()
{
    m_timer.stop();

    // On repart d'une frame fraiche pour que le scenario soit reproductible
    // a l'identique a chaque relance.
    m_gameManager.getMatch().startNewFrame();

    m_currentStep = 0;

    if (m_scenario.empty())
    {
        return;
    }

    m_timer.start();
}

void TestScenarioRunner::stop()
{
    m_timer.stop();
}

void TestScenarioRunner::setStepDelayMs(int delayMs)
{
    m_timer.setInterval(delayMs);
}

void TestScenarioRunner::playNextStep()
{
    if (m_currentStep >= m_scenario.size())
    {
        m_timer.stop();
        emit scenarioFinished();
        return;
    }

    Frame& frame = m_gameManager.getMatch().getCurrentFrame();

    // Le coup est tente tel quel : s'il est illegal a ce moment
    // (mauvaise bille, mauvais joueur...), le moteur le traite
    // automatiquement comme une faute (voir Frame::playShot).
    frame.playShot(m_scenario[m_currentStep]);

    m_gameManager.afterShot();

    m_currentStep++;

    emit stepPlayed();

    if (m_currentStep >= m_scenario.size())
    {
        m_timer.stop();
        emit scenarioFinished();
    }
}