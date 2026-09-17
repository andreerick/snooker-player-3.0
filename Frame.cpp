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
        // m_freeBall/m_freeBallColor manquaient ici : Match::startNewFrame()
        // fait `m_currentFrame = Frame()` pour repartir d'une frame vierge,
        // mais comme ces deux champs n'etaient jamais copies, la nouvelle
        // frame heritait silencieusement de l'etat Free Ball de l'ancienne
        // au lieu de repartir a "aucun Free Ball" (et le constructeur de
        // copie laissait m_freeBall non initialise, comportement indefini).
        m_freeBall = other.m_freeBall;
        m_freeBallColor = other.m_freeBallColor;
        m_freeBallForRed = other.m_freeBallForRed;
        m_touchingBall = other.m_touchingBall;
        m_nextColor = other.m_nextColor;
        m_phase = other.m_phase;
        m_history = other.m_history;
        m_ballSet = other.m_ballSet;

        if (other.m_currentPlayer == &other.m_player2)
        {
            m_currentPlayer = &m_player2;
        }
        else
        {
            m_currentPlayer = &m_player1;
        }

        // m_lastRedPotter pointe vers un joueur de l'AUTRE objet ; il faut
        // le retraduire vers nos propres m_player1/m_player2 (meme piege
        // que m_currentPlayer ci-dessus, sinon pointeur pendouillant).
        if (other.m_lastRedPotter == &other.m_player2)
        {
            m_lastRedPotter = &m_player2;
        }
        else if (other.m_lastRedPotter == &other.m_player1)
        {
            m_lastRedPotter = &m_player1;
        }
        else
        {
            m_lastRedPotter = nullptr;
        }

        // Meme piege que m_currentPlayer/m_lastRedPotter ci-dessus.
        if (other.m_concededBy == &other.m_player2)
        {
            m_concededBy = &m_player2;
        }
        else if (other.m_concededBy == &other.m_player1)
        {
            m_concededBy = &m_player1;
        }
        else
        {
            m_concededBy = nullptr;
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
    // Le tour du joueur actuel se termine : son break repart de zero.
    m_currentPlayer->resetBreak();

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

    // Une couleur devient jouable juste après. Le MEME joueur reste
    // toutefois autorise a empocher une autre rouge d'affilee (frequent
    // sur un seul coup reel, ex. la casse) : voir m_lastRedPotter et le
    // Cas 1 de playShot(), qui ne sanctionne que si c'est UN AUTRE joueur
    // qui tente une rouge alors qu'une couleur est due.
    m_needColor = true;
    m_lastRedPotter = m_currentPlayer;

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
        checkInsurmountableLead();
        if (m_phase == FramePhase::Finished)
        {
            return;
        }
        if (m_nextColor >= 6)
        {
            // Toutes les couleurs finales sont jouees, y compris la
            // noire. En cas d'egalite, la noire est respotee et
            // rejouee (regle officielle du snooker) plutot que de
            // terminer la frame sur un score nul.
            m_phase = (m_player1.getScore() == m_player2.getScore())
                ? FramePhase::BlackReplay
                : FramePhase::Finished;
        }
    }
    else if (m_phase == FramePhase::BlackReplay)
    {
        // La noire rejouee vient d'etre empochee. Si les scores sont
        // encore a egalite (deja arrive en competition), elle est
        // respotee une fois de plus ; sinon la frame est terminee.
        if (m_player1.getScore() != m_player2.getScore())
        {
            m_phase = FramePhase::Finished;
        }
    }
}

// =====================================
// Ecart de points insurmontable (noire seule restante)
// =====================================
void Frame::checkInsurmountableLead()
{
    if (m_phase != FramePhase::FinalColors || m_nextColor != 5)
    {
        return;
    }

    int diff = m_player1.getScore() - m_player2.getScore();
    if (diff > 7 || -diff > 7)
    {
        m_phase = FramePhase::Finished;
    }
}

// =====================================
// Points restants sur la table
// =====================================
int Frame::pointsRemaining() const
{
    // Frame terminee (forcee via forceFinishFrame() ou reellement finie) :
    // plus rien a jouer, quel que soit ce qu'il reste sur la table.
    if (m_phase == FramePhase::Finished)
    {
        return 0;
    }

    int total = 0;

    for (const auto& ball : m_ballSet.getBalls())
    {
        if (ball.getName() == "Blanche")
        {
            continue;
        }

        total += ball.getValue();
    }

    // Pendant la phase des rouges (et la couleur qui suit la derniere
    // rouge), chaque rouge restante peut theoriquement etre suivie
    // d'une noire (7 points) puisque les couleurs sont respotees.
    // Le calcul "points restants" classique du snooker compte donc
    // chaque rouge comme valant 8 points potentiels (1 + 7), et non
    // seulement sa valeur brute de 1 point. C'est ce qui donne 147
    // en tout debut de frame (15 rouges x 8 + 6 couleurs = 147).
    if (m_phase == FramePhase::Reds || m_phase == FramePhase::LastRedColor)
    {
        total += m_redsRemaining * 7;
    }

    return total;
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

    if (m_concededBy != nullptr)
    {
        return (m_concededBy == &m_player1) ? m_player2.getName() : m_player1.getName();
    }

    if (m_player1.getScore() > m_player2.getScore())
    {
        return m_player1.getName();
    }
    if (m_player2.getScore() > m_player1.getScore())
    {
        return m_player2.getName();
    }

    // En jeu normal, une egalite declenche desormais la phase
    // BlackReplay (noire respotee et rejouee) : ce cas n'est donc
    // atteint que si la frame a ete forcee a egalite via
    // forceFinishFrame() (bouton de test "Fin de frame").
    return "Egalite";
}

// =====================================
// Fin de frame forcee (telecommande de test)
// =====================================
bool Frame::forceFinishFrame()
{
    if (m_player1.getScore() == m_player2.getScore())
    {
        return false;
    }

    m_phase = FramePhase::Finished;
    return true;
}

bool Frame::concedeFrame(Player& conceder)
{
    if (m_phase == FramePhase::Finished)
    {
        return false;
    }

    m_concededBy = &conceder;
    m_phase = FramePhase::Finished;
    return true;
}

// =====================================
// Jeu normal
// =====================================
bool Frame::playShot(const Ball& ball)
{
    // Une frame deja terminee ne doit plus rien accepter : sans ce
    // garde, une frame reutilisee apres la fin du match (voir le bug
    // de Match::checkFrameEnd() qui comptait des victoires de frame en
    // double) continuait silencieusement a enregistrer des coups et a
    // marquer des points sur une frame qui n'existe plus pour le match.
    if (m_phase == FramePhase::Finished)
    {
        return false;
    }

    // Touching Ball (voir setTouchingBall()) : consomme immediatement,
    // que ce coup reussisse ou echoue -- ne vaut que pour CE coup precis.
    // Tant que la bille touchante n'est pas la noire rejouee (Cas 0,
    // volontairement non concerne, voir plus bas), plus aucune des
    // verifications "mauvaise bille en premier contact" ci-dessous ne
    // doit s'appliquer : le premier contact est deja repute valide.
    bool touchingBallBypass = m_touchingBall;
    m_touchingBall = false;

    // ---------------------------------------------------
    // Cas 0 : rejeu de la noire (egalite en fin de frame)
    // -> Seule la noire est legale ; toute autre bille est une faute.
    //    (La bille touchante ne s'applique pas ici : a ce stade il ne
    //    reste que la blanche et la noire sur la table, la situation ne
    //    se presente pas en pratique.)
    // ---------------------------------------------------
    if (m_phase == FramePhase::BlackReplay)
    {
        if (ball.getName() != "Noire")
        {
            Ball required = Ball("Noire", 7);
            int penalty = m_referee.calculateFoul(required, ball);
            foul(required, ball, penalty);
            return false;
        }

        potColor(ball);
        m_ballSet.removeBall(ball.getName());
        // Si l'egalite persiste, potColor() reste en BlackReplay : la
        // noire doit alors rester sur la table pour etre respotee.
        if (m_phase == FramePhase::BlackReplay)
        {
            m_ballSet.restoreBall(ball);
        }

        Shot shot(*m_currentPlayer, ball);
        m_history.addShot(shot);
        return true;
    }

    // ---------------------------------------------------
    // Cas 1 : bille ROUGE jouée pendant la phase des rouges
    // -> Legale si une rouge est effectivement due (m_needColor == false),
    //    OU si c'est le MEME joueur qui vient d'empocher la derniere
    //    rouge (m_lastRedPotter) : plusieurs rouges tombent souvent dans
    //    un seul et meme coup reel (la casse en particulier), et l'appli
    //    n'a pas de notion de "coup" distincte du clic sur une bille,
    //    donc on autorise ce cas plutot que de forcer un mode special.
    //    En revanche, si c'est un AUTRE joueur qui tente une rouge alors
    //    qu'une couleur est due, c'est bien une faute (meme regle que le
    //    Cas 2 ci-dessous pour une couleur jouee hors de propos) : deux
    //    joueurs differents ne peuvent jamais partager un seul coup.
    // ---------------------------------------------------
    if (ball.getName() == "Rouge" && m_phase != FramePhase::FinalColors)
    {
        if (m_needColor && m_currentPlayer != m_lastRedPotter && !touchingBallBypass)
        {
            Ball required = getRequiredBall();
            int penalty = m_referee.calculateFoul(required, ball);
            foul(required, ball, penalty);
            return false;
        }

        if (m_redsRemaining <= 0 && !touchingBallBypass)
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
    //    (m_needColor == true). Sinon : faute (deux couleurs de suite).
    // ---------------------------------------------------
    if (m_phase != FramePhase::FinalColors && !m_needColor && !touchingBallBypass)
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
    if (m_phase == FramePhase::FinalColors && !isCorrectFinalColor(ball) && !touchingBallBypass)
    {
        Ball required = getRequiredBall();
        int penalty = m_referee.calculateFoul(required, ball);
        foul(required, ball, penalty);
        return false;
    }

    // ---------------------------------------------------
    // Coup légal : la couleur compte.
    // - Pendant la phase des rouges (Reds, entre deux rouges) et
    //   juste après la toute dernière rouge (LastRedColor) : la
    //   couleur est respotée comme n'importe quelle couleur suivant
    //   une rouge, elle reste donc sur la table.
    // - Pendant les couleurs finales (FinalColors) uniquement : la
    //   couleur est retirée définitivement de la table.
    // ---------------------------------------------------
    bool shouldRespot = (m_phase == FramePhase::Reds || m_phase == FramePhase::LastRedColor);

    potColor(ball);

    m_ballSet.removeBall(ball.getName());

    // La noire respotee (egalite en fin de frame, voir potColor()) doit
    // elle aussi rester sur la table, meme si ce n'etait pas le cas
    // avant ce coup (phase FinalColors ne respote normalement pas).
    if (shouldRespot || m_phase == FramePhase::BlackReplay)
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
    // Voir le meme garde dans playShot() : une frame terminee ne doit
    // plus rien accepter.
    if (m_phase == FramePhase::Finished)
    {
        return false;
    }

    // Meme raisonnement que dans missShot() : ne doit pas survivre a ce
    // coup, meme si ce n'est pas playShot() qui le traite.
    m_touchingBall = false;

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
        // Motif explicite (au lieu du defaut "Mauvaise bille touchee") :
        // permet a parseScenarioFile() de distinguer sans ambiguite une
        // faute survenue PENDANT un Free Ball d'une faute ordinaire portant
        // sur les memes billes (voir Kind::FreeBallFoul dans MainWindow.cpp).
        foul(
            m_freeBallColor,
            ball,
            penalty,
            "Faute pendant un Free Ball"
        );

        // Le Free Ball ne concernait que ce coup precis : qu'il soit reussi
        // ou faute, l'obligation ne doit jamais se reporter sur le joueur
        // suivant (foul() a deja change de joueur juste au-dessus).
        m_freeBall = false;
        m_freeBallColor = Ball("Aucune", 0);

        return false;
    }

    // Le Free Ball compte toujours 1 point (comme une rouge, valeur non
    // modifiee ici), mais la SUITE de la partie doit correspondre a ce
    // qu'il remplace reellement (capture par setFreeBall(), voir
    // m_freeBallForRed), sinon la partie peut rester bloquee : une rouge
    // qui ne fait jamais avancer la sequence des couleurs finales, ou une
    // couleur qui laisse a tort une autre couleur due juste apres.
    // Respotee ou retiree definitivement selon la phase AU MOMENT du
    // coup (avant toute transition ci-dessous), comme pour un coup normal.
    bool shouldRespot = (m_phase == FramePhase::Reds || m_phase == FramePhase::LastRedColor);

    if (m_phase == FramePhase::BlackReplay)
    {
        // Cas exotique (Free Ball pendant la noire rejouee) : pas de
        // transition de phase a gerer ici, voir Cas 0 de playShot().
    }
    else if (m_freeBallForRed)
    {
        // Remplace une rouge : une couleur devient due ensuite.
        m_needColor = true;
    }
    else if (m_phase == FramePhase::FinalColors)
    {
        // Remplace la couleur specifique due dans la sequence des
        // couleurs finales : fait avancer la sequence comme un coup
        // normal (voir potColor()), sinon la partie resterait bloquee
        // sur cette couleur.
        m_nextColor++;
        if (m_nextColor >= 6)
        {
            m_phase = (m_player1.getScore() == m_player2.getScore())
                ? FramePhase::BlackReplay
                : FramePhase::Finished;
        }
    }
    else
    {
        // Remplace "une couleur" (n'importe laquelle etait due) pendant
        // la phase des rouges : une rouge redevient due ensuite.
        m_needColor = false;
        if (m_phase == FramePhase::LastRedColor)
        {
            m_phase = FramePhase::FinalColors;
            m_nextColor = 0;
        }
    }

    // Compte pour la valeur de la bille NORMALEMENT due (1 pour une
    // rouge, ou la vraie valeur de la couleur si les couleurs finales
    // etaient dues), pas forcement 1 -- voir m_freeBallValueBall, deduite
    // automatiquement a l'armement ou annoncee explicitement si ambigu
    // (voir isFreeBallValueAmbiguous()/setFreeBallValue()).
    int value = m_freeBallValueBall.getValue();
    m_currentPlayer->addFreeBallPoints(value);

    // Synchronisation avec la table reelle (voir shouldRespot ci-dessus) :
    // manquait entierement avant, ce qui faussait "points restants sur la
    // table" des qu'un Free Ball etait joue.
    m_ballSet.removeBall(ball.getName());
    if (shouldRespot)
    {
        m_ballSet.restoreBall(ball);
    }

    std::cout
        << "Free Ball : "
        << ball.getName()
        << " compte pour "
        << value
        << " point(s)"
        << std::endl;

    // Le journal (et donc l'annonce vocale, voir announceNewEvents() qui
    // additionne Shot::getPoints() pour le break en cours) doit refleter
    // la valeur REELLEMENT comptee (value, ci-dessus), pas la valeur
    // propre de la bille physiquement jouee (ball.getValue()) -- sinon
    // un Free Ball joue avec la Bleue (5) mais compte pour une rouge (1)
    // ferait annoncer "5 points" au lieu de "1 point". Le NOM affiche
    // reste celui de la bille reellement jouee.
    Ball loggedBall(ball.getName(), value);
    Shot shot(
        *m_currentPlayer,
        loggedBall
    );
    m_history.addShot(shot);

    // Fin du Free Ball
    m_freeBall = false;
    m_freeBallColor =
        Ball("Aucune", 0);

    // Cas exotique (Free Ball pendant les couleurs finales, voir plus
    // haut) : verifie apres coup, une fois les points reellement ajoutes.
    checkInsurmountableLead();

    return true;
}

// =====================================
// Coup raté
// =====================================
void Frame::missShot()
{
    // Voir le meme garde dans playShot() : une frame terminee ne doit
    // plus rien accepter.
    if (m_phase == FramePhase::Finished)
    {
        return;
    }

    // Une "bille touchante" armee ne vaut que pour le tout prochain coup
    // (voir setTouchingBall()) : un Miss consomme ce coup comme n'importe
    // quel autre, elle ne doit pas survivre au joueur suivant.
    m_touchingBall = false;

    std::cout
        << "Coup rate : changement de joueur"
        << std::endl;
    // Enregistre au journal AVANT de changer de joueur, pour que le
    // scenario reste rejouable fidelement (voir "Rejouer le scenario"
    // dans la telecommande) : sans cette ligne, un changement de tour
    // sans faute serait invisible dans le journal exporte.
    m_history.addMiss(m_currentPlayer->getName());
    switchPlayer();

    // Regle confirmee par l'utilisateur (pas la regle officielle stricte,
    // volontairement differente) : si un joueur empoche une rouge puis
    // rate sa couleur (Miss), le joueur suivant reprend TOUJOURS par une
    // rouge tant qu'il en reste sur la table -- il n'herite pas de
    // l'obligation de couleur laissee par le joueur precedent. Ne
    // s'applique que pendant la phase des rouges : une fois les rouges
    // epuisees (FinalColors), la sequence des couleurs finales reste
    // stricte et n'est pas concernee par cette regle.
    if (m_phase == FramePhase::Reds || m_phase == FramePhase::LastRedColor)
    {
        m_needColor = false;
    }
}

// =====================================
// Faute
// =====================================
void Frame::foul(
    const Ball& required,
    const Ball& touched,
    int points,
    const std::string& reason
)
{
    // Voir le meme garde dans playShot() : une frame terminee ne doit
    // plus rien accepter.
    if (m_phase == FramePhase::Finished)
    {
        return;
    }

    // Une faute declaree manuellement par l'arbitre (poussé de bille sur
    // la bille touchante, par exemple) consomme aussi ce coup : voir
    // missShot() pour le meme raisonnement.
    m_touchingBall = false;

    int penalty = points;
    if (penalty < 4)
    {
        penalty = 4;
    }

    std::string foulPlayer =
        m_currentPlayer->getName();

    if (m_currentPlayer == &m_player1)
    {
        m_player2.addPenalty(penalty);
    }
    else
    {
        m_player1.addPenalty(penalty);
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
        reason,
        penalty
    );

    // Une faute termine le tour : le prochain joueur
    // peut à nouveau jouer une rouge ou une couleur
    // selon l'état réel de la table.
    m_needColor = false;

    switchPlayer();

    // Regle speciale de "mort subite" pour la noire respotee (egalite
    // apres la derniere couleur, voir potColor()) : contrairement a une
    // faute normale (qui donne juste des points et continue), TOUTE
    // faute a ce stade fait perdre la frame sur-le-champ a son auteur --
    // la noire n'est pas rejouee une fois de plus dans ce cas.
    if (m_phase == FramePhase::BlackReplay)
    {
        m_phase = FramePhase::Finished;
    }

    // Une faute pendant que la noire est deja la seule bille restante
    // peut, elle aussi, creuser un ecart insurmontable (voir potColor()).
    checkInsurmountableLead();
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
    if (m_phase == FramePhase::BlackReplay)
    {
        return Ball("Noire", 7);
    }

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
    case FramePhase::BlackReplay:
        std::cout << "Noire rejouee (egalite)";
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

    if (value)
    {
        // Capture ce que le Free Ball remplace, au moment ou il est arme :
        // determine la suite (rouge ou couleur due ensuite) et si la
        // sequence des couleurs finales doit avancer, exactement comme si
        // la bille reellement due avait ete jouee normalement (voir
        // playFreeBall()).
        m_freeBallForRed =
            (m_phase == FramePhase::Reds || m_phase == FramePhase::LastRedColor) && !m_needColor;

        // Deduit la valeur a compter (voir m_freeBallValueBall) de la
        // bille normalement due. Non ambigu dans tous les cas SAUF quand
        // "n'importe quelle couleur" est legale (getRequiredBall() renvoie
        // alors "Couleur", valeur 0) : dans ce cas on garde la derniere
        // valeur connue en attendant l'annonce explicite (voir
        // isFreeBallValueAmbiguous()/setFreeBallValue()).
        Ball required = getRequiredBall();
        if (required.getName() != "Couleur")
        {
            m_freeBallValueBall = required;
        }
    }
}

bool Frame::isFreeBallValueAmbiguous() const
{
    return getRequiredBall().getName() == "Couleur";
}

void Frame::setFreeBallValue(const Ball& ball)
{
    m_freeBallValueBall = ball;
}

bool Frame::isFreeBall() const
{
    return m_freeBall;
}

// =====================================
// Touching Ball
// =====================================
void Frame::setTouchingBall(bool value, const std::string& ballName)
{
    m_touchingBall = value;

    // Enregistre uniquement l'armement (pas la desactivation silencieuse
    // en debut de coup, voir playShot()/missShot()/playFreeBall()/foul()) :
    // sinon le journal serait rempli d'entrees "desarmee" a chaque coup,
    // qu'il soit ou non concerne par une bille touchante.
    if (value)
    {
        m_history.addTouchingBall(m_currentPlayer->getName(), ballName);
    }
}

bool Frame::isTouchingBall() const
{
    return m_touchingBall;
}

// =====================================
// Faire rejouer
// =====================================
void Frame::requestReplay()
{
    switchPlayer();
    m_history.addReplay(m_currentPlayer->getName());
}

void Frame::logCorrection(const std::string& before, const std::string& after)
{
    m_history.addCorrection(before, after);
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