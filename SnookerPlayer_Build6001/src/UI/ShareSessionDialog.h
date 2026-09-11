#pragma once

#include <QDialog>
#include <QString>

class MatchWebServer;

// =====================================================================
// ShareSessionDialog
// ---------------------------------------------------------------------
// Affiche un QR code (voir third_party/qrcodegen) pointant vers la page
// de suivi du match servie par MatchWebServer, pour qu'un joueur la
// scanne avec son telephone et suive le score en direct (meme Wi-Fi que
// le PC, pas d'internet necessaire).
// =====================================================================
class ShareSessionDialog : public QDialog
{
    Q_OBJECT

public:
    // `server` doit deja etre demarre (MatchWebServer::start() appele)
    // avant de construire ce dialogue.
    explicit ShareSessionDialog(MatchWebServer* server, QWidget* parent = nullptr);
};

// Meilleure estimation de l'adresse IP locale (Wi-Fi/LAN) du PC, pour
// construire l'URL a encoder dans le QR code. Renvoie une chaine vide
// si aucune adresse IPv4 privee exploitable n'a ete trouvee (ex. pas de
// reseau connecte). Exposee separement du dialogue pour rester testable
// sans ouvrir de fenetre.
QString detectLocalLanAddress();
