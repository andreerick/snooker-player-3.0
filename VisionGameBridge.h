#pragma once

#include "BallDetector.h"
#include "BallTracker.h"
#include "TableCapture.h"
#include "BallMapRecorder.h"
#include "ShotAnalyzer.h"
#include "Frame.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

// =====================================================================
// VisionGameBridge
// ---------------------------------------------------------------------
// Le chainon manquant entre MesureVision (detection/suivi des billes)
// et le moteur de jeu (Frame) : transforme une SEQUENCE d'images (les
// images successives d'une meme camera, comme un flux video) en appels
// au moteur de jeu reel (Frame::playShot(), Frame::foul()), exactement
// comme le ferait un clic sur la telecommande.
//
// Principe (deliberement simple, sur la mecanique plutot que sur la
// precision de detection, qui restera a affiner avec de vraies photos) :
//   - Chaque image est passee a BallDetector puis a BallTracker.
//   - Une bille est consideree comme EMPOCHEE seulement si elle reste
//     invisible pendant plusieurs images consecutives (voir BallTracker,
//     maxFramesLost) : ca evite les faux positifs (main qui passe
//     devant, reflet, rate de detection ponctuel).
//   - Des qu'une bille est confirmee empochee, l'action correspondante
//     est jouee sur le Frame fourni : Frame::playShot() decide alors
//     lui-meme, selon les regles du snooker, si le coup est legal ou
//     si c'est une faute (exactement comme pour un clic telecommande).
//   - La bille blanche (cue ball) empochee est traduite en faute
//     directement (elle ne doit jamais disparaitre de la table).
//
// Ce que ce pont NE FAIT PAS (volontairement, hors-scope pour l'instant) :
//   - Detecter qu'un coup a eu lieu quand RIEN n'est empoche (safety) :
//     ca demanderait de suivre le deplacement de la bille blanche, un
//     probleme different (mouvement, pas comptage de billes).
//   - Distinguer une bille respotee (couleur apres une rouge) d'une
//     bille reellement disparue : Frame::playShot() gere deja cette
//     regle correctement des lors qu'on lui donne la bonne bille.
// =====================================================================
class VisionGameBridge
{
public:
    // analysisWidth : largeur (en pixels) a laquelle l'image est
    // redimensionnee juste pour la detection, quelle que soit la
    // resolution reelle de la camera (jusqu'a la 4K et au-dela). Les
    // seuils minRadius/maxRadius/maxMatchDistancePx sont calibres pour
    // cette echelle de travail, pas pour la resolution native de la
    // camera. Les positions detectees sont ensuite remises a l'echelle de
    // l'image d'origine (voir processImage()), donc tout le reste du
    // pipeline (suivi, cartographie, guide de repositionnement, photo de
    // preuve) continue de raisonner en coordonnees/resolution reelles.
    explicit VisionGameBridge(
        int maxFramesLost = 15,
        float maxMatchDistancePx = 40.f,
        int minRadius = 8,
        int maxRadius = 40,
        int analysisWidth = 1280
    );

    // A appeler avec CHAQUE nouvelle image de la sequence (flux video ou
    // suite de photos statiques). Met a jour le suivi interne, et joue
    // sur `frame` toute bille confirmee empochee depuis le dernier appel.
    // `log` recoit une ligne human-readable par action jouee (facultatif,
    // utile pour le debug/la demo).
    void processImage(const cv::Mat& image, Frame& frame, std::vector<std::string>* log = nullptr);

    // Reinitialise le suivi (par exemple au debut d'une nouvelle frame
    // de jeu, pour ne pas confondre l'etat de depart avec des billes
    // "disparues" par rapport a l'image precedente), et repart de zero
    // pour la numerotation des photos de preuve.
    void reset();

    // Dossier ou sauvegarder une photo de la table et la cartographie des
    // billes (.yml) apres chaque coup confirme (preuve visuelle et donnees
    // rechargeables, en cas de contestation ou de miss a verifier). Le
    // dossier doit deja exister (non cree automatiquement). Si jamais
    // appele/vide, rien n'est sauvegarde.
    void setSnapshotFolder(const std::string& folder);

    // Position actuelle de toutes les billes suivies (etat "en direct"),
    // utilisee pour comparer a un instantane precedent dans le guide de
    // repositionnement (voir BallMapRecorder::renderRepositioningGuide).
    const BallTracker& tracker() const { return m_tracker; }

    // Chemin du dernier fichier .yml de cartographie des billes sauvegarde
    // (le dernier coup confirme), a utiliser comme etat CIBLE du guide de
    // repositionnement. Vide si aucun coup n'a encore ete confirme.
    const std::string& lastBallMapPath() const { return m_lastBallMapPath; }

    // Billes ENCORE suivies (donc pas empochees/disparues) mais dont la
    // derniere position connue est jugee dangereusement proche du bord
    // de la table (voir ShotAnalyzer::isNearTableEdge()) au moment du
    // DERNIER appel a processImage() : alerte precoce, avant meme
    // qu'une bille ne sorte reellement. Recalculee entierement a chaque
    // appel (pas cumulative). Vide la plupart du temps.
    const std::vector<std::string>& ballsNearEdge() const { return m_ballsNearEdge; }

    // Definit les zones de poches REELLES (calibrees a la main sur
    // l'image complete de la table a vide, voir ShotAnalyzer) : une fois
    // appelee, remplace definitivement les zones par defaut auto-
    // calculees (proportionnelles a la taille de l'image, voir
    // ensurePocketZonesConfigured()), y compris a travers les appels a
    // reset() (le positionnement physique des cameras ne change pas
    // d'une frame de jeu a l'autre).
    void setPocketZones(const std::vector<PocketZone>& zones);

private:
    BallDetector m_detector;
    BallTracker m_tracker;
    TableCapture m_tableCapture;
    ShotAnalyzer m_shotAnalyzer;
    int m_minRadius;
    int m_maxRadius;
    int m_analysisWidth;

    std::string m_snapshotFolder;
    std::string m_lastBallMapPath;
    int m_shotCounter = 0;

    // Voir ballsNearEdge().
    std::vector<std::string> m_ballsNearEdge;

    // Taille d'image pour laquelle m_shotAnalyzer a ete configure pour
    // la derniere fois (voir ensurePocketZonesConfigured()) : evite de
    // recalculer les zones par defaut a chaque image tant que la
    // resolution ne change pas.
    cv::Size m_shotAnalyzerConfiguredSize;

    // Vrai des que setPocketZones() a ete appele explicitement : dans ce
    // cas, ensurePocketZonesConfigured() ne doit plus jamais ecraser les
    // zones avec ses valeurs par defaut approximatives.
    bool m_pocketZonesManuallySet = false;

    // Sans vraie calibration, calcule des zones de poches par defaut
    // (4 coins + 2 milieux haut/bas), proportionnelles a la taille de
    // l'image de la table complete : mieux que rien pour distinguer un
    // empochage plausible d'une disparition suspecte, en attendant une
    // vraie calibration (voir setPocketZones()).
    void ensurePocketZonesConfigured(const cv::Size& imageSize);
};