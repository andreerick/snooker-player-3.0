# Impression des marqueurs ArUco - notes pratiques

Ce que l'on a appris en imprimant l'annexe du manuel (pages 8 et 9 de
`manuel_calibration_cameras.pdf`, 10 marqueurs de 6 cm).

## A faire a chaque impression
- Imprimer a **100 % / taille reelle**, jamais "ajuster a la page" (A4).
- Apres impression, **mesurer a la regle un carre noir : 60 x 60 mm**.
- Decouper en suivant les pointillees, en gardant le bord blanc.
- La taille exacte n'a pas d'importance pour la detection ni pour la calibration
  (seuls comptent le numero du marqueur et la position de son centre sur la
  table), mais un marqueur tres deforme peut fausser le controle "carre" de l'outil.

## Premiere impression (imprimante Canon MX340, depuis le PC Windows)
- Mesure : marqueur **58 mm de large x 61,5 mm de haut** (attendu 60 x 60) ;
  decoupe 82 x 84,5 mm (attendu environ 85,6 x 82,6).
- Cause : l'imprimante / son pilote reduit la largeur d'environ 3 % et etire la
  hauteur d'environ 2,5 %. Le fichier PDF, lui, est exact (60,0 x 60,0 mm).
- **Correction pour la prochaine impression sur cette imprimante** : avant
  d'envoyer la page, l'etirer de **+3,4 % en largeur** (60/58) et la reduire de
  **2,4 % en hauteur** (60/61,5), puis remesurer un carre noir et ajuster.
- Cette correction est propre a l'imprimante : ne pas la mettre dans le PDF du
  depot (un autre appareil, ou le Mac, sortirait des marqueurs faux).

## Cartouche noire vide
- On peut imprimer en couleur : remplacer le noir pur par un bleu tres fonce
  (RVB 8, 8, 60). Le pilote le fabrique avec les encres de couleur. Le contraste
  avec le blanc reste tres fort pour la camera.
- Cartouches du **Canon PIXMA MX340** : noire **PG-510** (standard) ou **PG-512**
  (grande capacite) ; couleur **CL-511** (standard) ou **CL-513** (grande
  capacite). Les PG-40 / PG-50 sont pour le MX310, pas pour le MX340.
  Prix relevees en octobre 2026 : PG-510 originale environ 20 EUR ;
  PG-512 compatible environ 21 EUR ; verifier la reference ecrite sur la
  cartouche en place avant d'acheter.
