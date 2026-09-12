#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QFrame>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QMap>
#include <QJsonObject>
#include <QScrollArea>
#include <QTimer>
#include <QDateTime>
#include <QPointer>
#include <QDialog>
#include "GameManager.h"
#include "MoveLogWidget.h"
#include "TestScenarioRunner.h"
#include "SpeechAnnouncer.h"
#include "RulesReferenceDialog.h"
#include "MatchWebServer.h"
#include "HomeScreen.h"
#include "CueSenseLauncher.h"
#include <QStackedWidget>
#include "VisionGameBridge.h"
#include "TableCapture.h"
#include <opencv2/videoio.hpp>

// Action en attente d'une bille cliquee sur la telecommande : le prochain
// clic sur un bouton de bille declenche cette action au lieu d'un coup
// normal. Permet de choisir la bille concernee directement dans la
// telecommande (Faute/Free ball/Miss) plutot que via une boite de dialogue.
enum class PendingAction
{
    None,
    Foul,
    // Meme mecanique que Foul (meme calcul de penalite), mais motif distinct
    // dans le journal des coups : une bille forcee hors de la table est
    // toujours une faute, avec la meme formule de penalite (voir Referee).
    BallOffTable,
    ArmFreeBall,
    // Free ball arme alors que "n'importe quelle couleur" etait legale
    // (Frame::getRequiredBall() ambigu) : impossible de deduire quelle
    // valeur le Free Ball remplace, donc on demande explicitement au
    // arbitre de l'annoncer avant de pouvoir jouer le coup (voir
    // Frame::setFreeBallValue()). Etape intermediaire uniquement dans ce
    // cas ambigu -- sinon (rouge ou couleur des couleurs finales
    // clairement dues) la valeur est deduite automatiquement.
    ArmFreeBallValue,
    // Faute (ou Bille sortie de table) survenue alors que "n'importe
    // quelle couleur" etait legale (meme ambiguite que ArmFreeBallValue) :
    // impossible de deduire automatiquement la bille visee pour calculer
    // la penalite (Referee::calculateFoul utilise le MAX entre bille
    // visee et bille touchee), donc on demande explicitement au arbitre
    // de l'annoncer. La bille touchee (deja cliquee) est memorisee dans
    // m_pendingFoulTouchedBall/m_pendingFoulReason en attendant.
    AnnounceFoulTarget,
    MissReplay,
    MissSelfPlay,
    MissFoulThenFreeBall
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void refreshDisplay();

    // Demande les noms des joueurs ET le nombre de frames du match (menu
    // deroulant des joueurs deja enregistres + saisie libre, "meilleur
    // des X frames" a choisir a chaque match plutot qu'un reglage global
    // -- voir la discussion avec l'utilisateur). Utilise au demarrage et
    // par le bouton "Nouveau match".
    void promptPlayerNames(QString& player1Name, QString& player2Name, int& framesToWin);

    // Annonce a voix haute (voir SpeechAnnouncer) les evenements
    // survenus depuis le dernier appel : points marques, fautes,
    // changement de joueur. Compare l'etat courant du Frame a
    // m_lastAnnouncedLogSize/m_lastAnnouncedPlayer pour ne dire que ce
    // qui est NOUVEAU, quel que soit le nombre de fois ou refreshDisplay()
    // est appele. Appelee depuis refreshDisplay() : fonctionne donc pour
    // TOUTE source de coup (boutons manuels, suivi camera, rejeu de
    // scenario), pas seulement les clics.
    void announceNewEvents(Frame& frame);

    // Logique commune a un clic sur une bille depuis la telecommande ET
    // depuis la page de controle a distance (voir MatchWebServer) :
    // resout l'action en attente (Faute/Free ball/Miss/...) ou joue un
    // coup normal si aucune n'est en attente. Extrait du gestionnaire de
    // clic des boutons de bille pour eviter la duplication.
    void handleBallAction(const QString& ballName, int ballValue);

    // Sauvegarde une copie du Frame courant juste AVANT d'appliquer un
    // coup, pour permettre au bouton "Esc" de la telecommande 2.0
    // d'annuler ce dernier coup (un seul niveau d'annulation -- pas de
    // pile d'historique complete). Limite connue : si le coup annule
    // avait termine la frame (victoire comptabilisee dans Match), le
    // compteur de frames du match n'est pas revert -- cas rare, non gere
    // pour l'instant.
    void snapshotFrameForUndo();

    // Capture l'image camera courante, compare aux dernieres positions
    // connues (BallMapRecorder) et affiche le guide de repositionnement
    // en plein ecran (voir showRepositioningGuideDialog()). Extrait du
    // bouton "Guide de repositionnement" pour pouvoir aussi etre appelee
    // automatiquement des que l'option Miss "Remettre en place" est
    // resolue (voir handleBallAction(), cas PendingAction::MissReplay),
    // que le clic vienne du PC ou du telephone -- les deux passent par
    // handleBallAction(). Ne fait rien silencieusement si le suivi camera
    // n'est pas actif ou qu'aucun coup n'a encore ete confirme (l'appel
    // automatique ne doit pas interrompre la partie avec une erreur).
    void showRepositioningGuide(bool silentIfUnavailable);

    // Ferme le guide de repositionnement s'il est actuellement affiche
    // (voir m_repositionGuideDialog). Ne fait rien s'il n'y en a pas --
    // permet au bouton "Fermer le guide" d'etre present sur les 3
    // telecommandes (1.0, 2.0, telephone) sans jamais planter si le
    // guide n'est pas ouvert.
    void closeRepositioningGuide();

    // Dispatche une action recue depuis la page de controle a distance
    // (telephone) vers la meme logique que les boutons de la
    // telecommande de bureau. Connecte au signal
    // MatchWebServer::controlActionRequested (livraison automatiquement
    // mise en file sur le thread Qt/GUI, voir MatchWebServer).
    void handleRemoteControlAction(const QString& action, const QJsonObject& params);

    // Demande les noms sur le PC (promptPlayerNames()) puis demarre le
    // tout premier match via beginMatch(). Connectee a
    // HomeScreen::newMatchRequested (tuile "Nouveau match").
    void startNewMatchFromHome();

    // Reinitialise le moteur de jeu pour un nouveau match SANS refaire
    // l'initialisation "premiere fois seulement" de beginMatch() (timers,
    // voix, pont vision, rejeu de scenario -- deja crees et toujours
    // valides). Utilisee par le bouton "Nouveau match" de la telecommande
    // ET par l'enchainement automatique des matchs de tournoi (voir
    // startTournamentMatch()) : les deux ont besoin du meme reset leger,
    // seule la provenance des noms differe.
    void restartMatch(const QString& player1Name, const QString& player2Name, int framesToWin = 2);

    // Lance un match dans le cadre d'un tournoi (voir TournamentDialog::
    // matchRequested) : beginMatch() si c'est le tout premier match de la
    // session (ecran d'accueil encore affiche), sinon restartMatch().
    // Marque m_tournamentMatchActive pour que refreshDisplay() sache, une
    // fois ce match termine, qu'il doit enregistrer le resultat dans
    // TournamentManager et proposer le match suivant (voir
    // handleTournamentMatchFinished()).
    void startTournamentMatch(const QString& player1Name, const QString& player2Name);

    // Enregistre le resultat du match de tournoi qui vient de se
    // terminer dans tournoi.json, puis propose (boite de dialogue) de
    // lancer immediatement le match suivant du bracket/de la poule --
    // appelee depuis refreshDisplay() des que match.isMatchFinished()
    // ET m_tournamentMatchActive. Ne touche a rien du moteur de jeu.
    void handleTournamentMatchFinished();

    // Demarre reellement le tout premier match (GameManager::startNewMatch()
    // et toute l'initialisation qui en depend : timers, voix, suivi
    // camera, rejeu de scenario) puis bascule de l'ecran d'accueil vers
    // la vue du match. Sequence normalement lancee tot dans le
    // constructeur : deplacee ici pour ne se declencher qu'une fois les
    // noms des joueurs connus, que ce soit via le dialogue PC
    // (startNewMatchFromHome()) OU directement depuis le telephone (voir
    // handleRemoteControlAction(), action "submitNames" recue avant que
    // m_matchStarted ne soit vrai -- cas du QR code imprime sur la table,
    // scanne sans que personne ne touche au PC).
    void beginMatch(const QString& player1Name, const QString& player2Name, int framesToWin = 2);

    GameManager m_gameManager;

    // Voir snapshotFrameForUndo() / le bouton "Esc" de la telecommande 2.0.
    Frame m_undoSnapshot;
    bool m_hasUndoSnapshot = false;

    SpeechAnnouncer* m_speech = nullptr;
    QPushButton* m_speechToggleButton = nullptr;

    // Serveur web local + QR code (voir MatchWebServer/ShareSessionDialog) :
    // permet a un joueur de suivre le score en direct depuis son
    // telephone, sur le meme Wi-Fi que le PC.
    MatchWebServer* m_webServer = nullptr;
    QPushButton* m_shareButton = nullptr;

    // Ecran d'accueil (voir HomeScreen) : premiere vue affichee au
    // lancement, avant qu'un match n'existe reellement. m_rootStack
    // bascule entre cet ecran (index 0) et la vue du match (index 1,
    // construite normalement mais cachee jusqu'a "Nouveau match").
    QStackedWidget* m_rootStack = nullptr;
    HomeScreen* m_homeScreen = nullptr;

    // Vrai pendant le match actuellement en cours si celui-ci fait
    // partie d'un tournoi (voir TournamentManager/TournamentDialog) :
    // determine si refreshDisplay() doit, une fois le match termine,
    // enregistrer le resultat dans le tournoi et proposer le suivant
    // plutot que simplement sauvegarder dans matchs.json comme un match
    // isole. Remis a false des que le resultat a ete enregistre.
    bool m_tournamentMatchActive = false;

    // Vrai des que le tout premier match a reellement demarre (voir
    // beginMatch()) : protege handleRemoteControlAction() contre une
    // action recue (bille, faute...) avant qu'un match n'existe, et
    // empeche l'action "submitNames" du telephone de redeclencher
    // beginMatch() une fois la partie en cours.
    bool m_matchStarted = false;

    // Vrai pendant que la boite de dialogue de saisie des noms (voir
    // promptPlayerNames()) est ouverte sur le PC : le formulaire de
    // saisie sur le telephone reste utilisable pendant ce temps (voir le
    // connect() local dans promptPlayerNames()), donc handleRemoteControlAction()
    // doit s'abstenir de traiter "submitNames" lui-meme tant que ce
    // drapeau est vrai, pour eviter de demarrer le match deux fois.
    bool m_playerNameDialogOpen = false;

    // Lance CueSense (entrainement) dans le navigateur, voir
    // CueSenseLauncher. Objet simple (pas un QObject), duree de vie liee
    // a MainWindow.
    CueSenseLauncher m_cueSenseLauncher;

    // Curseur de progression dans le journal (voir announceNewEvents()) :
    // index (taille du journal) jusqu'ou les evenements ont deja ete
    // annonces. Remis a zero automatiquement si le journal redevient
    // plus court que ce curseur (nouvelle frame, rejeu de scenario...).
    size_t m_lastAnnouncedLogSize = 0;

    // Dernier joueur dont le tour a ete annonce (voir announceNewEvents()) ;
    // nullptr = rien annonce encore (evite d'annoncer "Au tour de ..."
    // au tout premier affichage).
    Player* m_lastAnnouncedPlayer = nullptr;

    // Evite de repeter "Frame pour ..."/"... gagne le match" a chaque
    // refreshDisplay() tant que l'etat de fin de frame/match reste vrai
    // (le score final reste affiche plusieurs secondes avant la frame
    // suivante, voir m_nextFrameTimer). Remis a false a chaque nouveau
    // match (bouton "Nouveau match") et, pour m_frameEndAnnounced, des
    // que l'etat "frame juste terminee" redevient faux.
    bool m_frameEndAnnounced = false;
    bool m_matchEndAnnounced = false;

    QFrame* m_player1Frame = nullptr;
    QFrame* m_player2Frame = nullptr;

    QLabel* m_player1RoleLabel = nullptr;
    QLabel* m_player2RoleLabel = nullptr;
    QLabel* m_player1NameLabel = nullptr;
    QLabel* m_player2NameLabel = nullptr;
    QLabel* m_pointsRemainingLabel = nullptr;

    QLabel* m_bestOfLabel = nullptr;
    QLabel* m_frameScoreLabel = nullptr;
    QLabel* m_currentFrameLabel = nullptr;

    QLabel* m_breakValueLabel = nullptr;
    QLabel* m_nextBallDot = nullptr;
    QLabel* m_nextBallNameLabel = nullptr;

    // Billes restantes sur la table : une pastille par couleur (Rouge,
    // Jaune, Verte, Marron, Bleue, Rose, Noire), avec le nombre restant
    // affiche dedans (15->0 pour les rouges, 1 ou rien pour les autres).
    // Cachee des que BallSet::isOnTable() devient faux pour cette
    // couleur (retiree DEFINITIVEMENT de la table, pas juste respotee
    // en cours de phase des rouges -- voir refreshDisplay()).
    QMap<QString, QLabel*> m_remainingBallLabels;

    QFrame* m_player1ScoreBox = nullptr;
    QFrame* m_player2ScoreBox = nullptr;
    QLabel* m_player1ScoreBoxValue = nullptr;
    QLabel* m_player2ScoreBoxValue = nullptr;

    // Detail des points marques dans la frame en cours (billes empochees
    // normalement / fautes adverses), un jeu de labels par joueur.
    QLabel* m_player1DetailPotted = nullptr;
    QLabel* m_player1DetailFouls = nullptr;
    QLabel* m_player2DetailPotted = nullptr;
    QLabel* m_player2DetailFouls = nullptr;

    QWidget* m_successionRow = nullptr;

    // Pied de page : statut cameras (statique pour l'instant, la
    // connexion reelle aux cameras n'est pas encore branchee a cette
    // UI), duree du match et statut d'enregistrement.
    QLabel* m_durationLabel = nullptr;
    QLabel* m_recordingDot = nullptr;
    QLabel* m_recordingStatusLabel = nullptr;
    QTimer* m_durationTimer = nullptr;
    QDateTime m_matchStartTime;

    QLabel* m_freeBallStatusLabel = nullptr;
    QLabel* m_pendingActionLabel = nullptr;
    QPushButton* m_cancelPendingButton = nullptr;
    PendingAction m_pendingAction = PendingAction::None;
    QPushButton* m_toggleLogButton = nullptr;

    MoveLogWidget* m_moveLogWidget = nullptr;

    // Panneau de controles manuels (boutons de billes, faute, etc.),
    // masquable pour afficher un tableau de score epure (voir
    // m_toggleRemoteButton, dans le pied de page, toujours visible).
    // Deux versions coexistent (voir Parametres > Telecommande) :
    // m_remotePanel (1.0, complet, pour scenarios/tests) et
    // m_remotePanelSimple (2.0, couleurs + Faute + Fin de break + Esc +
    // Game seulement, pour l'usage courant). Une seule des deux est
    // visible a la fois -- voir applyRemotePanelVisibility().
    QFrame* m_remotePanel = nullptr;
    QFrame* m_remotePanelSimple = nullptr;
    QPushButton* m_toggleRemoteButton = nullptr;

    // Guide de repositionnement actuellement affiche (fullscreen, non
    // modal -- voir showRepositioningGuideDialog()), ou nullptr si aucun.
    // QPointer se remet a nullptr automatiquement si le dialogue est
    // ferme/detruit, meme depuis un autre bouton (telephone, 1.0, 2.0).
    QPointer<QDialog> m_repositionGuideDialog;

    // Memorise la bille reellement touchee et le motif de la faute
    // (Foul ou BallOffTable) pendant l'etape intermediaire
    // PendingAction::AnnounceFoulTarget (voir handleBallAction()) : la
    // bille visee/annoncee arrive au clic SUIVANT, une fois l'ambiguite
    // (n'importe quelle couleur legale) resolue par l'arbitre.
    Ball m_pendingFoulTouchedBall = Ball("Aucune", 0);
    std::string m_pendingFoulReason = "Mauvaise bille touchee";

    // Preference manuelle (bouton m_toggleRemoteButton, persistee dans
    // settings.ini "ui/remoteVisible") : independante du masquage
    // automatique quand un telephone est connecte (voir
    // applyRemotePanelVisibility()).
    bool m_remoteManualVisible = true;

    // Applique la visibilite effective des DEUX panneaux telecommande :
    // le manuel (m_remoteManualVisible) ET l'auto-masquage si un
    // telephone est connecte (voir MatchWebServer::hasActiveClient())
    // ET le choix 1.0/2.0 (settings.ini "ui/remoteVersion"). Appelee a la
    // construction, sur clic du bouton, a chaque refreshDisplay() (pour
    // reagir a une connexion/deconnexion telephone) et si le choix change
    // en direct depuis Parametres.
    void applyRemotePanelVisibility();

    QTimer* m_nextFrameTimer = nullptr;

    // Rejeu automatique d'une sequence de coups de demonstration, sur le
    // vrai moteur de jeu, pour tester l'affichage sans cliquer coup par coup.
    TestScenarioRunner* m_scenarioRunner = nullptr;
    QPushButton* m_scenarioButton = nullptr;

    // Evite de sauvegarder plusieurs fois le meme match termine (refreshDisplay
    // est appele en continu, y compris apres la fin du match).
    bool m_matchSaved = false;

    // Suivi camera en direct : transforme les images de la camera en
    // coups reels sur le moteur de jeu (voir VisionGameBridge). Demarre/
    // arrete via le bouton "Demarrer suivi camera" de la telecommande.
    void toggleVisionTracking();

    VisionGameBridge* m_visionBridge = nullptr;
    cv::VideoCapture m_camera;   // camera 0 (gauche) : toujours utilisee, seule camera requise en mode simple.
    cv::VideoCapture m_camera1;  // camera 1 (milieu) : optionnelle, voir m_multiCameraMode.
    cv::VideoCapture m_camera2;  // camera 2 (droite) : optionnelle, voir m_multiCameraMode.
    QTimer* m_visionTimer = nullptr;
    QPushButton* m_visionButton = nullptr;

    // Assemble les 3 images camera en une seule image complete de la
    // table (voir TableCapture). Utilisee seulement si m_multiCameraMode
    // est actif.
    TableCapture m_tableCapture;

    // Vrai si les 3 cameras (indices 0, 1, 2) ont pu etre ouvertes au
    // demarrage du suivi : dans ce cas chaque image est l'assemblage des
    // 3 flux (voir m_tableCapture) plutot que le flux brut de la camera 0.
    // Reste a false (mode simple, 1 seule camera) si seule la camera 0
    // est disponible (ex. poste de developpement avec une seule webcam,
    // en attendant les 3 cameras reelles) : le comportement est alors
    // identique a avant l'ajout du support multi-camera.
    bool m_multiCameraMode = false;

    // Alerte precoce : liste (texte) des billes encore sur la table mais
    // detectees proches du bord (voir VisionGameBridge::ballsNearEdge()).
    // Cachee des que la liste est vide.
    QLabel* m_edgeWarningLabel = nullptr;

    // Rejoue un scenario_*.txt enregistre coup par coup (comme
    // m_scenarioRunner, mais pilote par un fichier au lieu d'une liste
    // codee en dur), pour suivre visuellement le deroule au lieu de
    // sauter directement au resultat final.
    QTimer* m_replayTimer = nullptr;

    // Choix du delai entre deux coups du rejeu (voir m_replayTimer),
    // lu au demarrage de chaque rejeu.
    QComboBox* m_replaySpeedCombo = nullptr;

    // Choix du scenario_*.txt a rejouer (le plus recent est en tete),
    // rafraichi a chaque nouvel enregistrement ("Enregistrer le scenario").
    QComboBox* m_scenarioFileCombo = nullptr;
};