# Snooker Player - ou on en est (2026-09-21)

## 1. Termine, teste et sur GitHub

- **Comptage complet** : points, breaks, fautes, Miss, Free ball, fin de frame et de
  match, egalite sur la noire. Tout le reglement officiel a ete passe en revue.
- **Telecommandes** : PC 1 (complete), PC 2 (simplifiee) et telephone, avec le
  menu **Autre** (bille touchante, sorties de table, recommencer la frame,
  conceder la frame ou le match, correction arbitre).
- **Retour** multi-niveaux avec confirmation.
- **Avertissement des Miss repetes** : bandeau 1/3, 2/3, puis proposition
  d'attribuer la frame a la 3e.
- **Mode de suivi** dans les Parametres : "Manuel" (actuel), "Automatique" (a venir).
- Ecrans : joueurs, tournoi, parametres, tutoriels (a jour), reglement.
- **Calcul de position de snooker / free ball** : pret et teste, pas encore
  branche a l'ecran (il attend de vraies positions de billes).

## 2. En attente de la premiere camera dans la salle

Premiere camera ELP (IMX317, objectif 100 degres) validee : image nette, sans
deformation visible, champ mesure environ 98 degres.

A tester sur la vraie table, a 120 cm (bord bas des abat-jour) :
- la lumiere et les reflets sous les lampes ;
- la taille des billes en pixels ;
- l'orientation et l'inclinaison de la camera (carre ou marqueur imprime) ;
- la consommation electrique (a chercher sur le site d'ELP) pour dimensionner
  l'alimentation des boitiers d'extension.

**Decision : on ne rachete les 2 autres cameras qu'apres ces tests.**

## 3. Pour la session Mac (notes deja sur GitHub, dossier docs/)

- `camera_stable_id_and_macos_port.md` : identifier chaque camera de facon
  stable (l'ordre 0/1/2 n'est pas garanti).
- `photo_entre_chaque_coup.md` : une photo 4K a chaque coup, table degagee, 10
  dernieres photos gardees, suppression apres 24 h, positions de tous les coups.
- `free_ball_snooker_detection.md` : suggestion "Vous semblez etre snooke".

## 4. Mis de cote volontairement

Voix plus naturelle sous Windows (le Mac est deja bon), cameras reseau (IP/PoE),
IA de detection (seulement si OpenCV seul echoue), plusieurs tables sur un
Mac, fiche d'installation par salle, snooker a 4 joueurs et a 6 rouges.

## Regle de travail Windows / Mac

Toujours passer par `sync.sh` (jamais de copie directe de fichiers) pour ne
pas ecraser le travail de l'autre session.
