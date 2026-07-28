#pragma once

#include "../GameManager.h"
#include "../Frame.h"
#include "../Ball.h"
#include "../camera/CameraFusion.h"

#include <string>

namespace Api {

// Serialise l'etat courant du jeu en JSON
std::string gameStateToJson(GameManager& manager);

// Serialise l'historique des coups en JSON
std::string shotHistoryToJson(const ShotHistory& history);

// Serialise l'etat fusionne des cameras en JSON
std::string fusedStateToJson(const Camera::FusedGameState& state);

// Parse une requete de coup (retourne la bille jouee)
// Format attendu: {"ball_name":"Noir","ball_value":7}
Ball parseShotRequest(const std::string& json);

// Retourne un objet JSON d'erreur
std::string errorJson(const std::string& message);

// Retourne un objet JSON de succes
std::string successJson(const std::string& message);

} // namespace Api
