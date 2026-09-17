#include "ExerciseDialog.h"
#include "UiUtils.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPainter>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kWhite = "#f5f5f5";

    const QColor kColorRed("#ff3b30");
    const QColor kColorBlack("#161616");
    const QColor kColorPink("#ff4fa3");
    const QColor kColorBlue("#1565ff");
    const QColor kColorBrown("#8b4513");
    const QColor kColorYellow("#ffcc00");
    const QColor kColorGreenBall("#00c853");
    const QColor kColorCue("#f5f5f5");
    const QColor kColorMarker("#d8c9a3");

    struct Exercise
    {
        QString title;
        QString html;
    };

    // Utilitaires communs a tous les diagrammes de mise en place, extraits
    // pour eviter de redupliquer le trace table/tapis/poches (identique
    // partout) dans chaque nouvelle fonction drawXxxDiagram ajoutee pour
    // les exercices 3 et suivants. Les diagrammes du Line-Up et du T
    // (exercices 1 et 2, ecrits avant l'ajout de cet utilitaire) restent
    // volontairement inchanges pour ne pas risquer de regression sur des
    // rendus deja valides par l'utilisateur.
    QPixmap createTableBase(int width, int height, QRectF& clothRectOut)
    {
        QPixmap pixmap(width, height);
        pixmap.fill(Qt::transparent);
        {
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);

            qreal margin = width * 0.03;
            QRectF cushionRect(margin, margin, width - 2 * margin, height - 2 * margin);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#4a2c17"));
            painter.drawRoundedRect(cushionRect, 8, 8);

            qreal clothMargin = width * 0.028;
            QRectF clothRect(
                cushionRect.left() + clothMargin, cushionRect.top() + clothMargin,
                cushionRect.width() - 2 * clothMargin, cushionRect.height() - 2 * clothMargin
            );
            painter.setBrush(QColor("#1a6b1a"));
            painter.drawRect(clothRect);

            qreal pocketR = width * 0.016;
            painter.setBrush(QColor("#111111"));
            for (const QPointF& p : {
                cushionRect.topLeft(), cushionRect.topRight(),
                cushionRect.bottomLeft(), cushionRect.bottomRight(),
                QPointF(cushionRect.center().x(), cushionRect.top()),
                QPointF(cushionRect.center().x(), cushionRect.bottom())
            })
            {
                painter.drawEllipse(p, pocketR, pocketR);
            }

            clothRectOut = clothRect;
        }
        return pixmap;
    }

    // Regroupe les coordonnees normalisees -> pixels et le dessin d'une
    // bille, plus le motif recurrent de la zone du "D" (billes jaune,
    // marron, verte et l'arc), pour les diagrammes ajoutes a partir de
    // l'exercice 3.
    struct BallPainter
    {
        QPainter& painter;
        QRectF clothRect;
        qreal ballR;

        QPointF at(qreal nx, qreal ny) const
        {
            return QPointF(
                clothRect.left() + nx * clothRect.width(),
                clothRect.top() + ny * clothRect.height()
            );
        }

        void ball(QPointF p, const QColor& color) const
        {
            painter.setPen(QPen(QColor("#000000"), 1));
            painter.setBrush(color);
            painter.drawEllipse(p, ballR, ballR);
        }

        // Dessine l'arc + la ligne de depart + jaune/marron/vert, et
        // renvoie la position du point marron (repere le plus utilise
        // par les diagrammes).
        QPointF dZone() const
        {
            QPointF brownPt = at(0.82, 0.5);
            QPointF yellowPt = at(0.82, 0.30);
            QPointF greenPt = at(0.82, 0.70);

            painter.setPen(QPen(QColor(255, 255, 255, 160), 1.2));
            painter.setBrush(Qt::NoBrush);
            qreal dRadius = clothRect.width() * 0.085;
            QRectF dRect(brownPt.x() - dRadius, brownPt.y() - dRadius, dRadius * 2, dRadius * 2);
            // Balaie vers la DROITE (span negatif) : le "D" doit se creuser
            // vers la bande la plus proche (cote mouche marron), pas vers
            // le paquet de rouges -- un span positif ici le faisait bomber
            // du mauvais cote (repere sur capture d'ecran, voir discussion
            // avec l'utilisateur du 2026-09-14).
            painter.drawArc(dRect, 90 * 16, -180 * 16);
            painter.drawLine(QPointF(brownPt.x(), clothRect.top()), QPointF(brownPt.x(), clothRect.bottom()));

            ball(brownPt, kColorBrown);
            ball(yellowPt, kColorYellow);
            ball(greenPt, kColorGreenBall);
            return brownPt;
        }
    };

    // Diagramme de mise en place de "L'alignement des rouges" (le
    // classique anglais "The Line-Up") : table vue de dessus, couleurs a
    // leurs points habituels, les 15 rouges alignees au centre. Dessine
    // en vectoriel (pas d'image source) -- voir le schema de reference
    // partage par l'utilisateur pour les proportions.
    QPixmap drawLineUpDiagram(int width, int height)
    {
        QPixmap pixmap(width, height);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        qreal margin = width * 0.03;
        QRectF cushionRect(margin, margin, width - 2 * margin, height - 2 * margin);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#4a2c17"));
        painter.drawRoundedRect(cushionRect, 8, 8);

        qreal clothMargin = width * 0.028;
        QRectF clothRect(
            cushionRect.left() + clothMargin, cushionRect.top() + clothMargin,
            cushionRect.width() - 2 * clothMargin, cushionRect.height() - 2 * clothMargin
        );
        painter.setBrush(QColor("#1a6b1a"));
        painter.drawRect(clothRect);

        // Pockets (4 coins + 2 milieux de grand cote).
        qreal pocketR = width * 0.016;
        painter.setBrush(QColor("#111111"));
        for (const QPointF& p : {
            cushionRect.topLeft(), cushionRect.topRight(),
            cushionRect.bottomLeft(), cushionRect.bottomRight(),
            QPointF(cushionRect.center().x(), cushionRect.top()),
            QPointF(cushionRect.center().x(), cushionRect.bottom())
        })
        {
            painter.drawEllipse(p, pocketR, pocketR);
        }

        auto toPt = [&](qreal nx, qreal ny)
        {
            return QPointF(
                clothRect.left() + nx * clothRect.width(),
                clothRect.top() + ny * clothRect.height()
            );
        };

        qreal ballR = clothRect.height() * 0.032;
        auto drawBall = [&](QPointF p, const QColor& color)
        {
            painter.setPen(QPen(QColor("#000000"), 1));
            painter.setBrush(color);
            painter.drawEllipse(p, ballR, ballR);
        };

        // Zone du "D" (billes jaune/vert/marron), a droite.
        QPointF brownPt = toPt(0.82, 0.5);
        QPointF yellowPt = toPt(0.82, 0.5 - 0.20);
        QPointF greenPt = toPt(0.82, 0.5 + 0.20);

        painter.setPen(QPen(QColor(255, 255, 255, 160), 1.2));
        painter.setBrush(Qt::NoBrush);
        qreal dRadius = clothRect.width() * 0.085;
        QRectF dRect(brownPt.x() - dRadius, brownPt.y() - dRadius, dRadius * 2, dRadius * 2);
        // Span negatif : le "D" doit se creuser vers la bande la plus
        // proche, pas vers le paquet de rouges (voir meme correction dans
        // BallPainter::dZone() ci-dessus).
        painter.drawArc(dRect, 90 * 16, -180 * 16);
        painter.drawLine(QPointF(brownPt.x(), clothRect.top()), QPointF(brownPt.x(), clothRect.bottom()));

        QPointF bluePt = toPt(0.52, 0.5);
        QPointF pinkPt = toPt(0.67, 0.5);
        QPointF blackPt = toPt(0.14, 0.5);

        // Les 15 rouges, alignees au centre de la table.
        const int redCount = 15;
        const qreal lineStartX = 0.06;
        const qreal lineEndX = 0.60;
        for (int i = 0; i < redCount; ++i)
        {
            qreal t = static_cast<qreal>(i) / (redCount - 1);
            qreal nx = lineStartX + t * (lineEndX - lineStartX);
            drawBall(toPt(nx, 0.5), QColor("#ff3b30"));
        }

        drawBall(blackPt, QColor("#161616"));
        drawBall(pinkPt, QColor("#ff4fa3"));
        drawBall(bluePt, QColor("#1565ff"));
        drawBall(brownPt, QColor("#8b4513"));
        drawBall(yellowPt, QColor("#ffcc00"));
        drawBall(greenPt, QColor("#00c853"));

        // Bille blanche de depart + trajectoire indicative.
        QPointF cuePt = toPt(0.10, 0.16);
        drawBall(cuePt, QColor("#f5f5f5"));
        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, toPt(0.07, 0.5 - 0.05));

        return pixmap;
    }

    // Diagramme de mise en place de "The T" : une barre de 10 rouges
    // traverse toute la largeur de la table (5 de chaque cote de l'axe
    // de croisement), croisee par une barre de 5 rouges qui s'etend de
    // cet axe vers la noire -- voir le schema de reference partage par
    // l'utilisateur pour les proportions et l'orientation.
    QPixmap drawTShapeDiagram(int width, int height)
    {
        QPixmap pixmap(width, height);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        qreal margin = width * 0.03;
        QRectF cushionRect(margin, margin, width - 2 * margin, height - 2 * margin);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#4a2c17"));
        painter.drawRoundedRect(cushionRect, 8, 8);

        qreal clothMargin = width * 0.028;
        QRectF clothRect(
            cushionRect.left() + clothMargin, cushionRect.top() + clothMargin,
            cushionRect.width() - 2 * clothMargin, cushionRect.height() - 2 * clothMargin
        );
        painter.setBrush(QColor("#1a6b1a"));
        painter.drawRect(clothRect);

        qreal pocketR = width * 0.016;
        painter.setBrush(QColor("#111111"));
        for (const QPointF& p : {
            cushionRect.topLeft(), cushionRect.topRight(),
            cushionRect.bottomLeft(), cushionRect.bottomRight(),
            QPointF(cushionRect.center().x(), cushionRect.top()),
            QPointF(cushionRect.center().x(), cushionRect.bottom())
        })
        {
            painter.drawEllipse(p, pocketR, pocketR);
        }

        auto toPt = [&](qreal nx, qreal ny)
        {
            return QPointF(
                clothRect.left() + nx * clothRect.width(),
                clothRect.top() + ny * clothRect.height()
            );
        };

        qreal ballR = clothRect.height() * 0.032;
        auto drawBall = [&](QPointF p, const QColor& color)
        {
            painter.setPen(QPen(QColor("#000000"), 1));
            painter.setBrush(color);
            painter.drawEllipse(p, ballR, ballR);
        };

        // Zone du "D" (billes jaune/vert/marron), a droite -- identique
        // au diagramme du Line-Up.
        QPointF brownPt = toPt(0.82, 0.5);
        QPointF yellowPt = toPt(0.82, 0.5 - 0.20);
        QPointF greenPt = toPt(0.82, 0.5 + 0.20);

        painter.setPen(QPen(QColor(255, 255, 255, 160), 1.2));
        painter.setBrush(Qt::NoBrush);
        qreal dRadius = clothRect.width() * 0.085;
        QRectF dRect(brownPt.x() - dRadius, brownPt.y() - dRadius, dRadius * 2, dRadius * 2);
        // Span negatif : le "D" doit se creuser vers la bande la plus
        // proche, pas vers le paquet de rouges (voir meme correction dans
        // BallPainter::dZone() ci-dessus).
        painter.drawArc(dRect, 90 * 16, -180 * 16);
        painter.drawLine(QPointF(brownPt.x(), clothRect.top()), QPointF(brownPt.x(), clothRect.bottom()));

        QPointF bluePt = toPt(0.56, 0.5);
        QPointF blackPt = toPt(0.12, 0.5);

        // Axe de croisement du "T" : a mi-chemin entre la noire et la
        // bleue le long de la longueur, au centre de la largeur.
        qreal crossX = 0.36;

        // Barre verticale (10 rouges, toute la largeur de la table,
        // 5 de chaque cote de l'axe).
        const int vertCount = 10;
        for (int i = 0; i < vertCount; ++i)
        {
            qreal t = static_cast<qreal>(i) / (vertCount - 1);
            qreal ny = 0.14 + t * (0.86 - 0.14);
            drawBall(toPt(crossX, ny), QColor("#ff3b30"));
        }

        // Barre horizontale (5 rouges, de l'axe vers la noire).
        const int horizCount = 5;
        for (int i = 1; i <= horizCount; ++i)
        {
            qreal t = static_cast<qreal>(i) / horizCount;
            qreal nx = crossX - t * (crossX - 0.16);
            drawBall(toPt(nx, 0.5), QColor("#ff3b30"));
        }

        // Repere au point de croisement (ni rouge ni bille reelle, juste
        // un marqueur visuel -- voir le schema de reference).
        drawBall(toPt(crossX, 0.5), QColor("#d8c9a3"));

        drawBall(blackPt, QColor("#161616"));
        drawBall(bluePt, QColor("#1565ff"));
        drawBall(brownPt, QColor("#8b4513"));
        drawBall(yellowPt, QColor("#ffcc00"));
        drawBall(greenPt, QColor("#00c853"));

        // Bille blanche de depart (variante 1 : entre les rouges et la
        // bleue) + trajectoire indicative.
        QPointF cuePt = toPt((crossX + 0.56) / 2, 0.5);
        drawBall(cuePt, QColor("#f5f5f5"));
        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(toPt(0.03, 0.06), cuePt);

        return pixmap;
    }

    // Diagramme "Controle de la bille blanche" : bleue sur son point,
    // blanche a ~25cm en ligne vers une poche du milieu, et un repere
    // vertical (5 points) a cote de la blanche pour visualiser les
    // hauteurs de frappe testees.
    QPixmap drawCueBallControlDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        bp.dZone();
        QPointF bluePt = bp.at(0.52, 0.5);
        bp.ball(bluePt, kColorBlue);

        QPointF cuePt = bp.at(0.52, 0.5 - 0.24);
        bp.ball(cuePt, kColorCue);

        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, QPointF(clothRect.center().x(), clothRect.top()));

        // Repere des 5 points de frappe verticaux, a cote de la blanche.
        qreal markX = cuePt.x() - bp.ballR * 2.4;
        for (int i = -2; i <= 2; ++i)
        {
            QPointF markPt(markX, cuePt.y() + i * bp.ballR * 0.55);
            painter.setPen(QPen(QColor("#000000"), 1));
            painter.setBrush(QColor(245, 245, 245, i == 0 ? 255 : 150));
            painter.drawEllipse(markPt, bp.ballR * 0.22, bp.ballR * 0.22);
        }

        return pixmap;
    }

    // Diagramme "Potting Long Blues" : bleue sur son point, blanche
    // alignee sur la meme ligne que la bleue et la poche de coin, a bonne
    // distance (l'exercice progresse ensuite en reculant la blanche).
    QPixmap drawLongBluesDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        bp.dZone();
        QPointF bluePt = bp.at(0.52, 0.5);
        bp.ball(bluePt, kColorBlue);

        QPointF cuePt = bp.at(0.72, 0.68);
        bp.ball(cuePt, kColorCue);

        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, bluePt);
        painter.drawLine(bluePt, clothRect.topLeft());

        return pixmap;
    }

    // Diagramme "Se mettre en place sur les spots" : blanche sur le point
    // marron, noire contre la bande directement derriere le point noir
    // (alignee noir-rose-bleu-marron) -- la blanche doit revenir droit a
    // son point de depart apres avoir touche la bande.
    QPixmap drawSpotsDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        QPointF brownPt = bp.dZone();
        // La blanche occupe le point marron (dessinee par-dessus).
        bp.ball(brownPt, kColorCue);

        QPointF blackPt = bp.at(0.02, 0.5);
        bp.ball(blackPt, kColorBlack);

        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(brownPt, QPointF(blackPt.x() + bp.ballR, blackPt.y()));

        return pixmap;
    }

    // Diagramme "Empocher la bille noire" : noire sur son point, blanche
    // a mi-chemin vers la bande laterale en position "1/2 bille" ; 2
    // positions alternatives (1/4, 3/4) indiquees en transparence.
    QPixmap drawBlackPottingDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        bp.dZone();
        QPointF blackPt = bp.at(0.14, 0.5);
        bp.ball(blackPt, kColorBlack);

        QPointF cuePt = bp.at(0.26, 0.68);
        bp.ball(cuePt, kColorCue);

        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, clothRect.bottomLeft());

        painter.setPen(QPen(QColor("#000000"), 1));
        painter.setBrush(QColor(245, 245, 245, 120));
        painter.drawEllipse(bp.at(0.26, 0.5 + 0.05), bp.ballR, bp.ballR);
        painter.drawEllipse(bp.at(0.28, 0.5 + 0.32), bp.ballR, bp.ballR);

        return pixmap;
    }

    // Diagramme "Potting direct des rouges" : les 3 niveaux (courte,
    // moyenne et longue distance depuis le "D") superposes sur un seul
    // schema, chaque colonne de rouges etiquetee 1/2/3.
    QPixmap drawRedsPottingDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.028};

        bp.dZone();
        QPointF bluePt = bp.at(0.52, 0.5);
        bp.ball(bluePt, kColorBlue);
        QPointF pinkPt = bp.at(0.30, 0.5);
        bp.ball(pinkPt, kColorPink);
        QPointF blackPt = bp.at(0.10, 0.5);
        bp.ball(blackPt, kColorBlack);

        // Les 3 niveaux restent tous en rouge plein (une transparence sur
        // fond vert virait au marron et brouillait la lecture) ; seule
        // l'etiquette numerotee sous chaque colonne distingue les niveaux.
        struct Level { qreal nx; QString label; };
        const Level levels[] = {
            { 0.67, "1" },
            { 0.52, "2" },
            { 0.38, "3" },
        };
        for (const Level& lvl : levels)
        {
            for (int i = -7; i <= 7; ++i)
            {
                qreal ny = 0.5 + i * 0.052;
                bp.ball(bp.at(lvl.nx, ny), kColorRed);
            }
            painter.setPen(QColor(255, 255, 255, 220));
            painter.drawText(bp.at(lvl.nx, 0.5 + 8 * 0.052).toPoint(), lvl.label);
        }

        QPointF cuePt = bp.at(0.80, 0.5);
        bp.ball(cuePt, kColorCue);

        return pixmap;
    }

    // Diagramme "Potting Long Reds" : rose sur son point, une rouge a
    // ~30cm alignee vers la poche de fond, blanche a mi-chemin entre la
    // ligne bleue et la bande de fond.
    QPixmap drawLongRedsDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        bp.dZone();
        QPointF bluePt = bp.at(0.52, 0.5);
        bp.ball(bluePt, kColorBlue);
        QPointF pinkPt = bp.at(0.30, 0.5);
        bp.ball(pinkPt, kColorPink);
        QPointF blackPt = bp.at(0.10, 0.5);
        bp.ball(blackPt, kColorBlack);

        QPointF redPt = bp.at(0.40, 0.5);
        bp.ball(redPt, kColorRed);

        QPointF cuePt = bp.at(0.62, 0.5);
        bp.ball(cuePt, kColorCue);

        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, redPt);
        painter.drawLine(redPt, QPointF(clothRect.left(), redPt.y()));

        return pixmap;
    }

    // Diagramme "Sortir des situations de snooker faciles" : bille objet
    // (bleue) et bille bloquante pres de la bande, blanche a cote --
    // l'image miroir de la bille objet (pointillee, au-dessus de la
    // bande) et la ligne de visee blanche->miroir->bille objet sont
    // calculees geometriquement pour illustrer la technique du miroir.
    QPixmap drawSnookerEscapeDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        QPointF targetPt = bp.at(0.18, 0.16);
        bp.ball(targetPt, kColorBlue);
        QPointF blockerPt = bp.at(0.18, 0.34);
        bp.ball(blockerPt, kColorMarker);
        QPointF cuePt = bp.at(0.32, 0.34);
        bp.ball(cuePt, kColorCue);

        QPointF mirrorPt(targetPt.x(), clothRect.top() - (targetPt.y() - clothRect.top()));
        painter.setPen(QPen(QColor(255, 255, 255, 130), 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(mirrorPt, bp.ballR, bp.ballR);

        qreal t = (clothRect.top() - cuePt.y()) / (mirrorPt.y() - cuePt.y());
        QPointF cushionAimPt(cuePt.x() + t * (mirrorPt.x() - cuePt.x()), clothRect.top());

        painter.setPen(QPen(QColor(255, 255, 255, 100), 1, Qt::DashLine));
        painter.drawLine(cuePt, mirrorPt);
        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, cushionAimPt);
        painter.drawLine(cushionAimPt, targetPt);

        return pixmap;
    }

    // Diagramme "Coups de securite au snooker sur les rouges" : noire,
    // rose, bleue a leurs points, une rouge alignee sur la noire, blanche
    // juste derriere -- eventail de lignes vers 6 positions cibles le
    // long de la bande de fond.
    QPixmap drawRedSafetyDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        bp.dZone();
        QPointF blackPt = bp.at(0.14, 0.5);
        bp.ball(blackPt, kColorBlack);
        QPointF pinkPt = bp.at(0.30, 0.5);
        bp.ball(pinkPt, kColorPink);
        QPointF bluePt = bp.at(0.52, 0.5);
        bp.ball(bluePt, kColorBlue);

        QPointF redPt = bp.at(0.14, 0.5 - 0.10);
        bp.ball(redPt, kColorRed);

        QPointF cuePt = bp.at(0.14, 0.5 - 0.22);
        bp.ball(cuePt, kColorCue);

        painter.setPen(QPen(QColor(255, 255, 255, 130), 1.2));
        for (int i = 0; i < 6; ++i)
        {
            qreal nx = 0.06 + i * 0.05;
            painter.drawLine(redPt, bp.at(nx, 0.92));
        }

        return pixmap;
    }

    // Diagramme "Empocher plusieurs bleues" : couleurs a leurs points, 3
    // rouges de chaque cote du point bleu (alignees avec marron et rose),
    // blanche placee pour enchainer rouge -> bleue -> rouge suivante.
    QPixmap drawMultipleBluesDiagram(int width, int height)
    {
        QRectF clothRect;
        QPixmap pixmap = createTableBase(width, height, clothRect);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        BallPainter bp{painter, clothRect, clothRect.height() * 0.032};

        bp.dZone();
        QPointF blackPt = bp.at(0.14, 0.5);
        bp.ball(blackPt, kColorBlack);
        QPointF pinkPt = bp.at(0.30, 0.5);
        bp.ball(pinkPt, kColorPink);
        QPointF bluePt = bp.at(0.52, 0.5);
        bp.ball(bluePt, kColorBlue);

        const int sideCount = 3;
        const qreal spacing = 0.055;
        for (int i = 1; i <= sideCount; ++i)
        {
            bp.ball(bp.at(0.52 - i * spacing, 0.5), kColorRed);
            bp.ball(bp.at(0.52 + i * spacing, 0.5), kColorRed);
        }

        QPointF cuePt = bp.at(0.52, 0.5 + 0.28);
        bp.ball(cuePt, kColorCue);

        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        painter.drawLine(cuePt, bluePt);

        return pixmap;
    }

    // Contenu corrige : le texte source (fourni par l'utilisateur, issu
    // d'une traduction automatique visiblement ratee d'un article anglais
    // sur "The Line-Up") parlait a tort de "joueurs"/"equipe" alors qu'il
    // s'agit de placer des BILLES (rouges + couleurs) sur la table, pas
    // des joueurs -- terminologie corrigee ici, contenu instructif
    // conserve et juste restructure (Mise en place / Deroulement /
    // Conseils), voir discussion avec l'utilisateur avant integration.
    const QVector<Exercise>& exercises()
    {
        static const QVector<Exercise> data = {
            {
                "Line-Up",
                "<p>Probablement l'exercice le plus utilise par les joueurs professionnels pour "
                "se detendre et travailler les bases : empocher rouge apres couleur, comme en "
                "match, mais avec les rouges deja parfaitement placees pour ne pas avoir a "
                "gerer un casse imparfait.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez toutes les couleurs a leurs points habituels.</li>"
                "<li>Alignez les 15 rouges au centre de la table, dans le prolongement de la "
                "ligne noire-rose-bleue-marron :"
                "<ul style='margin:4px 0; padding-left:20px'>"
                "<li>2 rouges sous la noire</li>"
                "<li>4 rouges entre la noire et la rose</li>"
                "<li>7 rouges entre la rose et la bleue</li>"
                "<li>2 rouges entre la bleue et la marron</li>"
                "</ul></li>"
                "<li>Choisissez la rouge de depart (souvent la 2e apres la noire).</li>"
                "</ol>"
                "<p style='color:#7a7f87'>D'autres repartitions existent (1 ou 3 rouges sous la "
                "noire, etc.) : le choix importe peu, mais gardez toujours la MEME repartition "
                "d'une seance a l'autre pour pouvoir comparer vos progres.</p>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Empochez une rouge puis une couleur, comme en jeu normal.</li>"
                "<li>Une fois toutes les rouges empochees, terminez avec les couleurs dans "
                "l'ordre habituel.</li>"
                "<li>En cas d'echec sur une bille, remettez tout en place et recommencez depuis "
                "le debut -- cette discipline aide a rester concentre.</li>"
                "<li>Notez votre score a chaque seance pour suivre vos progres dans le temps.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Imaginez la table divisee en 3 bandes egales dans la longueur : la bille "
                "visee doit s'arreter dans l'une d'elles pour garder un angle jouable sur la "
                "suivante -- jamais collee a la bande, ni trop pres des billes restantes.</li>"
                "<li>Une fois a l'aise, essayez de commencer par la rouge la plus eloignee "
                "(cote bleue) plutot que la plus proche : ca muscle la concentration au moment "
                "de choisir sa bille.</li>"
                "<li>Certains joueurs preferent ne pas compter leur score et se concentrer "
                "juste sur le rythme (combien de rouges avant une erreur), pour se vider "
                "l'esprit plutot que performer. Les deux approches sont valables -- l'important "
                "est la regularite de la pratique.</li>"
                "</ul>"
            },
            {
                "The T",
                "<p>Legerement plus exigeant que le Line-Up : les rouges forment un \"T\" plutot "
                "qu'une simple ligne, obligeant a jouer dans un espace plus restreint. S'adresse "
                "surtout aux joueurs intermediaires/avances qui veulent muscler les coups utiles "
                "a la construction d'une serie.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez toutes les couleurs a leurs points habituels.</li>"
                "<li>Disposez les 15 rouges pour former un \"T\" :"
                "<ul style='margin:4px 0; padding-left:20px'>"
                "<li>5 rouges regulierement espacees d'un cote de l'axe de croisement, jusqu'a la "
                "bande</li>"
                "<li>5 rouges regulierement espacees de l'autre cote de cet axe, jusqu'a l'autre "
                "bande (ces 10 rouges forment la barre traversant toute la largeur de la "
                "table)</li>"
                "<li>5 rouges reparties uniformement entre cet axe et la noire (la deuxieme "
                "barre du \"T\")</li>"
                "</ul></li>"
                "<li>Choisissez la rouge de depart.</li>"
                "</ol>"

                "<p><b>DEROULEMENT -- Version 1 (plus accessible)</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la blanche entre les rouges et la bleue.</li>"
                "<li>Empochez une rouge puis une couleur, comme en jeu normal.</li>"
                "<li>Une fois toutes les rouges empochees, terminez avec les couleurs dans "
                "l'ordre habituel.</li>"
                "<li>En cas d'echec, remettez tout en place et recommencez depuis le debut.</li>"
                "<li>Notez votre score a chaque seance pour suivre vos progres.</li>"
                "</ol>"

                "<p><b>DEROULEMENT -- Version 2 (plus exigeante)</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la blanche a l'INTERIEUR du \"T\".</li>"
                "<li>Empochez rouge puis couleur en restant a l'INTERIEUR du \"T\" a chaque coup.</li>"
                "<li>Le reste est identique a la version 1.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Evitez que la blanche touche les bandes, pour apprendre a ne pas la "
                "deplacer trop loin.</li>"
                "<li>Une fois a l'aise, essayez d'empocher les rouges dans un ordre precis pour "
                "vous mettre a l'epreuve.</li>"
                "<li>Comme pour le Line-Up, certains preferent ne pas compter leur score et "
                "juste suivre le rythme -- les deux approches sont valables.</li>"
                "</ul>"
            },
            {
                "Controle de la bille blanche",
                "<p>Petit exercice pour observer la reaction de la blanche apres l'impact avec "
                "la bille cible : sa trajectoire depend directement du point ou vous placez le "
                "procede de la queue sur son axe vertical.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la bleue sur son point.</li>"
                "<li>Placez la blanche a environ 25 cm de la bleue, en ligne droite vers une "
                "poche du milieu.</li>"
                "<li>Reperez 5 points de frappe sur l'axe vertical de la blanche : Haut, "
                "1 cm au-dessus du centre, Centre, 1 cm en dessous du centre, Bas (effet "
                "retro).</li>"
                "</ol>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Jouez un coup normal pour empocher la bleue dans la poche du milieu.</li>"
                "<li>Essayez 5 coups a chaque position de frappe et notez la reaction de la "
                "blanche apres l'impact.</li>"
                "<li>Gardez le bras et la prise souples, en poussant bien la blanche jusqu'au "
                "bout du mouvement (sans a-coups).</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Une frappe haute (effet suivi) fait suivre la blanche derriere la bleue, "
                "jusque dans la meme poche.</li>"
                "<li>Une frappe basse (effet retro) fait reculer la blanche vers la poche "
                "opposee a la bleue.</li>"
                "<li>Une fois a l'aise, ajoutez d'autres points de frappe sur l'axe vertical "
                "pour affiner votre controle.</li>"
                "</ul>"
            },
            {
                "Potting Long Blues",
                "<p>Empocher des bleues a moyenne/longue distance developpe le toucher necessaire "
                "aux coups d'ouverture : bien execute, ce type de coup vous place directement en "
                "position sur la rose.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la bleue sur son point.</li>"
                "<li>Placez la blanche a environ 60 cm de la bleue (puis 90 cm, 120 cm en "
                "progressant), en ligne droite vers une poche de coin.</li>"
                "</ol>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Empochez 5 a 10 bleues droites dans chaque poche de coin, a partir de "
                "60 cm.</li>"
                "<li>Repetez l'exercice a des distances plus importantes le long de la meme "
                "ligne.</li>"
                "<li>Notez votre score pour evaluer vos progres au fil des seances.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Bon exercice pour savoir si vous empochez droit, quelle partie de la poche "
                "viser pour un succes optimal, et travailler un rythme de queue fluide.</li>"
                "</ul>"
            },
            {
                "Se mettre en place sur les spots",
                "<p>Faire rouler la blanche sur les points centraux est un excellent indicateur "
                "pour savoir si vous jouez droit -- l'un des fondamentaux avant d'aborder des "
                "exercices plus complexes.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la blanche sur le point marron.</li>"
                "<li>Placez la noire dans l'alignement noir-rose-bleu-marron, mais contre la "
                "bande, juste derriere le point noir.</li>"
                "</ol>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Centrez bien le procede de la queue sur la blanche.</li>"
                "<li>Visez le centre de la table, par-dessus les points, droit sur la noire.</li>"
                "<li>Frappez avec assez de vitesse pour que la blanche remonte jusqu'a la bande "
                "de depart.</li>"
                "<li>Restez immobile apres la frappe : si la blanche revient exactement a son "
                "point de depart, c'est que vous avez frappe droit et centre.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Ne frappez ni trop fort ni trop doucement, sous peine de derive naturelle "
                "de la blanche sur le tapis.</li>"
                "<li>Si la blanche ne revient pas exactement a son point de depart, cela indique "
                "une frappe decentree ou un effet lateral involontaire.</li>"
                "</ul>"
            },
            {
                "Empocher la bille noire",
                "<p>Exercice mental qui consiste a empocher des noires sous differents angles, "
                "dans un espace restreint -- utile pour apprendre a simplifier son jeu de "
                "position coup apres coup.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la noire sur son point.</li>"
                "<li>Placez la blanche a mi-chemin entre la bande laterale et la noire, en "
                "position 1/4, 1/2, 3/4 ou pleine bille (par rapport a la noire).</li>"
                "</ol>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Commencez simple : une noire droite dans la poche de coin.</li>"
                "<li>Empochez-en le plus possible a la suite, sans effet lateral au debut (juste "
                "haut/bas).</li>"
                "<li>Notez votre score total et le coup ou la serie se casse.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Si vous perdez la position, vous devrez utiliser la bande laterale, voire "
                "faire le tour de la table pour vous replacer.</li>"
                "<li>Simple en apparence, redoutable pour la precision -- vous allez l'adorer.</li>"
                "</ul>"
            },
            {
                "Potting direct des rouges",
                "<p>Enchainer des rouges en ligne droite est excellent pour travailler la "
                "position et verifier que la queue part bien droite sur la ligne de visee -- si "
                "vous ratez toujours du meme cote, cela revele un defaut de mise en place ou de "
                "visee.</p>"

                "<p><b>MISE EN PLACE -- Niveau 1 (courte distance)</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez une rouge au milieu de la table, entre les points bleu et marron.</li>"
                "<li>Placez 7 rouges de chaque cote de celle-ci, en ligne, a egale distance les "
                "unes des autres.</li>"
                "</ol>"

                "<p><b>MISE EN PLACE -- Niveau 2 (distance moyenne)</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez une rouge sur le point bleu, 7 rouges de chaque cote, alignees avec "
                "les poches du milieu.</li>"
                "</ol>"

                "<p><b>MISE EN PLACE -- Niveau 3 (longue distance)</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez une rouge entre les points rose et bleu, 7 rouges de chaque cote.</li>"
                "</ol>"

                "<p><b>DEROULEMENT (commun aux 3 niveaux)</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Deplacez la blanche le long de la ligne de depart en empochant chaque rouge "
                "dans une poche de coin.</li>"
                "<li>Essayez avec une blanche fixe (stun), puis avec un effet retro, un effet "
                "suivi (top) ou une frappe au centre -- commencez par le centre.</li>"
                "<li>Rouge empochee directement : 2 points. Rouge tombee dans une poche du "
                "milieu apres avoir touche la bande : 1 point. Rouge qui touche d'abord la bande "
                "laterale : 0 point.</li>"
                "<li>Essayez d'empocher les 15 rouges et notez votre score.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Bon exercice pour savoir si vous empochez droit, quelle partie de la poche "
                "viser, et garder un rythme de queue fluide.</li>"
                "<li>Progressez du niveau 1 (court) vers le niveau 3 (long) au fil de vos "
                "seances.</li>"
                "</ul>"
            },
            {
                "Potting Long Reds",
                "<p>Empocher des rouges longues developpe le toucher necessaire au coup "
                "d'ouverture qui, bien execute, vous place directement en position sur la "
                "rose.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la rose sur son point.</li>"
                "<li>Placez une rouge alignee avec la rose et une poche de coin, a environ "
                "30 cm de la rose.</li>"
                "<li>Placez la blanche sur cette meme ligne, a mi-chemin entre la ligne bleue "
                "et la bande de fond.</li>"
                "</ol>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Frappez legerement en dessous du centre de la blanche.</li>"
                "<li>Empochez la rouge dans la poche de fond, avec un leger effet retro.</li>"
                "<li>Arretez la blanche a l'endroit ou se trouvait la rouge, pour vous retrouver "
                "en position sur la rose.</li>"
                "<li>Essayez des deux cotes de la table, 5 a 10 coups dans chaque poche.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Si l'exercice est difficile, commencez avec la blanche a 30 cm de la rouge, "
                "puis reculez-la de 30 cm a chaque progres.</li>"
                "<li>Restez bien baisse sur ce coup jusqu'a ce que la rouge tombe -- ne relevez "
                "pas la tete trop tot.</li>"
                "<li>Notez vos scores pour suivre vos progres.</li>"
                "</ul>"
            },
            {
                "Sortir des situations de snooker faciles",
                "<p>Sortir d'un snooker facile devrait l'etre... mais ce n'est pas toujours le "
                "cas. Cet exercice structure la reflexion a avoir avant de jouer un coup de "
                "sortie, via la technique du \"miroir\".</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Choisissez 3 billes (par exemple bleue, rose et blanche).</li>"
                "<li>Alignez la bille objet, la bille qui bloque et la blanche, a egale distance "
                "de la bande, en ligne droite.</li>"
                "<li>Placez la noire sur son point : elle sert de penalite si vous ratez la "
                "bille objet.</li>"
                "<li>Assurez-vous que la blanche reste facile a jouer.</li>"
                "</ol>"

                "<p><b>DEROULEMENT -- technique du miroir</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Tenez-vous debout et observez de face la position des 3 billes.</li>"
                "<li>Reperez la distance entre la bille objet et la bande la plus proche.</li>"
                "<li>Imaginez l'image miroir de la bille objet, comme si elle etait de l'autre "
                "cote de la bande.</li>"
                "<li>Toujours debout, imaginez une ligne droite entre la blanche et cette image "
                "miroir.</li>"
                "<li>Le point ou cette ligne coupe la bande est l'endroit a viser pour toucher "
                "la bille objet.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>La vitesse de frappe determine la vitesse de la bille objet apres contact, "
                "et la position finale de la blanche en cas d'echec.</li>"
                "<li>Une fois a l'aise, essayez de \"caresser\" la bille objet pour creer de la "
                "distance entre les deux billes apres l'impact.</li>"
                "<li>Variez : espacez davantage les billes, changez leur distance a la bande, "
                "essayez une sortie plein fouet, ou par une autre bande.</li>"
                "<li>Testez les effets retro/stun/suivi pour observer comment ils changent la "
                "trajectoire de sortie.</li>"
                "</ul>"
            },
            {
                "Coups de securite au snooker sur les rouges",
                "<p>Exercice de toucher pour les petits coups de securite delicats : imaginez "
                "que l'adversaire a rate une rouge longue et l'a laissee juste sous le paquet -- "
                "l'ideal est de la laisser en zone de securite, avec un snooker difficile a la "
                "cle.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Un exercice a la fois, une seule rouge.</li>"
                "<li>Utilisez environ 6 positions le long de la bande (une marque a la craie "
                "peut aider).</li>"
                "<li>La 1ere rouge doit etre alignee avec le point noir.</li>"
                "<li>La blanche se place toujours juste derriere la noire.</li>"
                "</ol>"

                "<p><b>DEROULEMENT -- 3 objectifs a chaque coup</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Laisser la blanche derriere une bille de retenue.</li>"
                "<li>Laisser la blanche contre la bande de depart, ou aussi pres que possible.</li>"
                "<li>Ramener la rouge vers la rose et la noire, pour creer un snooker sur la "
                "blanche.</li>"
                "<li>Essayez d'abord le coup le plus facile pour rapprocher les 2 billes de la "
                "bande de depart, puis testez des effets lateraux/retro/vises pour arriver au "
                "meme resultat par un autre chemin.</li>"
                "<li>Notez 1 point par objectif atteint (3 points max par coup) pour suivre vos "
                "progres.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Il n'y a pas toujours un chemin evident vers la securite -- parfois un "
                "petit detour est necessaire pour proteger a la fois la rouge et la blanche.</li>"
                "</ul>"
            },
            {
                "Empocher plusieurs bleues",
                "<p>Exercice de toucher pour la situation ou rose et noire sont a egalite au "
                "score et qu'il reste des rouges autour de la bleue : viser le bon cote de la "
                "bleue devient crucial pour enchainer sur la rouge suivante -- un exercice a "
                "deux coups d'avance, plus difficile qu'il n'y parait.</p>"

                "<p><b>MISE EN PLACE</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez toutes les couleurs a leurs points.</li>"
                "<li>Placez 3 rouges de chaque cote du point bleu, alignees avec les points "
                "marron et rose.</li>"
                "</ol>"

                "<p><b>DEROULEMENT</b></p>"
                "<ol style='margin:4px 0 12px; padding-left:20px; line-height:1.7'>"
                "<li>Placez la blanche pour empocher une rouge tout en gardant un angle sur la "
                "bleue, puis enchainez sur la rouge suivante.</li>"
                "<li>Exercice de precision : evitez de trop deplacer la blanche.</li>"
                "<li>Si vous perdez la position, visez une autre partie de la poche pour recreer "
                "un angle (en adaptant la vitesse du coup).</li>"
                "<li>Une fois les 6 rouges eliminees, ajoutez-en une a chaque extremite pour "
                "augmenter la difficulte au fil des seances.</li>"
                "</ol>"

                "<p><b>CONSEILS</b></p>"
                "<ul style='margin:4px 0; padding-left:20px; line-height:1.7'>"
                "<li>Notez vos scores pour suivre vos progres et leur vitesse d'amelioration.</li>"
                "</ul>"
            },
        };
        return data;
    }
}

ExerciseDialog::ExerciseDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Exercice");
    applyDarkTitleBar(this);
    resize(820, 560);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(20);

    QVBoxLayout* leftCol = new QVBoxLayout();
    QLabel* leftTitle = new QLabel("EXERCICES", this);
    leftTitle->setStyleSheet("color: " + kWhite + "; font-size: 14px; font-weight: bold; letter-spacing: 1px;");
    leftCol->addWidget(leftTitle);

    m_exerciseList = new QListWidget(this);
    m_exerciseList->setStyleSheet(
        "QListWidget { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px; }"
        "QListWidget::item { padding: 10px 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: " + kGreen + "; }"
    );
    for (const Exercise& exercise : exercises())
    {
        m_exerciseList->addItem(exercise.title);
    }
    leftCol->addWidget(m_exerciseList, 1);

    QWidget* leftWidget = new QWidget(this);
    leftWidget->setLayout(leftCol);
    leftWidget->setFixedWidth(220);
    outer->addWidget(leftWidget);

    QVBoxLayout* rightCol = new QVBoxLayout();

    m_diagramLabel = new QLabel(this);
    m_diagramLabel->setAlignment(Qt::AlignCenter);
    m_diagramLabel->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 6px; padding: 8px;"
    );
    rightCol->addWidget(m_diagramLabel);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 6px; }"
    );
    m_contentLabel = new QLabel(this);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_contentLabel->setStyleSheet("color: " + kWhite + "; padding: 16px; font-size: 13px;");
    m_contentLabel->setTextFormat(Qt::RichText);
    scrollArea->setWidget(m_contentLabel);
    rightCol->addWidget(scrollArea, 1);

    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px; }"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    rightCol->addWidget(closeButton, 0, Qt::AlignRight);

    outer->addLayout(rightCol, 1);

    connect(m_exerciseList, &QListWidget::currentRowChanged, this, &ExerciseDialog::showExercise);
    m_exerciseList->setCurrentRow(0);
}

void ExerciseDialog::showExercise(int index)
{
    if (index < 0 || index >= exercises().size())
    {
        return;
    }
    m_contentLabel->setText(exercises()[index].html);
    // Chaque exercice a son propre diagramme de mise en place -- ajouter
    // un "case" ici pour tout nouvel exercice.
    switch (index)
    {
    case 1:
        m_diagramLabel->setPixmap(drawTShapeDiagram(520, 220));
        break;
    case 2:
        m_diagramLabel->setPixmap(drawCueBallControlDiagram(520, 220));
        break;
    case 3:
        m_diagramLabel->setPixmap(drawLongBluesDiagram(520, 220));
        break;
    case 4:
        m_diagramLabel->setPixmap(drawSpotsDiagram(520, 220));
        break;
    case 5:
        m_diagramLabel->setPixmap(drawBlackPottingDiagram(520, 220));
        break;
    case 6:
        m_diagramLabel->setPixmap(drawRedsPottingDiagram(520, 220));
        break;
    case 7:
        m_diagramLabel->setPixmap(drawLongRedsDiagram(520, 220));
        break;
    case 8:
        m_diagramLabel->setPixmap(drawSnookerEscapeDiagram(520, 220));
        break;
    case 9:
        m_diagramLabel->setPixmap(drawRedSafetyDiagram(520, 220));
        break;
    case 10:
        m_diagramLabel->setPixmap(drawMultipleBluesDiagram(520, 220));
        break;
    default:
        m_diagramLabel->setPixmap(drawLineUpDiagram(520, 220));
        break;
    }
}
