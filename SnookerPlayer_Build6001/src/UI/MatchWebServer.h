#pragma once

#include <QObject>
#include <QString>
#include <QMutex>
#include <QJsonObject>
#include <QMap>
#include <memory>
#include <thread>

namespace httplib { class Server; }

// =====================================================================
// MatchWebServer
// ---------------------------------------------------------------------
// Petit serveur web local (cpp-httplib, voir third_party/httplib.h) qui
// expose l'etat courant du match (score, break, joueur au tir) a une
// page web simple, pour qu'un joueur puisse suivre le match en direct
// depuis son telephone connecte au meme Wi-Fi que le PC (voir
// ShareSessionDialog pour le QR code qui pointe vers cette page).
//
// Volontairement LAN uniquement (pas d'exposition internet) : pas de
// HTTPS, juste un jeton alea toire dans l'URL pour eviter qu'un autre
// appareil sur le meme Wi-Fi tombe par hasard sur le match. Le serveur
// tourne sur son propre thread (httplib::Server::listen() est bloquant) ;
// l'etat partage est protege par un mutex, seul point de contact avec
// le thread Qt/GUI qui appelle updateState() a chaque refreshDisplay().
//
// Limite a 2 telephones simultanement connectes par table (voir
// registerClient() dans le .cpp) : au-dela, la page de controle
// interfererait plus qu'elle n'aiderait.
// =====================================================================
class MatchWebServer : public QObject
{
    Q_OBJECT

public:
    explicit MatchWebServer(QObject* parent = nullptr);
    ~MatchWebServer() override;

    // Tente d'ouvrir le port utilise la derniere fois (ou preferredPort
    // au tout premier lancement), puis les 9 suivants si deja occupe.
    // Renvoie false si aucun port n'a pu etre ouvert. Le jeton de session
    // ET le port sont persistes localement (table.ini) : le lien reste
    // stable d'un lancement de l'appli a l'autre (voir MatchWebServer.cpp).
    bool start(int preferredPort = 8080);
    void stop();
    bool isRunning() const { return m_running; }

    int port() const { return m_port; }
    QString sessionToken() const { return m_token; }

    // Chemin complet (sans le http://host:port) de la page a scanner,
    // ex. "/m/AB12CD34".
    QString pagePath() const { return "/m/" + m_token; }

    // A appeler depuis le thread Qt/GUI (ex. dans MainWindow::refreshDisplay())
    // pour publier l'etat courant du match. Thread-safe.
    void updateState(const QJsonObject& state);

signals:
    // Emis depuis le thread du serveur HTTP (voir setupRoutes()) quand la
    // page de controle envoie une action (bille cliquee, faute armee...).
    // Qt met automatiquement la livraison en file sur le thread du
    // recepteur (voir le commentaire de connexion dans MainWindow) : le
    // code qui gere `action` peut donc toucher Frame/Match sans risque,
    // comme s'il s'agissait d'un clic de souris normal.
    void controlActionRequested(const QString& action, const QJsonObject& params);

private:
    void setupRoutes();

    // Verifie/enregistre un appareil (identifiant genere cote telephone,
    // voir kPageHtml) parmi les clients actifs de cette table. Purge
    // d'abord les clients qui n'ont plus donne signe de vie depuis
    // kClientTimeoutMs (onglet ferme, telephone sorti du Wi-Fi...) pour
    // liberer leur place. Renvoie true si `clientId` est deja connu OU
    // s'il reste une place (kMaxClients), false si la table est deja
    // utilisee par kMaxClients AUTRES appareils. Thread-safe (appelee
    // depuis le thread du serveur HTTP).
    bool registerClient(const QString& clientId);

    std::unique_ptr<httplib::Server> m_server;
    std::thread m_serverThread;
    mutable QMutex m_stateMutex;
    QJsonObject m_state;
    QString m_token;
    int m_port = 0;
    bool m_running = false;

    // Appareils actuellement "connectes" a cette table (voir registerClient()) :
    // identifiant -> horodatage (ms epoch) du dernier appel recu. Meme
    // mutex que m_state (contention negligeable, un seul point d'acces
    // partage entre les quelques requetes/seconde des 1-2 telephones vises).
    QMap<QString, qint64> m_activeClients;
};
