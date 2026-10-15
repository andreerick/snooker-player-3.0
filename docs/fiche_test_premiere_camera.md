# Fiche de test - premiere camera ELP sur le Mac

Objectif : verifier, avec UNE camera, que tout fonctionne avant de racheter
les 2 autres. A faire dans l'ordre ; noter le resultat de chaque etape
(OK / pas OK + une phrase).

## 0. Avant de commencer

- [ ] Mettre le projet a jour sur le Mac (`git pull`), compiler, lancer.
- [ ] Materiel : camera ELP, rallonge USB 3 m, adaptateur USB-A femelle -> USB-C
      (cote Mac), cable lampe/support, metre, et les marqueurs ArUco **5 et 6**
      imprimes a 100 % (`docs/planche_10_marqueurs_aruco_a_imprimer.pdf`,
      page 1).
- [ ] Brancher la camera DIRECTEMENT sur une prise USB-C du Mac (pas de hub).

## 1. La camera est-elle vue ?

1. Lancer Snooker Player, ouvrir **Parametres > Camera**.
2. Au premier lancement, macOS demande l'autorisation d'utiliser la camera :
   **Autoriser**. (Sinon : Reglages Systeme > Confidentialite > Camera.)
3. Regarder la liste "Camera 0 / 1 / 2 : detectee / non detectee".

> Attention : la camera integree du MacBook prend souvent l'index **0**. La
> camera ELP sera alors l'index **1** (ce qui tombe bien : c'est le role
> "camera 2", verticale, au centre). Noter l'index obtenu.

- Resultat attendu : une camera de plus apparait "detectee" quand on branche l'ELP.

## 2. L'image est-elle bonne ?

1. Ouvrir **Calibrer...** sur la camera ELP (ou l'outil `CalibrationTool`).
2. Verifier : image nette, pas de flou ni de deformation visible, pas
   saccadee.
3. Verifier le **sens** : si l'image est a l'envers, tourner la camera
   (ou noter qu'il faudra une correction logicielle).
4. Verifier la **lumiere** : reflets des lampes sur le tapis ? billes bien
   contrastees ?

## 3. Position physique (120 cm au-dessus de la table)

1. Fixer la camera a 120 cm (bord bas des abat-jour), verticale, au centre.
2. Mesurer la hauteur reelle et la noter (ecart tolere : quelques cm).
3. Verifier que toute la largeur de la table (178 cm) est dans l'image.

## 4. Calibration par marqueurs

1. Coller les marqueurs de la camera 2 sur le rail marron, de part et d'autre
   de la poche du milieu (voir `docs/guide_pose_marqueurs_aruco.pdf`) :
   - marqueur **5** : bord HAUT, a **droite** de la poche (cote baulk) ;
   - marqueur **6** : bord BAS, a **gauche** de la poche (cote noire).
2. **Mesurer le decalage d** : distance entre la poche du milieu et chaque
   marqueur (quelques cm, la meme pour les deux). Noter d = ______ cm.
3. Tester le positionnement **avant** de calibrer :
   `ArucoCalibrationTool --decalage <d> --check 1 <flux>`
   (index selon l'etape 1 ; sous Windows : `ArucoCalibrationTool.exe`).
4. Si les 2 marqueurs sont reconnus : calibrer, meme commande sans `--check` :
   `ArucoCalibrationTool --decalage <d> 1 <flux>`.
5. Noter les erreurs eventuelles ("marqueur non detecte", etc.).
   Si d = 0 (marqueurs face a face), l'option `--decalage` est inutile.

## 5. Mesures a relever (pour la suite)

- [ ] Taille d'une bille en pixels sur l'image 4K.
- [ ] Temps entre deux images en 4K (attendu : ~1,5 image/s).
- [ ] Temperature / echauffement de la camera apres 15 min.
- [ ] Consommation (si possible, a noter pour dimensionner l'alimentation
      du kit d'extension reseau).

## 6. Que faire en cas de probleme

| Constat | Premiere piste |
|---|---|
| Camera non detectee | Autorisation camera macOS, autre prise, autre cable |
| Image saccadee | Passer en 1080p / 720p, brancher sans hub |
| Image a l'envers | Tourner la camera ou noter la correction a coder |
| Marqueurs non reconnus | Plus de lumiere, marqueurs plus grands, mieux a plat |
| Index change apres redemarrage | Normal : voir `camera_stable_id_and_macos_port.md` |

## 7. A ne PAS faire encore

- Ne pas racheter les 2 autres cameras avant d'avoir valide les points 1 a 5.
- Ne pas brancher le kit USB-reseau avant ce test : le premier test se fait
  en direct, avec le cable simple.
