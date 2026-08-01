#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <map>
#include <vector>
#include "BallTracker.h"

// =====================================================================
// BallMapRecorder
// ---------------------------------------------------------------------
// Apres chaque coup, enregistre la position EXACTE de toutes les billes
// encore sur la table (coordonnees pixel de l'image complete produite
// par TableCapture). Objectif : pouvoir revenir en arriere si l'arbitre
// ou un joueur demande une repositionnement des billes (par exemple
// apres une contestation, ou si des billes sont accidentellement
// deplacees/bousculees).
//
// Deux formats de sortie :
//   1. Un fichier .yml (donnees brutes, precises, rechargeables par le
//      programme lui-meme).
//   2. Une image .png (schema visuel, lisible par un humain, avec un
//      cercle de la bonne couleur a la bonne position sur un plan de
//      la table) -> utile pour que l'arbitre ou le joueur remette
//      physiquement les billes au bon endroit en un coup d'oeil.
// =====================================================================
class BallMapRecorder
{
public:
    explicit BallMapRecorder(cv::Size tableImageSize);

    // Sauvegarde la position de toutes les billes (etat courant du
    // BallTracker) dans un fichier .yml, avec un numero de coup et un
    // horodatage dans le nom de fichier.
    // Renvoie le chemin du fichier cree, ou "" en cas d'echec.
    std::string saveSnapshot(
        const std::map<std::string, std::vector<TrackedBall>>& currentState,
        const std::string& folderPath,
        int shotNumber
    ) const;

    // Recharge un instantane precedemment sauvegarde.
    std::map<std::string, std::vector<cv::Point2f>> loadSnapshot(const std::string& filePath) const;

    // Genere une image visuelle (schema de la table avec toutes les
    // billes dessinees a leur position), pour reference humaine.
    cv::Mat renderVisualMap(
        const std::map<std::string, std::vector<TrackedBall>>& currentState
    ) const;

private:
    cv::Size m_tableImageSize;

    // Couleur d'affichage (BGR) associee a chaque nom de bille, pour le rendu visuel.
    cv::Scalar colorForName(const std::string& colorName) const;
};
