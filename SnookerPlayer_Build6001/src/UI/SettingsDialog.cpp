#include "SettingsDialog.h"
#include "UiUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QFile>
#include <QScrollArea>
#include <QProcess>
#include <opencv2/videoio.hpp>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";
    const QString kRed = "#e74c3c";

    QString settingsFilePath()
    {
        return QCoreApplication::applicationDirPath() + "/settings.ini";
    }

    void showInfo(QWidget* parent, const QString& title, const QString& text)
    {
        QMessageBox box(QMessageBox::Information, title, text, QMessageBox::Ok, parent);
        applyDarkTitleBar(&box);
        box.setStyleSheet(
            "QMessageBox { background-color: " + kBg + "; }"
            "QLabel { color: " + kWhite + "; background: transparent; }"
            "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
        );
        box.exec();
    }

    // Cree une cellule visuellement distincte (cadre + titre) pour un
    // groupe de reglages (Son / Camera / Logiciel) -- demande par
    // l'utilisateur ("des cellules bien distinctes"), plutot que la
    // simple liste de titres gris + widgets a plat de la version
    // precedente. Renvoie la disposition ou ajouter le contenu de la
    // cellule ; la cellule elle-meme est deja ajoutee a `parentLayout`.
    QVBoxLayout* addSettingsCard(QVBoxLayout* parentLayout, QWidget* parent, const QString& title)
    {
        QFrame* card = new QFrame(parent);
        card->setStyleSheet(
            "QFrame { background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 10px; }"
        );
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 14, 16, 16);
        cardLayout->setSpacing(10);

        QLabel* titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet(
            "color: " + kGray + "; font-size: 11px; font-weight: bold; letter-spacing: 1px; background: transparent;"
        );
        cardLayout->addWidget(titleLabel);

        parentLayout->addWidget(card);
        return cardLayout;
    }

    // Ouvre brievement la camera `index` pour verifier sa presence, puis
    // la relache tout de suite (juste un test de detection, pas un
    // apercu) -- meme indices/backend par defaut que MainWindow (voir
    // toggleVisionTracking()), pour rester coherent avec ce que le suivi
    // camera reel essaiera d'ouvrir. Si le suivi camera tourne DEJA
    // (match en cours avec camera active), cette verification peut
    // renvoyer "non detectee" par erreur (le peripherique est deja
    // ouvert ailleurs) -- limitation connue, pas grave pour un simple
    // indicateur de statut consulte depuis les parametres.
    bool isCameraAvailable(int index)
    {
        cv::VideoCapture capture;
        bool opened = capture.open(index);
        if (opened)
        {
            capture.release();
        }
        return opened;
    }

    // Lance CalibrationTool.exe (executable separe, voir CMakeLists.txt)
    // sur la camera `index` en flux live -- meme dossier que
    // SnookerPlayer.exe puisque c'est une cible soeur du meme projet
    // CMake, generee au meme endroit.
    void launchCalibrationTool(QWidget* parent, int index)
    {
#ifdef Q_OS_WIN
        const QString toolName = "CalibrationTool.exe";
#else
        const QString toolName = "CalibrationTool";
#endif
        QString exePath = QCoreApplication::applicationDirPath() + "/" + toolName;
        if (!QFile::exists(exePath))
        {
            QMessageBox box(QMessageBox::Warning, "Utilitaire camera",
                toolName + " est introuvable a cote de l'executable principal.\n"
                "Recompilez le projet (cible CalibrationTool).", QMessageBox::Ok, parent);
            applyDarkTitleBar(&box);
            box.setStyleSheet(
                "QMessageBox { background-color: " + kBg + "; }"
                "QLabel { color: " + kWhite + "; background: transparent; }"
                "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
                "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
            );
            box.exec();
            return;
        }
        QStringList args{ QString::number(index), QString::number(index) };
        QProcess::startDetached(exePath, args);
    }
}

bool SettingsDialog::loadSpeechEnabled()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    return settings.value("speech/enabled", true).toBool();
}

QVoice::Gender SettingsDialog::loadSpeechGender()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    QString value = settings.value("speech/gender", "female").toString();
    return (value == "male") ? QVoice::Male : QVoice::Female;
}

QString SettingsDialog::loadRemoteVersion()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    QString value = settings.value("ui/remoteVersion", "2.0").toString();
    return (value == "2.0") ? "2.0" : "1.0";
}

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Parametres");
    applyDarkTitleBar(this);
    resize(480, 680);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    QWidget* content = new QWidget(scrollArea);
    scrollArea->setWidget(content);
    outer->addWidget(scrollArea, 1);

    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 16);
    layout->setSpacing(14);

    QLabel* title = new QLabel("PARAMETRES", content);
    title->setStyleSheet("color: " + kWhite + "; font-size: 16px; font-weight: bold; letter-spacing: 1px;");
    layout->addWidget(title);

    const QString comboStyle =
        "QComboBox { background-color: " + kBg + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 4px; padding: 6px; }";
    const QString smallButtonStyle =
        "QPushButton {"
        "  background-color: " + kBg + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 4px; padding: 6px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kGray + "; }";

    // --- Son ---
    QVBoxLayout* soundCard = addSettingsCard(layout, content, "SON");

    m_speechCheckBox = new QCheckBox("Annonces vocales (points, fautes, changement de joueur)", content);
    m_speechCheckBox->setChecked(loadSpeechEnabled());
    m_speechCheckBox->setStyleSheet("color: " + kWhite + "; font-size: 13px; background: transparent;");
    connect(m_speechCheckBox, &QCheckBox::toggled, this, [this](bool checked)
        {
            QSettings settings(settingsFilePath(), QSettings::IniFormat);
            settings.setValue("speech/enabled", checked);
            settings.sync();
            emit speechEnabledChanged(checked);
        });
    soundCard->addWidget(m_speechCheckBox);

    QWidget* genderRow = new QWidget(content);
    QHBoxLayout* genderRowLayout = new QHBoxLayout(genderRow);
    genderRowLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* genderLabel = new QLabel("Voix :", genderRow);
    genderLabel->setStyleSheet("color: " + kWhite + "; font-size: 13px; background: transparent;");
    m_genderCombo = new QComboBox(genderRow);
    m_genderCombo->setStyleSheet(comboStyle);
    m_genderCombo->addItem("Feminine", "female");
    m_genderCombo->addItem("Masculine", "male");
    m_genderCombo->setCurrentIndex(loadSpeechGender() == QVoice::Male ? 1 : 0);
    connect(m_genderCombo, &QComboBox::currentTextChanged, this, [this](const QString&)
        {
            QString value = m_genderCombo->currentData().toString();
            QSettings settings(settingsFilePath(), QSettings::IniFormat);
            settings.setValue("speech/gender", value);
            settings.sync();
            emit speechGenderChanged(value == "male" ? QVoice::Male : QVoice::Female);
        });
    genderRowLayout->addWidget(genderLabel);
    genderRowLayout->addWidget(m_genderCombo, 1);
    soundCard->addWidget(genderRow);

    QLabel* genderHint = new QLabel(
        "Selon le moteur vocal installe sur ce PC, la voix demandee peut ne pas etre disponible "
        "(la voix par defaut est alors gardee).",
        content
    );
    genderHint->setWordWrap(true);
    genderHint->setStyleSheet("color: " + kGray + "; font-size: 10px; background: transparent;");
    soundCard->addWidget(genderHint);

    // --- Telecommande ---
    QVBoxLayout* remoteCard = addSettingsCard(layout, content, "TELECOMMANDE");

    QWidget* remoteVersionRow = new QWidget(content);
    QHBoxLayout* remoteVersionRowLayout = new QHBoxLayout(remoteVersionRow);
    remoteVersionRowLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* remoteVersionLabel = new QLabel("Modele :", remoteVersionRow);
    remoteVersionLabel->setStyleSheet("color: " + kWhite + "; font-size: 13px; background: transparent;");
    m_remoteVersionCombo = new QComboBox(remoteVersionRow);
    m_remoteVersionCombo->setStyleSheet(comboStyle);
    m_remoteVersionCombo->addItem("1 - Telecommande test (scenarios)", "1.0");
    m_remoteVersionCombo->addItem("2 - Telecommande simplifiee", "2.0");
    m_remoteVersionCombo->setCurrentIndex(loadRemoteVersion() == "2.0" ? 1 : 0);
    connect(m_remoteVersionCombo, &QComboBox::currentTextChanged, this, [this](const QString&)
        {
            QString value = m_remoteVersionCombo->currentData().toString();
            QSettings settings(settingsFilePath(), QSettings::IniFormat);
            settings.setValue("ui/remoteVersion", value);
            settings.sync();
            emit remoteVersionChanged(value);
        });
    remoteVersionRowLayout->addWidget(remoteVersionLabel);
    remoteVersionRowLayout->addWidget(m_remoteVersionCombo, 1);
    remoteCard->addWidget(remoteVersionRow);

    QLabel* remoteVersionHint = new QLabel(
        "La 1 garde tous les boutons de test (scenarios, rejeu, historique...). La 2 ne garde que "
        "les couleurs, Faute, Fin de break, Esc et Game -- pour jouer sans se perdre dans les "
        "options. Dans les deux cas, la telecommande se masque automatiquement des qu'un "
        "telephone se connecte (voir tuile Smartphone).",
        content
    );
    remoteVersionHint->setWordWrap(true);
    remoteVersionHint->setStyleSheet("color: " + kGray + "; font-size: 10px; background: transparent;");
    remoteCard->addWidget(remoteVersionHint);

    // --- Camera ---
    QVBoxLayout* cameraCard = addSettingsCard(layout, content, "CAMERA");

    for (int index = 0; index < 3; ++index)
    {
        QWidget* row = new QWidget(content);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        bool detected = isCameraAvailable(index);
        QLabel* statusDot = new QLabel(row);
        statusDot->setFixedSize(10, 10);
        statusDot->setStyleSheet(
            "background-color: " + QString(detected ? kGreen : kRed) + "; border-radius: 5px;"
        );
        QLabel* statusText = new QLabel(
            "Camera " + QString::number(index) + " : " + (detected ? "detectee" : "non detectee"), row
        );
        statusText->setStyleSheet("color: " + kWhite + "; font-size: 12px; background: transparent;");

        QPushButton* calibrateButton = new QPushButton("Calibrer...", row);
        calibrateButton->setStyleSheet(smallButtonStyle);
        calibrateButton->setEnabled(detected);
        connect(calibrateButton, &QPushButton::clicked, this, [this, index]()
            {
                launchCalibrationTool(this, index);
            });

        rowLayout->addWidget(statusDot);
        rowLayout->addWidget(statusText, 1);
        rowLayout->addWidget(calibrateButton);
        cameraCard->addWidget(row);
    }

    QLabel* cameraHint = new QLabel(
        "Statut verifie a l'ouverture de cette fenetre. Si un match avec suivi camera est deja en cours, "
        "les cameras utilisees peuvent apparaitre \"non detectees\" ici (deja ouvertes ailleurs).",
        content
    );
    cameraHint->setWordWrap(true);
    cameraHint->setStyleSheet("color: " + kGray + "; font-size: 10px; background: transparent;");
    cameraCard->addWidget(cameraHint);

    // --- Logiciel ---
    QVBoxLayout* softwareCard = addSettingsCard(layout, content, "LOGICIEL");

    QLabel* versionLabel = new QLabel("Snooker Player -- Version 1.0.0", content);
    versionLabel->setStyleSheet("color: " + kWhite + "; font-size: 13px; background: transparent;");
    softwareCard->addWidget(versionLabel);

    QWidget* updateRow = new QWidget(content);
    QHBoxLayout* updateRowLayout = new QHBoxLayout(updateRow);
    updateRowLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton* updateButton = new QPushButton("Rechercher une mise a jour", updateRow);
    updateButton->setStyleSheet(smallButtonStyle);
    connect(updateButton, &QPushButton::clicked, this, [this]()
        {
            showInfo(this, "Mise a jour",
                "Pas encore de mise a jour automatique : l'application n'est pour l'instant "
                "distribuee que depuis son code source (pas d'installateur ni de serveur de "
                "mise a jour). Cette fonctionnalite arrivera avec le premier vrai packaging "
                "de l'application.");
        });
    updateRowLayout->addWidget(updateButton);
    updateRowLayout->addStretch();
    softwareCard->addWidget(updateRow);

    // Pas de section "Donnees locales" ici : chaque action d'effacement
    // (historique des matchs, liste des joueurs, tournoi en cours) vit
    // desormais directement dans l'ecran concerne (panneau de match,
    // PlayersDialog, TournamentDialog) plutot que centralisee ici, sur
    // demande de l'utilisateur -- plus logique de trouver "effacer les
    // joueurs" dans l'ecran des joueurs que dans un fourre-tout Parametres.
    layout->addStretch();

    // Hors de la zone defilante (ajoute a `outer`, pas `layout`) : reste
    // visible et cliquable en permanence, plutot que d'etre enterre en
    // bas d'une liste de reglages qui s'est allongee (camera, logiciel).
    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px; }"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    QHBoxLayout* closeRow = new QHBoxLayout();
    closeRow->setContentsMargins(24, 8, 24, 16);
    closeRow->addStretch();
    closeRow->addWidget(closeButton);
    outer->addLayout(closeRow);
}
