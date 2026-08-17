#include "Frame.h"
#include <iostream>
#include "Shot.h"

Frame::Frame()
{
    m_player1.setName("Joueur 1");
    m_player2.setName("Joueur 2");
    m_currentPlayer = &m_player1;
    m_redsRemaining = 15;
    m_needColor = false;
    m_freeBall = false;
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

void Frame::setPlayerNames(const std::string& name1, const std::string& name2)
{
    m_player1.setName(name1);
    m_player2.setName(name2);
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

    // Synchronisation avec la table réelle
    m_ballSet.removeBall("Rouge");

    m_currentPlayer->addPoints(1);

    // Une couleur devient jouable juste après,
    // mais une autre rouge reste toujours autorisée aussi
    // (plusieurs rouges d'affilée sont légales au snooker).
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

std::string Frame::getWinnerName() const
{
    if (!isFinished())
    {
        return "";
    }

    if (m_player1.getScore() > m_player2.getScore())
    {
        return m_player1.getName();
    }
    if (m_player2.getScore() > m_player1.getScore())
    {
        return m_player2.getName();
    }

    // Egalite en fin de frame (cas de la noire a rejouer) :
    // pas encore gere, a traiter separement.
    return "Egalite";
}

// =====================================
// Jeu normal
// =====================================
bool Frame::playShot(const Ball& ball)
{
    // ---------------------------------------------------
    // Cas 1 : bille ROUGE jouée pendant la phase des rouges
    // -> Toujours légale tant qu'il reste des rouges sur la table.
    //    (Empocher plusieurs rouges d'affilée est autorisé,
    //     ce n'est jamais une faute.)
    // ---------------------------------------------------
    if (ball.getName() == "Rouge" && m_phase != FramePhase::FinalColors)
    {
        if (m_redsRemaining <= 0)
        {
            // Il n'y a plus de rouge sur la table : faute.
            Ball required = getRequiredBall();
            int penalty = m_referee.calculateFoul(required, ball);
            foul(required, ball, penalty);
            return false;
        }

        potRed();

        Shot shot(*m_currentPlayer, ball);
        m_history.addShot(shot);
        return true;
    }

    // ---------------------------------------------------
    // Cas 2 : bille de COULEUR jouée pendant la phase des rouges
    // -> Légale UNIQUEMENT si la bille juste avant était une rouge
    //    (m_needColor == true). Sinon : faute (rouge attendue).
    // ---------------------------------------------------
    if (m_phase != FramePhase::FinalColors && !m_needColor)
    {
        Ball required = Ball("Rouge", 1);
        int penalty = m_referee.calculateFoul(required, ball);
        foul(required, ball, penalty);
        return false;
    }

    // ---------------------------------------------------
    // Cas 3 : phase des couleurs finales
    // -> La couleur doit respecter l'ordre (Jaune, Verte, Marron,
    //    Bleue, Rose, Noire). Sinon : faute (bille respotée).
    // ---------------------------------------------------
    if (m_phase == FramePhase::FinalColors && !isCorrectFinalColor(ball))
    {
        Ball required = getRequiredBall();
        int penalty = m_referee.calculateFoul(required, ball);
        foul(required, ball, penalty);
        return false;
    }

    // ---------------------------------------------------
    // Coup légal : la couleur compte et la bille est retirée.
    // En phase des rouges (Reds), les couleurs sont remises sur
    // la table après avoir été empochées (re-spot).
    // En phase finale (FinalColors), elles sont définitivement
    // retirées de la table.
    // ---------------------------------------------------
    FramePhase phaseBefore = m_phase;
    potColor(ball);
    m_ballSet.removeBall(ball.getName());

    // Re-spot : en phase des rouges, la couleur revient sur la table
    if (phaseBefore == FramePhase::Reds)
    {
        m_ballSet.restoreBall(ball);
    }

    Shot shot(*m_currentPlayer, ball);
    m_history.addShot(shot);
    return true;
}

bool Frame::playTurn(const Ball& ball)
{
    return playShot(ball);
}

// =====================================
// Gestion Free Ball
// =====================================
bool Frame::playFreeBall(const Ball& ball)
{
    if (!m_freeBall)
    {
        return false;
    }

    // Vérification de la couleur choisie
    if (ball.getName() != m_freeBallColor.getName())
    {
        int penalty =
            m_referee.calculateFoul(
                m_freeBallColor,
                ball
            );
        foul(
            m_freeBallColor,
            ball,
            penalty
        );
        return false;
    }

    // La couleur Free Ball compte comme une rouge
    m_currentPlayer->addPoints(1);

    std::cout
        << "Free Ball : "
        << ball.getName()
        << " compte pour 1 point"
        << std::endl;

    Shot shot(
        *m_currentPlayer,
        ball
    );
    m_history.addShot(shot);

    // Fin du Free Ball : le joueur doit ensuite jouer une couleur
    m_freeBall = false;
    m_freeBallColor =
        Ball("Aucune", 0);
    m_needColor = true;

    return true;
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

    std::string foulPlayer =
        m_currentPlayer->getName();

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
        foulPlayer,
        required.getName(),
        touched.getName(),
        "Mauvaise bille touchee",
        penalty
    );

    // Une faute termine le tour : le prochain joueur
    // peut à nouveau jouer une rouge ou une couleur
    // selon l'état réel de la table.
    m_needColor = false;

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
    if (m_phase == FramePhase::Reds || m_phase == FramePhase::LastRedColor)
    {
        if (m_needColor)
        {
            // Une couleur est jouable, mais une rouge
            // resterait également légale (voir playShot).
            return Ball("Couleur", 0);
        }
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
        << "Rouges restantes : "
        << m_redsRemaining
        << std::endl;
}

// =====================================
// Free Ball
// =====================================
void Frame::setFreeBall(bool value)
{
    m_freeBall = value;
}

bool Frame::isFreeBall() const
{
    return m_freeBall;
}

// =====================================
// Free Ball couleur
// =====================================
void Frame::setFreeBallColor(const Ball& ball)
{
    m_freeBallColor = ball;
}

Ball Frame::getFreeBallColor() const
{
    return m_freeBallColor;
}

// =====================================
// Historique
// =====================================
const ShotHistory& Frame::getHistory() const
{
    return m_history;
}

// =====================================
// Table de jeu
// =====================================
const BallSet& Frame::getBallSet() const
{
    return m_ballSet;
}
