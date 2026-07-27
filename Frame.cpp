#include "Frame.h"

#include <iostream>

#include "Shot.h"



Frame::Frame()
{
    m_currentPlayer = &m_player1;

    m_redsRemaining = 15;

    m_needColor = false;

    m_nextColor = 0;

    m_phase = FramePhase::Reds;
}



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
    {
        m_currentPlayer = &m_player2;
    }
    else
    {
        m_currentPlayer = &m_player1;
    }
}



int Frame::redsRemaining() const
{
    return m_redsRemaining;
}



void Frame::potRed()
{
    if (m_redsRemaining <= 0)
    {
        return;
    }


    m_redsRemaining--;

    m_currentPlayer->addPoints(1);


    // Après une rouge il faut une couleur
    m_needColor = true;


    // Dernière rouge jouée
    if (m_redsRemaining == 0)
    {
        m_phase = FramePhase::LastRedColor;
    }
}



void Frame::potColor(Ball ball)
{
    if (m_needColor)
    {
        m_currentPlayer->addPoints(ball.getValue());

        m_needColor = false;


        // Couleur après la dernière rouge
        if (m_phase == FramePhase::LastRedColor)
        {
            m_phase = FramePhase::FinalColors;

            m_nextColor = 0;
        }

        return;
    }



    if (m_phase == FramePhase::FinalColors)
    {
        m_currentPlayer->addPoints(ball.getValue());

        m_nextColor++;


        if (m_nextColor >= 6)
        {
            m_phase = FramePhase::Finished;
        }
    }
}



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
bool Frame::playShot(const Ball& ball)
{
    bool validShot = false;


    if (ball.getName() == "Rouge")
    {
        if (!m_needColor && m_phase == FramePhase::Reds)
        {
            validShot = true;

            potRed();
        }
    }
    else
    {
        if (m_needColor || m_phase == FramePhase::FinalColors)
        {
            if (isCorrectFinalColor(ball))
            {
                validShot = true;

                potColor(ball);
            }
        }
    }



    if (validShot)
    {
        Shot shot(*m_currentPlayer, ball);

        m_history.addShot(shot);

        return true;
    }


    std::cout << "Coup interdit !" << std::endl;

    return false;
}



bool Frame::playTurn(const Ball& ball)
{
    return playShot(ball);
}




void Frame::missShot()
{
    std::cout << "Coup rate : changement de joueur"
        << std::endl;

    switchPlayer();
}





bool Frame::checkShot(const Ball& intended, const Ball& hit)
{
    if (intended.getName() == hit.getName())
    {
        std::cout << "Coup valide"
            << std::endl;

        return true;
    }


    int penalty = hit.getValue();


    if (intended.getValue() > penalty)
    {
        penalty = intended.getValue();
    }


    foul(penalty);


    return false;
}




void Frame::foul(int points)
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
        << " points pour l'adversaire"
        << std::endl;


    switchPlayer();
}






bool Frame::isCorrectFinalColor(const Ball& ball) const
{
    if (m_phase != FramePhase::FinalColors)
    {
        return true;
    }


    switch (m_nextColor)
    {
    case 0:
        return ball.getName() == "Jaune";

    case 1:
        return ball.getName() == "Verte";

    case 2:
        return ball.getName() == "Marron";

    case 3:
        return ball.getName() == "Bleue";

    case 4:
        return ball.getName() == "Rose";

    case 5:
        return ball.getName() == "Noire";

    default:
        return false;
    }
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






Ball Frame::getRequiredBall() const
{
    if (m_phase == FramePhase::Reds)
    {
        return Ball("Rouge", 1);
    }


    if (m_needColor)
    {
        return Ball("Couleur", 0);
    }


    if (m_phase == FramePhase::LastRedColor)
    {
        return Ball("Couleur", 0);
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






void Frame::displayPhase() const
{
    std::cout << "Phase actuelle : ";


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






const ShotHistory& Frame::getHistory() const
{
    return m_history;
}