# Detection de la position de snooker (Free Ball) - note de passation

Ecrite le 2026-09-20 (session Windows) pour la session Mac, qui fera les
vrais tests camera. Le calcul est pret et teste ; il manque seulement la
source de positions reelles pour le brancher a l'ecran.

## Le principe (reglement Sect. 2 §13 et §17, Sect. 3 §12)

Une "free ball" n'est pas une bille mais une SITUATION : le joueur qui arrive
est snooke apres une FAUTE de l'adversaire. Snooke = la blanche ne peut pas
toucher, en ligne droite, une bille jouable sur ses DEUX bords extremes, a
cause d'autres billes non jouables (une bande ne snooke jamais). Le joueur
peut alors designer n'importe quelle bille comme bille jouable.

## Ce qui existe : `SnookerGeometry.h/.cpp` (commit 57a5358)

Module pur C++ (aucune dependance Qt/OpenCV), dans la bibliotheque
`MesureVision`. Entrees en **centimetres dans un repere commun de la table**
(le meme pour toutes les billes : c'est a l'appelant de convertir les
positions image avec `CameraCalibration`).

```cpp
SnookerAssessment assessSnookerOnTable(
    const PositionedBall& cue,                 // "Blanche"
    const std::vector<PositionedBall>& allBalls, // toutes les autres billes
    const std::string& requiredBall,           // Frame::getRequiredBall().getName()
    double toleranceCm = 0.5);
```

Sortie : `NotSnookered` / `Borderline` / `Snookered`, une marge de
degagement (cm) et le nom de la bille qui gene. La tolerance (0,5 cm par
defaut, a regler selon la precision reelle des caméras) rend `Borderline` les
cas a quelques mm pres : ce sont ceux a SUGGERER a l'arbitre.

Verifie sur 12 situations calculees a la main (obstacle au milieu, decale,
derriere la cible, bille jouable devant une autre, blanche au contact,
"n'importe quelle couleur", cas limite). Il n'y a pas de test automatise
dans le depot : a ajouter si le module evolue.

## Performance (mesuree)

~19 microsecondes pour une table complete de 22 billes (build non optimise).
Le calcul n'est jamais le maillon lent. Le delai reel sera : attendre que les
billes soient immobiles (~0,5 a 1 s, estimation) + photo et detection sur les
3 cameras (a MESURER sur le Mac) + calibration.

## Ce qui reste a faire (cote Mac / vision)

1. Fournir des positions REELLES en cm (blanche + toutes les billes) apres
   detection et calibration. Le tracker actuel (`BallTracker`) travaille en
   pixels image : la conversion en cm de table est a faire.
2. Detecter le bon moment : billes immobiles apres une FAUTE.
3. Appeler `assessSnookerOnTable` et afficher, si le resultat est `Snookered`
   ou `Borderline`, un bandeau du type :
   **"Vous semblez etre snooke, possibilite de Free ball"**.
   Toujours une SUGGESTION : l'arbitre decide et arme lui-meme le bouton
   Free ball (deja existant).
4. Mesurer le delai reel et la precision necessaire (regler la tolerance).

## Limites connues

- Bille en main (Sect. 2 §17(a)) : il faut tester depuis TOUS les points du
  "D" ; non gere (l'appelant doit fournir une position precise de blanche).
- Regle des rouges (§17c) et bille "realisant le snooker" (§17b) : le module
  dit seulement snooke ou non, pas quelle bille est reputee faire le snooker
  au sens du reglement (il renvoie la plus contraignante).
- La precision depend entierement des positions detectees.
