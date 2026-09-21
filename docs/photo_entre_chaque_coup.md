# Photo entre chaque coup - exigence et decisions

Note de passation ecrite le 2026-09-21 (session Windows) pour la session Mac,
qui fera les vrais tests camera. Rien de ce qui est decrit ici n'est encore
construit.

## L'exigence (decidee par l'utilisateur)

Dans TOUS les modes (Manuel comme Automatique), les cameras doivent prendre
une photo entre chaque coup, des que les billes ne bougent plus. C'est la
raison d'etre des cameras. L'etat enregistre sert ensuite de position "cible"
pour le guide de repositionnement (apres une faute, "Remettre en place") et,
plus tard, pour le calcul de position de snooker (voir
`free_ball_snooker_detection.md`).

## Decisions

1. **Photo uniquement table degagee** : le corps du joueur (penche sur la
   table) cache des billes. On n'accepte un instantane que si la table est
   degagee (pas de personne dans le champ) ET que le nombre de billes
   detectees est coherent avec l'etat precedent.
2. **Photos 4K : on garde les 10 dernieres** (JPEG plutot que PNG, ~2 Mo au lieu
   de ~9 Mo, soit ~20 Mo au total). Les plus anciennes sont supprimees.
3. **Duree de conservation : 24 heures**, "pour l'instant" (a revoir). Toute
   photo de plus de 24 h est supprimee automatiquement. Les photos contiennent
   des personnes (RGPD si usage en club).
4. **Positions des billes pour chaque coup** : en plus des photos, on garde
   pour CHAQUE coup un petit fichier texte (nom + x/y en cm de chaque bille,
   quelques centaines d'octets, aucune personne dessus). Il n'est pas limite
   aux 10 derniers coups : il permet de repositionner ou de calculer un
   snooker meme quand la photo n'existe plus. Le code a deja
   `BallMapRecorder::saveSnapshot()` pour ce format.

## Etat actuel du code (a la date de cette note)

- `VisionGameBridge` enregistre bien un instantane par coup confirme
  (`saveSnapshot`, `m_shotCounter`), mais seulement pendant le suivi camera.
- Le bouton pour demarrer ce suivi a ete retire des telecommandes le
  2026-09-16 : le guide de repositionnement
  (`MainWindow::showRepositioningGuide`) est donc pratiquement inaccessible.
- Le guide affiche une image FIXE (prise a l'ouverture) : les fleches ne passent
  pas au vert quand on deplace les billes. A ameliorer : rafraichir environ
  toutes les secondes, et afficher de preference une vue de dessus schematique
  (positions en cm) plutot qu'une image recollee.

## Mecanisme propose (a valider cote Mac)

1. Surveiller la table a basse frequence (quelques images/s).
2. Detecter mouvement, puis environ 1 s d'immobilite complete.
3. Verifier que la table est degagee et que le nombre de billes est coherent.
4. Prendre la photo 4K, detecter les billes, enregistrer le fichier de positions
   (numero du coup), purger les photos de plus de 24 h ou au-dela des 10 dernieres.

## Points ouverts

- Comment detecter "table degagee" de facon fiable (personne dans le champ) ?
- Que faire si aucun instantane propre n'est disponible avant une faute ?
- Duree de conservation definitive et information des joueurs (RGPD).
