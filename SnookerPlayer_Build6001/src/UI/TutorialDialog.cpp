#include "TutorialDialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";

    struct Topic
    {
        QString title;
        QString html;
    };

    // Contenu statique : decrit le comportement REEL de l'appli tel que
    // construit (voir les memoires de session correspondantes), pas une
    // liste de fonctionnalites esperees -- a mettre a jour si le
    // comportement decrit ici change.
    const QVector<Topic>& topics()
    {
        static const QVector<Topic> data = {
            {
                "Demarrer un match",
                "Depuis l'ecran d'accueil, clique sur la tuile <b>NOUVEAU MATCH</b>.<br><br>"
                "Choisis le nom des deux joueurs (dans la liste des joueurs deja enregistres, "
                "ou en tapant un nouveau nom) et la longueur du match : 1 frame (partie rapide) "
                "ou meilleur des 3/5/7/9 frames.<br><br>"
                "L'appli bascule alors sur l'ecran de match, avec le score, le suivi de break et "
                "les boutons d'action (faute, bille sortie, free ball...)."
            },
            {
                "Connecter un telephone",
                "Clique sur la tuile <b>SMARTPHONE</b> pour afficher un QR code a scanner.<br><br>"
                "Le telephone doit etre connecte au MEME reseau Wi-Fi que ce PC. Une fois scanne, "
                "il peut suivre le score en direct ET agir sur le match (billes, fautes...) comme "
                "la telecommande de bureau. <b>2 telephones maximum</b> peuvent etre connectes en "
                "meme temps a une table.<br><br>"
                "Si aucun match n'est en cours, scanner ce lien propose directement de saisir les "
                "noms des joueurs et de demarrer un match depuis le telephone, sans toucher au PC "
                "-- pratique si le lien est imprime et colle sur la table."
            },
            {
                "Gerer les joueurs",
                "La tuile <b>JOUEURS</b> ouvre la liste des joueurs connus : photo (cliquer sur "
                "l'avatar pour la changer), statistiques (parties jouees, victoires, frames) et "
                "historique des matchs joues -- calcules automatiquement a partir des matchs "
                "sauvegardes, jamais a saisir a la main.<br><br>"
                "\"Ajouter\"/\"Supprimer\" gerent la liste rapide (utilisee aussi dans la boite de "
                "dialogue de demarrage d'un match) ; \"Effacer tous les joueurs\" efface toute la "
                "liste et les photos (sans toucher a l'historique des matchs deja joues)."
            },
            {
                "Organiser un tournoi",
                "La tuile <b>TOURNOI</b> permet de creer un tournoi avec les joueurs de ton choix, "
                "en <b>elimination directe</b> (bracket, avec exemption automatique si le nombre de "
                "joueurs n'est pas une puissance de 2) ou en <b>round robin</b> (tout le monde "
                "affronte tout le monde, classement par victoires puis difference de frames).<br><br>"
                "Une fois un match termine, l'appli propose directement le suivant.<br><br>"
                "Si plusieurs matchs se jouent EN MEME TEMPS sur d'autres tables, choisis le match "
                "concerne dans la liste puis \"Saisir un resultat...\" pour l'enregistrer sans rien "
                "lancer sur cette table."
            },
            {
                "S'entrainer (CueSense)",
                "La tuile <b>ENTRAINEMENT</b> lance CueSense, un outil separe d'analyse du geste au "
                "snooker (via un capteur de mouvement porte sur la queue de billard).<br><br>"
                "Il s'ouvre dans le navigateur web par defaut du PC."
            },
            {
                "Regler l'application",
                "Le bouton <b>PARAMETRES</b> (en haut a droite) regroupe : les annonces vocales "
                "(activer/desactiver, choisir une voix feminine ou masculine), le statut des "
                "cameras avec acces direct a leur calibration, et les informations sur la version "
                "du logiciel."
            },
            {
                "Consulter le reglement",
                "Le bouton <b>AIDE</b> (en haut a droite) ouvre une recherche dans le reglement "
                "officiel du snooker (francais et anglais) -- utile pour citer la regle exacte en "
                "cas de litige pendant un match."
            },
        };
        return data;
    }
}

TutorialDialog::TutorialDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Tutoriels");
    resize(760, 520);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(20);

    QVBoxLayout* leftCol = new QVBoxLayout();
    QLabel* leftTitle = new QLabel("TUTORIELS", this);
    leftTitle->setStyleSheet("color: " + kWhite + "; font-size: 14px; font-weight: bold; letter-spacing: 1px;");
    leftCol->addWidget(leftTitle);

    m_topicList = new QListWidget(this);
    m_topicList->setStyleSheet(
        "QListWidget { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px; }"
        "QListWidget::item { padding: 10px 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: " + kGreen + "; }"
    );
    for (const Topic& topic : topics())
    {
        m_topicList->addItem(topic.title);
    }
    leftCol->addWidget(m_topicList, 1);

    QWidget* leftWidget = new QWidget(this);
    leftWidget->setLayout(leftCol);
    leftWidget->setFixedWidth(220);
    outer->addWidget(leftWidget);

    QVBoxLayout* rightCol = new QVBoxLayout();

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

    connect(m_topicList, &QListWidget::currentRowChanged, this, &TutorialDialog::showTopic);
    m_topicList->setCurrentRow(0);
}

void TutorialDialog::showTopic(int index)
{
    if (index < 0 || index >= topics().size())
    {
        return;
    }
    m_contentLabel->setText(topics()[index].html);
}
