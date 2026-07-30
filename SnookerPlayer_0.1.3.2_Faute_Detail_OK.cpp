#include "Frame.h"

#include <iostream>

#include "Shot.h"


// =====================================
// Constructeur
// =====================================

Frame::Frame()
{
    m_player1.setName("Eric");
    m_player2.setName("Jean");

    m_currentPlayer = &m_player1;

    m_redsRemaining = 15;

    m_needColor = false;

    m_nextColor = 0;

    m_phase = FramePhase::Reds;
}


// =====================================
// Copie
// =====================================

Frame::Frame(const Frame& other)
{
    *this = other;
}


Frame& Frame::operator=(const Frame& other)
{
    if (this != &other)
    {
        m_player1 = other.m_player1;
        m_player2 = other.m_player2;

        m_redsRemaining = other.m_redsRemaining;
        m_needColor = other.m_needColor;
        m_nextColor = other.m_nextColor;
        m_phase = other.m_phase;

        m_history = other.m_history;


        if (other.m_currentPlayer == &other.m_player2)
        {
            m_currentPlayer = &m_player2;
        }
        else
        {
            m_currentPlayer = &m_player1;
        }
    }

    return *this;
}


// =====================================
// Joueurs
// =====================================

Player& Frame::currentPlayer()
{
    return *m_currentPlayer;
}


Player& Frame::getPlayer1()
{
    return m_player1;
}


Player& Frame::getPlayer2()
{
    return m_player2;
}


void Frame::switchPlayer()
{
    if (m_currentPlayer == &m_player1)
        m_currentPlayer = &m_player2;
    else
        m_currentPlayer = &m_player1;
}


// =====================================
// Gestion rouges
// =====================================

int Frame::redsRemaining() const
{
    return m_redsRemaining;
}


void Frame::potRed()
{
    if (m_redsRemaining <= 0)
        return;


    m_redsRemaining--;

    m_currentPlayer->addPoints(1);

    m_needColor = true;


    if (m_redsRemaining == 0)
    {
        m_phase = FramePhase::LastRedColor;
    }
}


// =====================================
// Gestion couleurs
// =====================================

void Frame::potColor(Ball ball)
{
    m_currentPlayer->addPoints(ball.getValue());


    m_needColor = false;


    if (m_phase == FramePhase::LastRedColor)
    {
        m_phase = FramePhase::FinalColors;
        m_nextColor = 0;
    }
    else if (m_phase == FramePhase::FinalColors)
    {
        m_nextColor++;

        if (m_nextColor >= 6)
        {
            m_phase = FramePhase::Finished;
        }
    }
}
// =====================================
// Etat du jeu
// =====================================

bool Frame::isColorNeeded() const
{
    return m_needColor;
}


bool Frame::isFinished() const
{
    return m_phase == FramePhase::Finished;
}


FramePhase Frame::getPhase() const
{
    return m_phase;
}


// =====================================
// Jeu normal
// =====================================

bool Frame::playShot(const Ball& ball)
{
    Ball required = getRequiredBall();


    if (m_referee.checkContact(required, ball))
    {

        if (ball.getName() == "Rouge")
        {
            potRed();
        }
        else
        {
            potColor(ball);
        }


        Shot shot(
            *m_currentPlayer,
            ball
        );


        m_history.addShot(shot);


        return true;
    }


    int penalty =
        m_referee.calculateFoul(
            required,
            ball
        );


    foul(
        required,
        ball,
        penalty
    );


    return false;
}


bool Frame::playTurn(const Ball& ball)
{
    return playShot(ball);
}


// =====================================
// Coup raté
// =====================================

void Frame::missShot()
{
    std::cout
        << "Coup rate : changement de joueur"
        << std::endl;

    switchPlayer();
}


// =====================================
// Arbitrage manuel
// =====================================

bool Frame::checkShot(
    const Ball& intended,
    const Ball& hit
)
{
    if (intended.getName() == hit.getName())
    {
        return true;
    }


    int penalty = hit.getValue();


    if (intended.getValue() > penalty)
    {
        penalty = intended.getValue();
    }


    foul(
        intended,
        hit,
        penalty
    );


    return false;
}


// =====================================
// Faute
// =====================================

void Frame::foul(
    const Ball& required,
    const Ball& touched,
    int points
)
{
    int penalty = points;


    if (penalty < 4)
    {
        penalty = 4;
    }


    if (m_currentPlayer == &m_player1)
    {
        m_player2.addPoints(penalty);
    }
    else
    {
        m_player1.addPoints(penalty);
    }


    std::cout
        << "Faute : "
        << penalty
        << " points"
        << std::endl;


    m_history.addFoul(
        m_currentPlayer->getName(),
        required.getName(),
        touched.getName(),
        penalty
    );


    switchPlayer();
}
// =====================================
// Couleurs finales
// =====================================

bool Frame::isCorrectFinalColor(const Ball& ball) const
{
    if (m_phase != FramePhase::FinalColors)
    {
        return true;
    }

    return ball.getName() == getNextColorName();
}


std::string Frame::getNextColorName() const
{
    switch (m_nextColor)
    {
    case 0:
        return "Jaune";

    case 1:
        return "Verte";

    case 2:
        return "Marron";

    case 3:
        return "Bleue";

    case 4:
        return "Rose";

    case 5:
        return "Noire";

    default:
        return "Termine";
    }
}


// =====================================
// Bille demandée
// =====================================

Ball Frame::getRequiredBall() const
{
    if (m_needColor)
    {
        return Ball("Couleur", 0);
    }


    if (m_phase == FramePhase::Reds)
    {
        return Ball("Rouge", 1);
    }


    if (m_phase == FramePhase::FinalColors)
    {
        switch (m_nextColor)
        {
        case 0:
            return Ball("Jaune", 2);

        case 1:
            return Ball("Verte", 3);

        case 2:
            return Ball("Marron", 4);

        case 3:
            return Ball("Bleue", 5);

        case 4:
            return Ball("Rose", 6);

        case 5:
            return Ball("Noire", 7);
        }
    }


    return Ball("Aucune", 0);
}


// =====================================
// Affichage
// =====================================

void Frame::displayPhase() const
{
    std::cout
        << "Phase actuelle : ";


    switch (m_phase)
    {
    case FramePhase::Reds:
        std::cout << "Rouges";
        break;

    case FramePhase::LastRedColor:
        std::cout << "Derniere couleur apres rouge";
        break;

    case FramePhase::FinalColors:
        std::cout << "Couleurs finales";
        break;

    case FramePhase::Finished:
        std::cout << "Frame termine";
        break;
    }


    std::cout << std::endl;
}



void Frame::displayStatus() const
{
    std::cout
        << "Joueur : "
        << m_currentPlayer->getName()
        << std::endl;


    std::cout
        << "Score Eric : "
        << m_player1.getScore()
        << std::endl;


    std::cout
        << "Score Jean : "
        << m_player2.getScore()
        << std::endl;


    std::cout
        << "Rouges restantes : "
        << m_redsRemaining
        << std::endl;
}


// =====================================
// Historique
// =====================================

const ShotHistory& Frame::getHistory() const
{
    return m_history;
}