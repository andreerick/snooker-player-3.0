#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// =====================================================================
// TableCapture
// ---------------------------------------------------------------------
// Assemble les images des 3 cameras en UNE SEULE image complete de la
// table. Volontairement simple pour l'instant :
//   - Pas de conversion en cm, pas d'homographie complexe (ça, c'est
//     reserve a un futur programme d'assistance/mesure, genre "cuesense").
//   - Juste un assemblage cote a cote (les 3 zones se suivent de gauche
//     a droite le long de la table), en rognant la zone de recouvrement
//     entre cameras voisines pour eviter les doublons visuels.
//
// Rappel du placement (voir notes du projet) :
//   Table 357 x 178 cm, 3 cameras a 1.20m de hauteur, chacune couvrant
//   environ un tiers de la longueur (~119-130cm), avec ~10-15cm de
//   recouvrement entre zones voisines.
// =====================================================================
class TableCapture
{
public:
    // overlapPixels : largeur (en pixels) de la zone de recouvrement a
    // rogner entre deux cameras voisines. A ajuster une fois les vraies
    // cameras branchees et les images reelles observees (10-15cm de
    // recouvrement reel ne correspondent pas forcement au meme nombre
    // de pixels sur chaque camera).
    explicit TableCapture(int overlapPixels = 60);

    // Assemble 3 images (dans l'ordre gauche -> droite le long de la
    // table) en une seule image complete. Les 3 images doivent avoir
    // la meme hauteur (redimensionnees si besoin).
    cv::Mat buildFullTableImage(
        const cv::Mat& camera0,
        const cv::Mat& camera1,
        const cv::Mat& camera2
    ) const;

    // Sauvegarde l'image complete sur disque, avec un nom horodate
    // (utile pour garder une image "apres chaque coup").
    // Renvoie le chemin du fichier sauvegarde, ou une chaine vide en cas d'echec.
    std::string saveSnapshot(const cv::Mat& fullTableImage, const std::string& folderPath) const;

private:
    int m_overlapPixels;

    // Rogne la moitie du recouvrement de chaque cote d'une image du
    // milieu, pour ne garder que sa partie "unique".
    cv::Mat trimOverlap(const cv::Mat& image, bool trimLeft, bool trimRight) const;
};
