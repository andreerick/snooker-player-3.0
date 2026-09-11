#include "HomeScreen.h"

#include <QPainter>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QMessageBox>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QDateTime>
#include <QLocale>
#include <QFile>
#include <QCoreApplication>
#include <opencv2/videoio.hpp>

#ifndef HOME_SCREEN_IMAGE_PATH
#error "HOME_SCREEN_IMAGE_PATH doit etre defini par CMakeLists.txt (voir target_compile_definitions)"
#endif

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";

    // Meme boite de dialogue stylee que MainWindow::showStyledMessage() :
    // QMessageBox natif peut rendre le texte illisible selon le theme
    // clair/sombre de Windows, donc on force explicitement le style.
    void showStyledMessage(QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text)
    {
        QMessageBox box(icon, title, text, QMessageBox::Ok, parent);
        box.setStyleSheet(
            "QMessageBox { background-color: " + kBg + "; }"
            "QLabel { color: " + kWhite + "; background: transparent; }"
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  padding: 6px 16px;"
            "}"
            "QPushButton:hover { border-color: " + kGray + "; }"
        );
        box.exec();
    }

    // Dimensions natives de l'image de fond (assets/accueil_v3.png) :
    // toutes les zones cliquables ci-dessous sont exprimees dans cet
    // espace de coordonnees, mesure pixel par pixel sur l'image source.
    // Recalculer ces mesures si l'image source est un jour regeneree
    // (meme un leger changement de resolution decale toutes les zones).
    constexpr int kImageWidth = 1546;
    constexpr int kImageHeight = 1017;

    // Zone du texte "21:30 / 21 juillet 2024" (heure/date figees, decor
    // du mockup) en bas a droite -- masquee en repeignant du noir
    // par-dessus (voir paintEvent()) puis recouverte par m_clockLabel
    // affichant l'heure reelle. L'icone horloge du decor, elle, reste
    // celle de l'image (a gauche de cette zone, non masquee).
    const QRect kClockTextRect(1375, 908, 133, 90);

    // Zone de la ligne de statut ("3 cameras detectees / Mini-PC
    // connecte / Base de donnees OK", decor du mockup, fausse en l'etat
    // reel du projet) sous le titre "SYSTEME PRET" (lui, laisse tel
    // quel -- juste un intitule de section, pas une affirmation de fait).
    const QRect kStatusTextRect(480, 864, 550, 32);

    // Sonde brievement chaque camera (meme methode que SettingsDialog::
    // isCameraAvailable()) pour compter combien sont reellement
    // detectees -- fait une seule fois a la construction (sondage lent),
    // voir m_cameraCount.
    int countAvailableCameras()
    {
        int count = 0;
        for (int index = 0; index < 3; ++index)
        {
            cv::VideoCapture capture;
            if (capture.open(index))
            {
                ++count;
                capture.release();
            }
        }
        return count;
    }

    // Verifie que l'ecriture locale fonctionne reellement (au lieu
    // d'afficher "Base de donnees OK" sans jamais rien verifier) : cree
    // puis supprime un petit fichier temoin a cote de l'executable, la ou
    // joueurs.ini/matchs.json/etc. sont deja ecrits normalement.
    bool canWriteLocalStorage()
    {
        QString path = QCoreApplication::applicationDirPath() + "/.write_test";
        QFile file(path);
        bool ok = file.open(QIODevice::WriteOnly);
        if (ok)
        {
            file.close();
            QFile::remove(path);
        }
        return ok;
    }
}

HomeScreen::HomeScreen(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: " + kBg + ";");
    setMinimumSize(400, 267); // conserve le ratio 1536x1024, evite un widget degenere

    m_background = QPixmap(HOME_SCREEN_IMAGE_PATH);

    // --- Bandeau du haut ---
    QPushButton* aideButton = createHotspot(QRect(1165, 10, 110, 85), true, QString());
    QPushButton* settingsButton = createHotspot(QRect(1280, 10, 120, 85), true, QString());
    QPushButton* quitButton = createHotspot(QRect(1408, 10, 107, 85), true, QString());

    // --- Rangee de tuiles ---
    QPushButton* smartphoneTile = createHotspot(QRect(88, 598, 240, 205), true, QString());
    QPushButton* playersTile = createHotspot(QRect(348, 598, 256, 205), true, QString());
    QPushButton* newMatchTile = createHotspot(QRect(628, 598, 248, 205), true, QString());
    QPushButton* tournamentTile = createHotspot(QRect(896, 598, 252, 205), true, QString());
    QPushButton* trainingTile = createHotspot(QRect(1172, 598, 268, 205), true, QString());

    // --- Barre du bas ---
    QPushButton* tutorialsLink = createHotspot(QRect(465, 920, 185, 60), true, QString());
    QPushButton* rulesLink = createHotspot(QRect(665, 920, 160, 60), true, QString());
    QPushButton* aboutLink = createHotspot(QRect(840, 920, 205, 60), true, QString());

    connect(aideButton, &QPushButton::clicked, this, &HomeScreen::rulesRequested);
    connect(settingsButton, &QPushButton::clicked, this, &HomeScreen::settingsRequested);
    connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(smartphoneTile, &QPushButton::clicked, this, &HomeScreen::smartphoneRequested);
    connect(playersTile, &QPushButton::clicked, this, &HomeScreen::playersRequested);
    connect(tournamentTile, &QPushButton::clicked, this, &HomeScreen::tournamentRequested);
    connect(newMatchTile, &QPushButton::clicked, this, &HomeScreen::newMatchRequested);
    connect(trainingTile, &QPushButton::clicked, this, &HomeScreen::trainingRequested);
    connect(tutorialsLink, &QPushButton::clicked, this, &HomeScreen::tutorialsRequested);
    connect(rulesLink, &QPushButton::clicked, this, &HomeScreen::rulesRequested);
    connect(aboutLink, &QPushButton::clicked, this, &HomeScreen::showAboutDialog);

    // --- Horloge en direct (remplace l'heure figee du decor) ---
    m_clockLabel = new QLabel(this);
    m_clockLabel->setAlignment(Qt::AlignCenter);
    m_clockLabel->setStyleSheet("color: " + kWhite + "; background: transparent;");
    m_hotspots.append({ kClockTextRect, m_clockLabel });
    updateClock();
    QTimer* clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &HomeScreen::updateClock);
    clockTimer->start(1000);

    // --- Bandeau de statut reel (remplace "3 cameras detectees / Mini-PC
    // connecte / Base de donnees OK" fige et faux, voir kStatusTextRect) ---
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("background: transparent;");
    m_hotspots.append({ kStatusTextRect, m_statusLabel });
    m_cameraCount = countAvailableCameras();
    updateSystemStatus();

    relayoutHotspots();
}

void HomeScreen::setWifiStatus(bool active)
{
    m_wifiActive = active;
    updateSystemStatus();
}

void HomeScreen::updateSystemStatus()
{
    if (!m_statusLabel)
    {
        return;
    }

    auto dot = [](bool ok) { return QString("<span style='color:") + (ok ? kGreen : "#e74c3c") + ";'>&#9679;</span> "; };

    QString cameraText = m_cameraCount > 0
        ? QString::number(m_cameraCount) + (m_cameraCount > 1 ? " cameras detectees" : " camera detectee")
        : "Aucune camera detectee";

    QString wifiText = m_wifiActive ? "Partage Wi-Fi actif" : "Partage Wi-Fi indisponible";

    bool storageOk = canWriteLocalStorage();
    QString storageText = storageOk ? "Sauvegarde locale OK" : "Sauvegarde locale indisponible";

    m_statusLabel->setText(
        "<span style='font-size:12px; color:" + kWhite + ";'>"
        + dot(m_cameraCount > 0) + cameraText
        + "&nbsp;&nbsp;&nbsp;&nbsp;" + dot(m_wifiActive) + wifiText
        + "&nbsp;&nbsp;&nbsp;&nbsp;" + dot(storageOk) + storageText
        + "</span>"
    );
}

QPushButton* HomeScreen::createHotspot(const QRect& nativeRect, bool enabled, const QString& tooltip)
{
    QPushButton* button = new QPushButton(this);
    button->setFlat(true);
    button->setEnabled(enabled);
    button->setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (!tooltip.isEmpty())
    {
        button->setToolTip(tooltip);
    }
    // Totalement invisible : l'image de fond porte deja tout le visuel
    // (bouton, icone, texte) -- ce QPushButton ne sert qu'a capter le clic.
    button->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
    );
    m_hotspots.append({ nativeRect, button });
    return button;
}

void HomeScreen::updateClock()
{
    if (!m_clockLabel)
    {
        return;
    }
    QDateTime now = QDateTime::currentDateTime();
    QString time = now.toString("HH:mm");
    QString date = QLocale::system().toString(now.date(), "d MMMM yyyy");
    m_clockLabel->setText(
        "<div style='font-size:22px; font-weight:bold;'>" + time + "</div>"
        "<div style='font-size:12px; color:" + kGray + ";'>" + date + "</div>"
    );
}

void HomeScreen::relayoutHotspots()
{
    if (width() <= 0 || height() <= 0)
    {
        return;
    }

    // Meme logique que Qt::KeepAspectRatio pour QPixmap::scaled() :
    // l'image (1536x1024) est mise a l'echelle pour tenir entierement
    // dans le widget, centree, avec des bandes noires (letterbox) sur
    // les bords si le ratio du widget differe de celui de l'image.
    qreal scale = qMin(
        static_cast<qreal>(width()) / kImageWidth,
        static_cast<qreal>(height()) / kImageHeight
    );
    int scaledWidth = static_cast<int>(kImageWidth * scale);
    int scaledHeight = static_cast<int>(kImageHeight * scale);
    int offsetX = (width() - scaledWidth) / 2;
    int offsetY = (height() - scaledHeight) / 2;

    for (const Hotspot& spot : m_hotspots)
    {
        spot.widget->setGeometry(
            offsetX + static_cast<int>(spot.nativeRect.x() * scale),
            offsetY + static_cast<int>(spot.nativeRect.y() * scale),
            static_cast<int>(spot.nativeRect.width() * scale),
            static_cast<int>(spot.nativeRect.height() * scale)
        );
    }
}

void HomeScreen::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayoutHotspots();
}

void HomeScreen::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(kBg));

    if (m_background.isNull())
    {
        return;
    }

    QPixmap scaled = m_background.scaled(
        size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
    );
    int offsetX = (width() - scaled.width()) / 2;
    int offsetY = (height() - scaled.height()) / 2;
    painter.drawPixmap(offsetX, offsetY, scaled);

    // Masque les zones de texte figees du decor (heure/date, ligne de
    // statut cameras/Wi-Fi/stockage) : leur fond est deja quasi noir
    // dans l'image source, un simple aplat suffit a les rendre
    // invisibles avant que les QLabel correspondants (positionnes
    // par-dessus, voir relayoutHotspots()) n'affichent les vraies
    // valeurs au meme endroit.
    qreal scale = static_cast<qreal>(scaled.width()) / kImageWidth;
    auto patch = [&](const QRect& nativeRect)
    {
        painter.fillRect(
            QRect(
                offsetX + static_cast<int>(nativeRect.x() * scale),
                offsetY + static_cast<int>(nativeRect.y() * scale),
                static_cast<int>(nativeRect.width() * scale),
                static_cast<int>(nativeRect.height() * scale)
            ),
            QColor(kBg)
        );
    };
    patch(kClockTextRect);
    patch(kStatusTextRect);
}

void HomeScreen::showAboutDialog()
{
    showStyledMessage(
        this,
        QMessageBox::Information,
        "A propos",
        "Snooker Player\nVersion 1.0.0\n\n"
        "Assistant d'arbitrage et de suivi de match de snooker,\n"
        "avec detection camera, connexion smartphone et\n"
        "entrainement au geste (CueSense)."
    );
}
