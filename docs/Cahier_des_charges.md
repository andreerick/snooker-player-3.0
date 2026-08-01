# Cahier des charges
## MesureVision V0.1

### Objectif

Créer un logiciel capable de mesurer avec précision la position des billes d'une table de snooker à l'aide d'une caméra USB.

---

# Fonctionnalités

## Acquisition vidéo

- Connexion automatique à la caméra USB
- Affichage vidéo en temps réel
- Capture d'image

---

## Calibration

- Calibration de la caméra
- Correction de la perspective
- Conversion Pixels → Millimètres

---

## Détection

Le logiciel doit détecter automatiquement :

- les billes
- leur centre
- leur diamètre apparent

---

## Mesures

Calculer :

- distance entre deux billes
- coordonnées X
- coordonnées Y

---

## Sauvegarde

Enregistrer :

- image annotée
- coordonnées des billes
- distances calculées

---

# Matériel

Table :

- Snooker 12 pieds

Caméra :

- Delock IMX415 USB

Hauteur :

- environ 1,20 m

Support :

- impression 3D

PC :

- Windows 10 ou Windows 11

---

# Logiciels

- C++
- OpenCV
- Visual Studio 2022
- GitHub

---

# Précision visée

Après calibration :

- erreur maximale : 3 mm
- objectif : 2 mm

---

# Philosophie

Faire au plus simple, sans sacrifier la qualité.

Chaque nouvelle fonctionnalité doit être testée avant d'être intégrée au projet principal.
