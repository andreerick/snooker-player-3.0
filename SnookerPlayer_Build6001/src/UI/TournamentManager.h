#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

// =====================================================================
// TournamentManager
// ---------------------------------------------------------------------
// Modele de donnees + logique d'un tournoi (elimination directe OU
// round robin, au choix a la creation), independant de l'UI (voir
// TournamentDialog) et du moteur de match reel (voir GameManager) --
// ne connait que des noms de joueurs (QString) et des scores en frames,
// jamais un objet Player/Frame/Match. Persiste dans tournoi.json (a
// cote de l'executable, meme convention que joueurs.ini/matchs.json).
//
// Un seul tournoi actif a la fois (pas d'archives multiples) : creer un
// nouveau tournoi remplace le precedent. Choix delibere pour rester
// simple, coherent avec le reste de l'appli qui ne gere pas non plus
// plusieurs "sessions" en parallele.
// =====================================================================
class TournamentManager
{
public:
    // Tournoi et Championnat partagent toute la logique mais pas le
    // fichier de sauvegarde : les deux peuvent exister en meme temps
    // (tournoi.json / championnat.json), voir CompetitionChoiceDialog.
    enum class Kind
    {
        Tournament,
        Championship
    };

    enum class Format
    {
        Elimination,
        RoundRobin,
        // Championnat : round robin ALLER-RETOUR (chaque paire se
        // rencontre deux fois, une fois a domicile de chacun -- le
        // joueur1 du match retour est le joueur2 du match aller).
        League
    };

    // Points de classement d'une victoire (une defaite vaut 0).
    static constexpr int kPointsPerWin = 2;

    explicit TournamentManager(Kind kind = Kind::Tournament) : m_kind(kind) {}

    // Un affrontement prevu au tournoi. `winner` reste vide tant que le
    // match n'a pas ete joue. `isBye` : joueur2 absent (nombre de
    // joueurs pas une puissance de 2 en elimination) -- resolu
    // automatiquement, jamais propose comme "prochain match".
    struct Matchup
    {
        int round = 0;
        QString player1;
        QString player2;
        QString winner;
        int scorePlayer1 = 0;
        int scorePlayer2 = 0;
        bool isBye = false;
        int leg = 0; // Championnat : 1 = match aller, 2 = match retour (0 sinon)
        bool isPlayed() const { return !winner.isEmpty(); }
    };

    // true si un tournoi (fini ou non) est actuellement charge.
    bool isActive() const { return !m_name.isEmpty(); }

    const QString& name() const { return m_name; }
    Format format() const { return m_format; }
    Kind kind() const { return m_kind; }
    // Longueur d'un match en frames a gagner (Championnat : choisie a
    // la creation ; Tournoi : toujours 2, comportement historique).
    int framesToWin() const { return m_framesToWin; }
    bool isRoundRobinLike() const { return m_format != Format::Elimination; }
    const QVector<Matchup>& matchups() const { return m_matchups; }

    // Cree un nouveau tournoi (remplace l'eventuel tournoi en cours,
    // sans le sauvegarder au prealable -- a l'appelant de confirmer
    // aupres de l'utilisateur avant d'appeler ceci si un tournoi non
    // termine existait deja). `players` : au moins 2 noms.
    void create(const QString& name, Format format, const QStringList& players, int framesToWin = 2);

    void clear();

    // Prochain affrontement REEL (ni joue, ni bye, les deux joueurs
    // connus) a proposer au joueur -- ordre = ordre de creation pour le
    // round robin, ou premier round incomplet pour l'elimination
    // (un round n'est "prochain" que si le round precedent est
    // entierement joue). Renvoie nullptr si le tournoi est termine ou
    // si le round courant attend encore un resultat pour determiner les
    // adversaires (elimination uniquement).
    const Matchup* nextMatch() const;

    // Tous les affrontements REELS actuellement en attente (meme
    // critere que nextMatch(), mais la liste complete plutot que le
    // premier seulement) -- necessaire des qu'un tournoi de club peut
    // avoir plusieurs matchs en cours EN MEME TEMPS sur d'autres tables
    // (voir TournamentDialog) : "le prochain match" n'a alors plus de
    // sens unique, l'utilisateur doit pouvoir choisir lequel saisir/lancer.
    QVector<const Matchup*> pendingMatches() const;

    // Enregistre le resultat d'un match (identifie par ses deux noms de
    // joueurs, peu importe l'ordre) : marque le Matchup correspondant
    // comme joue, et pour l'elimination, cree/complete l'affrontement
    // du round suivant avec le vainqueur des que les deux matchs du
    // round courant menant a lui sont joues. Ne fait rien si aucun
    // Matchup en attente ne correspond a ces deux noms (ex. match hors
    // tournoi).
    void recordResult(const QString& player1, const QString& player2, const QString& winner, int scoreWinner, int scoreLoser);

    // true si le tournoi a un vainqueur final (elimination : le dernier
    // match joue ; round robin : tous les matchs joues).
    bool isFinished() const;
    // Vide tant que pas termine ; plusieurs noms seulement en cas
    // d'egalite parfaite en tete (voir Standing::rank).
    QStringList champions() const;
    QString champion() const; // champions() joints par " et "

    // Classement round-robin : points desc. (2 par victoire, pas de nul
    // possible), puis difference de frames desc., puis resultats des
    // matchs DIRECTS entre les joueurs encore a egalite (victoires, puis
    // difference de frames dans ces seuls matchs). Si rien ne les
    // departage, ils gardent le meme rang (Standing::rank) -- jamais
    // d'ordre arbitraire presente comme un vrai classement. -- vide/non pertinent en elimination.
    struct Standing
    {
        QString name;
        int played = 0;
        int won = 0;
        int points = 0; // Championnat : kPointsPerWin par victoire, 0 par defaite
        int rank = 0;   // 1 = premier ; deux joueurs vraiment indeparables partagent le meme rang
        int framesFor = 0;
        int framesAgainst = 0;
    };
    QVector<Standing> standings() const;

    void save() const;
    static TournamentManager load(Kind kind = Kind::Tournament);

private:
    void generateElimination(const QStringList& players);
    void generateRoundRobin(const QStringList& players);
    void generateLeague(const QStringList& players);
    void advanceEliminationIfRoundComplete(int completedRound);

    QString m_name;
    Format m_format = Format::Elimination;
    Kind m_kind = Kind::Tournament;
    int m_framesToWin = 2;
    QVector<Matchup> m_matchups;
};
