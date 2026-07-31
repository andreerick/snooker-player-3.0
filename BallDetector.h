#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

// =====================================================================
// DetectedBall
// ---------------------------------------------------------------------
// Represente une bille detectee sur une image : sa couleur (nom) et
// sa position en pixels (centre du cercle detecte).
// =====================================================================
struct DetectedBall
{
    std::string colorName;
    cv::Point2f pixelPosition;
    float radius;
};

// =====================================================================
// BallDetector
// ---------------------------------------------------------------------
// Detecte les billes de snooker sur une image de camera, par couleur.
// Principe :
//   1. Convertir l'image en HSV (plus stable que RGB face aux variations
//      de luminosite/ombres que le RGB pur).
//   2. Pour chaque couleur de bille (rouge, jaune, vert, marron, bleu,
//      rose, noir, blanc), appliquer un seuillage HSV (cv::inRange)
//      pour isoler les pixels de cette couleur.
//   3. Chercher des contours/cercles dans le masque resultant.
//   4. Filtrer par taille (rayon) pour eliminer le bruit.
//
// Les plages HSV fournies par defaut sont des valeurs de DEPART realistes
// pour un tapis de snooker, mais elles DEVRONT etre ajustees une fois les
// vraies cameras branchees (l'eclairage reel change beaucoup les couleurs
// percues). Voir calibrateColorRange() pour un outil d'ajustement.
// =====================================================================
class BallDetector
{
public:
    BallDetector();

    // Detecte toutes les billes visibles sur l'image donnee.
    // minRadius/maxRadius en pixels : a ajuster selon la distance
    // camera-table et la resolution (1.20m de hauteur, cf. notes projet).
    std::vector<DetectedBall> detectBalls(
        const cv::Mat& frame,
        int minRadius = 8,
        int maxRadius = 40
    ) const;

    // Renvoie le masque binaire (pixels blancs = pixels de cette couleur)
    // pour une couleur donnee. Utile pour debug/reglage visuel.
    cv::Mat getColorMask(const cv::Mat& frame, const std::string& colorName) const;

    // Detecte les billes par la FORME (cercles, via HoughCircles) plutot
    // que par la couleur seule, puis identifie la couleur de chaque
    // cercle trouve en echantillonnant son centre.
    //
    // POURQUOI CETTE METHODE EXISTE EN PLUS DE detectBalls() :
    // La bille verte a une teinte HSV quasiment identique a celle du
    // tapis vert (seule la luminosite differe un peu). Avec un simple
    // seuillage couleur, la bille se "fond" dans tout le tapis et devient
    // indetectable (un seul enorme bloc au lieu d'un petit cercle).
    // HoughCircles cherche des CONTOURS circulaires (gradients de
    // luminosite), ce qui reste efficace meme quand la couleur ne
    // suffit pas a distinguer la bille du fond.
    std::vector<DetectedBall> detectBallsByShape(
        const cv::Mat& frame,
        int minRadius = 8,
        int maxRadius = 40
    ) const;

    // Ajuste la plage HSV d'une couleur (pour calibrer selon l'eclairage reel).
    void setColorRange(
        const std::string& colorName,
        cv::Scalar hsvLow,
        cv::Scalar hsvHigh
    );

private:
    struct ColorRange
    {
        std::string name;
        cv::Scalar low;
        cv::Scalar high;
    };

    std::vector<ColorRange> m_colorRanges;

    std::vector<DetectedBall> detectForColor(
        const cv::Mat& hsvFrame,
        const ColorRange& range,
        int minRadius,
        int maxRadius
    ) const;

    // Retrouve le nom de la couleur la plus proche pour un pixel HSV donne
    // (utilise par detectBallsByShape). Retourne "Inconnue" si aucune
    // plage ne correspond.
    std::string classifyColorAtPoint(const cv::Vec3b& hsvPixel) const;
};
