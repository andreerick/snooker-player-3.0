#include "Frame.h"

#include <iostream>
#include <string>

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
    {
        m_currentPlayer = &m_player2;
    }
    else
    {
        m_currentPlayer = &m_player1;
    }
}





// =====================================
// Gestion des rouges
// =====================================

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



    // Après une rouge,
    // il faut une couleur

    m_needColor = true;



    if (m_redsRemaining == 0)
    {
        m_phase = FramePhase::LastRedColor;
    }
}





// =====================================
// Gestion des couleurs
// =====================================

void Frame::potColor(Ball ball)
{

    if (m_needColor)
    {

        m_currentPlayer->addPoints(
            ball.getValue()
        );


        m_needColor = false;



        if (m_phase == FramePhase::LastRedColor)
        {
            m_phase = FramePhase::FinalColors;

            m_nextColor = 0;
        }


        return;
    }





    if (m_phase == FramePhase::FinalColors)
    {

        m_currentPlayer->addPoints(
            ball.getValue()
        );


        m_nextColor++;



        if (m_nextColor >= 6)
        {
            m_phase = FramePhase::Finished;
        }
    }

}
// =====================================
// Jeu normal
// =====================================

bool Frame::playShot(
    const Ball& ball
)
{

    Ball requiredBall = getRequiredBall();



    // Contrôle arbitre

    if (m_referee.checkContact(
        requiredBall,
        ball
    ))
    {

        if (ball.getName() == "Rouge")
        {
            potRed();
        }
        else
        {
            potColor(ball);
        }




        std::cout
            << "SHOT DEBUT"
            << std::endl;



        Shot shot(
            *m_currentPlayer,
            ball
        );



        m_history.addShot(shot);



        m_history.displayHistory();



        return true;
    }





    // ==============================
    // Faute
    // ==============================

    int penalty =
        m_referee.calculateFoul(
            requiredBall,
            ball
        );



    foul(penalty);



    return false;
}





bool Frame::playTurn(
    const Ball& ball
)
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
// Contrôle manuel arbitre
// =====================================

bool Frame::checkShot(
    const Ball& intended,
    const Ball& hit
)
{

    if (intended.getName() == hit.getName())
    {
        std::cout
            << "Coup valide"
            << std::endl;


        return true;
    }



    int penalty =
        hit.getValue();



    if (intended.getValue() > penalty)
    {
        penalty = intended.getValue();
    }



    foul(penalty);



    return false;
}





// =====================================
// Faute
// =====================================

void Frame::foul(
    int points
)
{

    int penalty = points;



    if (penalty < 4)
    {
        penalty = 4;
    }




    // Attribution des points
    // à l'adversaire

    if (m_currentPlayer == &m_player1)
    {
        m_player2.addPoints(penalty);
    }
    else
    {
        m_player1.addPoints(penalty);
    }





    // =================================
    // Enregistrement arbitre
    // =================================

    std::string playerName =
        m_currentPlayer->getName();



    Ball required =
        getRequiredBall();



    // Pour l'instant nous mémorisons
    // la bille demandée.
    // La bille jouée sera ajoutée
    // avec la prochaine évolution
    // de l'objet FoulRecord.


    m_history.addFoul(
        playerName,
        required.getName(),
        "Inconnue",
        penalty
    );





    std::cout
        << "Faute : "
        << penalty
        << " points pour l'adversaire"
        << std::endl;




    switchPlayer();

}
// =====================================
// Couleurs finales
// =====================================

bool Frame::isCorrectFinalColor(
    const Ball& ball
) const
{
    if (m_phase != FramePhase::FinalColors)
    {
        return true;
    }


    return ball.getName() == getNextColorName();
}




// =====================================
// Nom de la prochaine couleur
// =====================================

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

    // Après une rouge,
    // une couleur est obligatoire

    if (m_needColor)
    {
        return Ball(
            "Couleur",
            0
        );
    }



    // Phase des rouges

    if (m_phase == FramePhase::Reds)
    {
        return Ball(
            "Rouge",
            1
        );
    }



    // Dernière rouge + couleur

    if (m_phase == FramePhase::LastRedColor)
    {
        return Ball(
            "Couleur",
            0
        );
    }



    // Couleurs finales

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



    return Ball(
        "Aucune",
        0
    );
}





// =====================================
// Affichage phase
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





// =====================================
// Affichage état du frame
// =====================================

void Frame::displayStatus() const
{
    std::cout
        << std::endl;

    std::cout
        << "========================"
        << std::endl;

    std::cout
        << "      SNOOKER PLAYER"
        << std::endl;

    std::cout
        << "========================"
        << std::endl;


    std::cout
        << "Joueur : "
        << m_currentPlayer->getName()
        << std::endl;


    std::cout
        << "Score "
        << m_player1.getName()
        << " : "
        << m_player1.getScore()
        << std::endl;


    std::cout
        << "Score "
        << m_player2.getName()
        << " : "
        << m_player2.getScore()
        << std::endl;


    std::cout
        << "Break actuel : "
        << m_currentPlayer->getBreak()
        << std::endl;


    std::cout
        << "Rouges restantes : "
        << m_redsRemaining
        << std::endl;


    std::cout
        << "Bille demandee : "
        << getRequiredBall().getName()
        << std::endl;


    std::cout
        << "========================"
        << std::endl;
}





// =====================================
// Historique
// =====================================

const ShotHistory& Frame::getHistory() const
{
    return m_history;
}

// =====================================
// Operateur de copie
// =====================================

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



        m_currentPlayer = &m_player1;


        if (other.m_currentPlayer == &other.m_player2)
        {
            m_currentPlayer = &m_player2;
        }
    }


    return *this;
}
