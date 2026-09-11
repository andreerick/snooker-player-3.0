#pragma once

#include <QWidget>
#include <QPixmap>
#include <QRect>
#include <QVector>

class QPushButton;
class QLabel;

// =====================================================================
// HomeScreen
// ---------------------------------------------------------------------
// Premier ecran affiche au lancement de l'appli (voir MainWindow) : point
// d'entree unique pour se connecter au telephone, consulter le
// reglement, lancer l'entrainement (CueSense) ou demarrer un match.
//
// Contrairement a une UI Qt classique construite en widgets/layouts,
// cet ecran affiche l'image de maquette fournie par l'utilisateur
// TELLE QUELLE en fond (voir HOME_SCREEN_IMAGE_PATH, assets/accueil_v2.png,
// demande explicitement "je veux cette image exactement"), avec de vrais
// QPushButton totalement transparents positionnes par-dessus les zones
// cliquables de l'image (coordonnees mesurees pixel par pixel sur
// l'image source, voir m_hotspots). L'image est mise a l'echelle en
// conservant ses proportions (letterbox noir, comme le fond de l'image
// elle-meme) ; resizeEvent() replace les boutons en consequence.
// =====================================================================
class HomeScreen : public QWidget
{
    Q_OBJECT

public:
    explicit HomeScreen(QWidget* parent = nullptr);

    // A appeler par MainWindow juste apres la construction, une fois
    // l'etat reel connu (m_webServer->isRunning() -- le serveur demarre
    // dans le constructeur de MainWindow, avant que HomeScreen n'existe,
    // donc son resultat est deja disponible a cet instant). Met a jour
    // le bandeau de statut (voir updateSystemStatus()).
    void setWifiStatus(bool active);

signals:
    void newMatchRequested();
    void smartphoneRequested();
    void rulesRequested();
    void trainingRequested();
    void playersRequested();
    void tournamentRequested();
    void settingsRequested();
    void tutorialsRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // Cree un bouton invisible (pas de style visible : l'image porte
    // deja tout le visuel) au-dessus de `nativeRect` (coordonnees en
    // pixels de l'image source, voir m_hotspots) et l'ajoute a la liste
    // repositionnee par resizeEvent(). `tooltip` reste vide pour les
    // zones fonctionnelles (pas besoin d'explication), et porte
    // "Bientot disponible" pour les zones pas encore cablees.
    QPushButton* createHotspot(const QRect& nativeRect, bool enabled, const QString& tooltip);

    void showAboutDialog();

    // Recalcule le facteur d'echelle + le decalage de centrage pour que
    // l'image (proportions fixes) tienne dans la taille actuelle du
    // widget sans la deformer (letterbox), et repositionne chaque
    // bouton en consequence. Appelee par resizeEvent() ET par le
    // constructeur (position initiale).
    void relayoutHotspots();

    QPixmap m_background;

    // Zone cliquable (coordonnees natives, dans l'espace de l'image
    // source) associee a chaque bouton -- parallele a l'ordre de
    // creation des boutons, reutilise par relayoutHotspots().
    struct Hotspot
    {
        QRect nativeRect;
        QWidget* widget; // QPushButton pour les zones cliquables, QLabel pour l'horloge
    };
    QVector<Hotspot> m_hotspots;

    // Horloge en direct : l'image de fond affiche une heure/date figees
    // ("21:30 / 21 juillet 2024", decor du mockup) -- ce QLabel se place
    // par-dessus (meme mecanisme que les hotspots cliquables) avec un
    // fond noir opaque pour la masquer, et affiche l'heure reelle,
    // rafraichie chaque minute.
    QLabel* m_clockLabel = nullptr;
    void updateClock();

    // Bandeau "SYSTEME PRET" : l'image de fond affiche 3 lignes de
    // statut figees ("3 cameras detectees", "Mini-PC connecte", "Base
    // de donnees OK", decor du mockup) qui sont fausses en l'etat reel
    // du projet (une seule camera achetee, pas de "mini-PC" distinct
    // dans l'architecture) -- meme traitement que l'horloge : masquees
    // puis remplacees par un vrai statut mesure a la construction.
    QLabel* m_statusLabel = nullptr;
    bool m_wifiActive = false;
    int m_cameraCount = 0; // mesure une seule fois a la construction (sondage lent, voir HomeScreen.cpp)
    void updateSystemStatus();
};
