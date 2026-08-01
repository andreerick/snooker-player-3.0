#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <map>

// =====================================================================
// PocketZone
// ---------------------------------------------------------------------
// Zone approximative (cercle) d'une poche, en coordonnees PIXEL de
// l'image complete de la table (celle produite par TableCapture).
// Pas de conversion en cm ici, comme demande : uniquement des zones
// pixel, a definir une fois (a la main, comme pour CalibrationTool)
// en cliquant sur l'image complete de la table a vide.
// =====================================================================
struct PocketZone
{
    std::string name;      // "haut-gauche", "haut-milieu", "haut-droite",
                            // "bas-gauche", "bas-milieu", "bas-droite"
    cv::Point2f center;
    float radius;
};

// =====================================================================
// BallPosition
// ---------------------------------------------------------------------
// Position d'une bille a un instant donne (une image), pour construire
// une trajectoire (suite de positions dans le temps).
// =====================================================================
struct BallPosition
{
    cv::Point2f position;
    bool visible = true; // false si la bille n'a pas ete detectee cette image
};

// =====================================================================
// ContactResult
// =====================================================================
struct ContactResult
{
    bool contactFound = false;
    std::string touchedColorName;
    int touchedBallIndex = -1; // utile pour distinguer laquelle des 15 rouges
    int frameIndex = -1;       // a quelle image du coup le contact a eu lieu
};

// =====================================================================
// PottedResult
// =====================================================================
enum class DisappearanceType
{
    Pocketed,       // disparue pres d'une poche -> probablement empochee
    OffTable,       // disparue pres du bord de la table, pas d'une poche -> suspect (sortie de table)
    Unknown         // disparue ailleurs (occlusion, main du joueur, etc.) -> a verifier, pas fiable
};

struct DisappearanceResult
{
    DisappearanceType type;
    std::string pocketName; // rempli seulement si type == Pocketed
};

// =====================================================================
// ShotAnalyzer
// ---------------------------------------------------------------------
// Analyse un coup a partir des trajectoires de billes (suites de
// positions image par image, fournies par BallDetector + BallTracker) :
//   1. Quelle bille la blanche touche EN PREMIER (verification de la
//      bille "sur laquelle on joue", designee par le joueur).
//   2. Si une bille qui disparait le fait pres d'une poche (empochage
//      valide) ou ailleurs (suspect : sortie de table ou occlusion).
//   3. Si une bille encore visible s'approche dangereusement du bord
//      de la table (risque de sortie).
// =====================================================================
class ShotAnalyzer
{
public:
    ShotAnalyzer();

    // Definit les zones de poches (a calibrer une fois, a la main,
    // sur l'image complete de la table a vide).
    void setPocketZones(const std::vector<PocketZone>& zones);
    const std::vector<PocketZone>& getPocketZones() const;

    // Distance de contact : si la distance entre la blanche et une autre
    // bille passe sous ce seuil (en pixels), on considere qu'il y a
    // contact. A ajuster selon le rayon reel des billes a l'image
    // (typiquement un peu plus que 2x le rayon d'une bille).
    void setContactThreshold(float pixels);

    // Marge (en pixels) depuis le bord de l'image de la table a partir
    // de laquelle une bille est consideree "proche du bord" (risque de
    // sortie de table).
    void setEdgeMargin(float pixels);

    // -----------------------------------------------------------------
    // Analyse le premier contact de la blanche sur tout un coup, a
    // partir de sa trajectoire et de celles des autres billes (indexees
    // par nom, avec plusieurs billes possibles pour "Rouge").
    // whiteBallTrajectory[i] = position de la blanche a l'image i.
    // otherBallsTrajectories["Rouge"][j][i] = position de la rouge n°j a l'image i.
    // -----------------------------------------------------------------
    ContactResult findFirstContact(
        const std::vector<BallPosition>& whiteBallTrajectory,
        const std::map<std::string, std::vector<std::vector<BallPosition>>>& otherBallsTrajectories
    ) const;

    // -----------------------------------------------------------------
    // Determine pourquoi une bille a disparu, a partir de sa DERNIERE
    // position connue avant de disparaitre, et de la taille de l'image
    // complete de la table.
    // -----------------------------------------------------------------
    DisappearanceResult analyzeDisappearance(
        const cv::Point2f& lastKnownPosition,
        const cv::Size& tableImageSize
    ) const;

    // -----------------------------------------------------------------
    // Vérifie si une bille ENCORE VISIBLE est dangereusement proche du
    // bord (risque de sortie de table imminente, avant même qu'elle
    // ne disparaisse). Utile pour une alerte en direct.
    // -----------------------------------------------------------------
    bool isNearTableEdge(
        const cv::Point2f& position,
        const cv::Size& tableImageSize
    ) const;

private:
    std::vector<PocketZone> m_pocketZones;
    float m_contactThresholdPx;
    float m_edgeMarginPx;

    static float distance(const cv::Point2f& a, const cv::Point2f& b);
};
