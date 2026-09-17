# Identification stable des cameras + portage macOS

Note de passation ecrite le 2026-09-18 (session Windows) pour la session
Claude Code qui travaille sur le Mac. Contexte : il y aura une version
Windows ET une version macOS de Snooker Player, et les vrais tests
camera/vision se feront sur le Mac (c'est la que vit le montage physique a
3 cameras).

## Le probleme

`ArucoCalibrationTool.cpp` ouvre chaque camera par un simple index entier :

```cpp
cv::VideoCapture cap(std::stoi(source)); // source = "0", "1" ou "2"
```

Le manuel de calibration (`manuel_calibration_cameras.docx`) attribue un
role PHYSIQUE precis a chaque index :

- Camera 0 : bout "baulk" de la table (billes jaune/verte/marron), inclinee ~4°
- Camera 1 : verticale, centree au-dessus de la bille bleue (camera de reference)
- Camera 2 : bout "noire" de la table (billes rose/noire), inclinee ~4° (sens oppose)

Ce mapping index -> role physique n'est PAS garanti stable par l'OS : apres
un redemarrage, un debranchement/rebranchement, ou juste un ordre de
detection different au demarrage, la camera qui repondait a l'index 0 peut
tres bien repondre a l'index 1 la fois suivante. Si ca arrive, l'appli
applique silencieusement la mauvaise calibration a la mauvaise camera —
aucune erreur visible, juste une detection qui devient fausse.

Le montage physique actuel aggrave le risque : les 3 cameras USB passent
par des extendeurs USB-vers-Ethernet-vers-USB (cable RJ45 sur 3-5m) avant
d'arriver sur le Mac en 3 sorties USB-C separees — encore un intermediaire
dont l'ordre de detection au boot n'est pas garanti.

## Ce qu'il faudrait construire

Remplacer l'ouverture par index brut par une identification basee sur un
identifiant STABLE, propre a chaque camera physique, plutot que sur l'ordre
de detection.

Sur macOS, ca passe par AVFoundation : chaque `AVCaptureDevice` a un
`uniqueID` qui reste generalement stable pour un meme port physique. Ca
demande du code Objective-C++ (`.mm`) pour enumerer les peripheriques et
faire correspondre "quel uniqueID physique = quel role (0/1/2 du manuel)",
puis retrouver quel index OpenCV/`cv::VideoCapture` correspond a cet
uniqueID au demarrage.

Sur Windows, l'equivalent serait une enumeration DirectShow/Media
Foundation par device path ou numero de serie (pas fait non plus a ce
jour — le code actuel est deja index-brut sur les deux plateformes).

Idee d'approche, a valider/adapter cote Mac :
1. Au premier lancement (ou via un ecran de config), enumerer les
   peripheriques disponibles avec leur identifiant stable.
2. Demander a l'utilisateur (ou detecter automatiquement via un motif
   visuel/marqueur ArUco specifique a chaque camera) quel identifiant
   correspond a quel role (0/1/2).
3. Sauvegarder cette association (fichier de config, genre `cameras.yml` a
   cote des calibrations existantes `camera_<N>.yml`).
4. Au demarrage normal, retrouver l'index `cv::VideoCapture` courant a
   partir de l'identifiant stable sauvegarde, plutot que de supposer que
   l'ordre n'a pas change.

Pas encore decide qui code quoi (Mac ou Windows) — cette note sert juste a
transferer le contexte, pas a figer une repartition du travail.

## References

- `docs/manuel_calibration_cameras.docx` — geometrie/roles des 3 cameras (authoritative)
- `ArucoCalibrationTool.cpp` — ou l'ouverture par index brut se trouve actuellement
- Montage physique : pince de fixation sur le rail + boitier caméra + bras flexible, cameras reliees au Mac (4 ports USB-C) via des extendeurs USB-vers-Ethernet-vers-USB sur 3-5m
