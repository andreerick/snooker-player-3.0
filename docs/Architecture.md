# Architecture - Snooker Player 3.0

## Architecture générale

Snooker Player 3.0 est composé de modules indépendants.

Chaque module peut être développé et testé séparément.

---

# Modules

## MesureVision

Responsabilités :

- Connexion caméra
- Calibration
- Détection des billes
- Calcul des coordonnées
- Mesure des distances
- Sauvegarde des images

---

## Moteur de jeu

Responsabilités :

- Comptage des points
- Gestion des règles du snooker
- Gestion des fautes
- Calcul des scores

---

## Interface utilisateur

Responsabilités :

- Affichage des scores
- Affichage de la table
- Paramétrage
- Historique des coups

---

## Base de données

Responsabilités :

- Sauvegarde des matchs
- Joueurs
- Statistiques
- Historique

---

# Technologies

- C++20
- OpenCV
- Visual Studio 2022
- GitHub

---

# Philosophie

Chaque module doit pouvoir fonctionner indépendamment.

Les échanges entre modules se feront par des interfaces clairement définies.
