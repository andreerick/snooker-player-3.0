#include "PlayersDialog.h"
#include "../Storage/MatchStorage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QSet>
#include <QIcon>
#include <QLineEdit>
#include <QRegularExpression>
#include <algorithm>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kOrange = "#f5a623";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";
    const QString kRed = "#e74c3c";

    // Meme fichier/cle que MainWindow::loadSavedPlayers()/savePlayerList() :
    // la liste de "joueurs connus" est partagee entre cette fenetre et la
    // boite de dialogue de saisie des noms au demarrage d'un match.
    QStringList loadSavedPlayers()
    {
        QSettings settings(QCoreApplication::applicationDirPath() + "/joueurs.ini", QSettings::IniFormat);
        return settings.value("joueurs/noms").toStringList();
    }

    void savePlayerList(const QStringList& names)
    {
        QSettings settings(QCoreApplication::applicationDirPath() + "/joueurs.ini", QSettings::IniFormat);
        settings.setValue("joueurs/noms", names);
        settings.sync();
    }

    // Chemin du photo separe (players.json, a cote de l'executable) :
    // juste une association nom -> chemin de fichier photo, tout le
    // reste (stats, historique) est recalcule depuis matchs.json a la
    // demande plutot que stocke, pour ne jamais pouvoir se desynchroniser.
    QString playersJsonPath()
    {
        return QCoreApplication::applicationDirPath() + "/players.json";
    }

    QJsonObject loadPlayerProfiles()
    {
        QFile file(playersJsonPath());
        if (!file.open(QIODevice::ReadOnly))
        {
            return QJsonObject();
        }
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        return doc.isObject() ? doc.object() : QJsonObject();
    }

    void savePlayerProfiles(const QJsonObject& profiles)
    {
        QFile file(playersJsonPath());
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file.write(QJsonDocument(profiles).toJson(QJsonDocument::Indented));
            file.close();
        }
    }

    QString photoPathFor(const QString& playerName)
    {
        QJsonObject profiles = loadPlayerProfiles();
        return profiles.value(playerName).toObject().value("photo").toString();
    }

    void setPhotoPathFor(const QString& playerName, const QString& photoPath)
    {
        QJsonObject profiles = loadPlayerProfiles();
        QJsonObject entry = profiles.value(playerName).toObject();
        entry["photo"] = photoPath;
        profiles[playerName] = entry;
        savePlayerProfiles(profiles);
    }

    // Union des noms "enregistres" (joueurs.ini) et des noms qui
    // apparaissent au moins une fois dans l'historique des matchs
    // (matchs.json) : un joueur ayant deja joue reste visible ici meme
    // s'il a ete retire de la liste rapide, puisque son historique, lui,
    // ne peut pas etre efface retroactivement.
    QStringList allKnownPlayerNames()
    {
        QSet<QString> names;
        for (const QString& name : loadSavedPlayers())
        {
            names.insert(name);
        }
        for (const QJsonValue& value : MatchStorage::loadHistory())
        {
            QJsonObject match = value.toObject();
            names.insert(match.value("joueur1").toString());
            names.insert(match.value("joueur2").toString());
        }
        names.remove(QString());
        QStringList sorted = names.values();
        sorted.sort(Qt::CaseInsensitive);
        return sorted;
    }

    struct PlayerStats
    {
        int played = 0;
        int won = 0;
        int framesWon = 0;
        int framesLost = 0;
    };

    struct HistoryEntry
    {
        QString date;
        QString opponent;
        QString scoreText;
        bool won = false;
    };

    // Parcourt matchs.json une seule fois pour extraire a la fois les
    // stats agregees et le detail des matchs joues par `playerName`
    // (les deux ont besoin de la meme lecture, autant ne le faire qu'une
    // fois). Triee la plus recente en premier.
    void computePlayerData(const QString& playerName, PlayerStats& stats, QVector<HistoryEntry>& history)
    {
        for (const QJsonValue& value : MatchStorage::loadHistory())
        {
            QJsonObject match = value.toObject();
            QString p1 = match.value("joueur1").toString();
            QString p2 = match.value("joueur2").toString();
            if (p1 != playerName && p2 != playerName)
            {
                continue;
            }
            int framesP1 = match.value("frames_joueur1").toInt();
            int framesP2 = match.value("frames_joueur2").toInt();
            bool isP1 = (p1 == playerName);
            int myFrames = isP1 ? framesP1 : framesP2;
            int otherFrames = isP1 ? framesP2 : framesP1;
            bool won = myFrames > otherFrames;

            stats.played++;
            if (won)
            {
                stats.won++;
            }
            stats.framesWon += myFrames;
            stats.framesLost += otherFrames;

            HistoryEntry entry;
            entry.date = match.value("date").toString();
            entry.opponent = isP1 ? p2 : p1;
            entry.scoreText = QString::number(myFrames) + " - " + QString::number(otherFrames);
            entry.won = won;
            history.append(entry);
        }
        std::reverse(history.begin(), history.end());
    }

    // Dessine une pastille ronde : la photo du joueur si fournie, sinon
    // un cercle uni avec son initiale -- meme rendu dans les deux cas
    // (taille, anti-crenelage) pour que l'un ou l'autre s'integre
    // proprement au meme endroit.
    QPixmap roundAvatar(const QPixmap& source, int diameter, const QString& fallbackInitial)
    {
        QPixmap result(diameter, diameter);
        result.fill(Qt::transparent);

        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath clip;
        clip.addEllipse(0, 0, diameter, diameter);
        painter.setClipPath(clip);

        if (source.isNull())
        {
            painter.fillRect(0, 0, diameter, diameter, QColor(kPanel));
            painter.setPen(QColor(kGray));
            QFont font = painter.font();
            font.setPointSize(diameter / 3);
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(QRect(0, 0, diameter, diameter), Qt::AlignCenter, fallbackInitial.toUpper());
        }
        else
        {
            QPixmap scaled = source.scaled(diameter, diameter, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int offsetX = (scaled.width() - diameter) / 2;
            int offsetY = (scaled.height() - diameter) / 2;
            painter.drawPixmap(-offsetX, -offsetY, scaled);
        }

        painter.setClipping(false);
        painter.setPen(QPen(QColor(kBorder), 2));
        painter.drawEllipse(1, 1, diameter - 2, diameter - 2);

        return result;
    }
}

PlayersDialog::PlayersDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Joueurs");
    resize(820, 560);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(20);

    // --- Colonne de gauche : liste des joueurs ---
    QVBoxLayout* leftCol = new QVBoxLayout();
    leftCol->setSpacing(10);

    QLabel* leftTitle = new QLabel("JOUEURS", this);
    leftTitle->setStyleSheet("color: " + kWhite + "; font-size: 14px; font-weight: bold; letter-spacing: 1px;");
    leftCol->addWidget(leftTitle);

    m_playerList = new QListWidget(this);
    m_playerList->setStyleSheet(
        "QListWidget { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px; }"
        "QListWidget::item { padding: 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: " + kGreen + "; }"
    );
    leftCol->addWidget(m_playerList, 1);

    QHBoxLayout* leftButtons = new QHBoxLayout();
    QString smallButtonStyle =
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 4px; padding: 6px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kGray + "; }";
    QPushButton* addButton = new QPushButton("Ajouter", this);
    addButton->setStyleSheet(smallButtonStyle);
    QPushButton* removeButton = new QPushButton("Supprimer", this);
    removeButton->setStyleSheet(smallButtonStyle);
    leftButtons->addWidget(addButton);
    leftButtons->addWidget(removeButton);
    leftCol->addLayout(leftButtons);

    // Distinct de "Supprimer" (qui ne retire que le joueur selectionne) :
    // efface TOUTE la liste + les photos d'un coup. Reste ici (pas dans
    // Parametres) puisque c'est l'ecran des joueurs, sur demande de
    // l'utilisateur.
    QPushButton* clearAllButton = new QPushButton("Effacer tous les joueurs", this);
    clearAllButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kRed + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 4px; padding: 6px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kRed + "; }"
    );
    connect(clearAllButton, &QPushButton::clicked, this, &PlayersDialog::clearAllPlayers);
    leftCol->addWidget(clearAllButton);

    QWidget* leftWidget = new QWidget(this);
    leftWidget->setLayout(leftCol);
    leftWidget->setFixedWidth(240);
    outer->addWidget(leftWidget);

    // --- Colonne de droite : profil du joueur selectionne ---
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);

    QHBoxLayout* headerRow = new QHBoxLayout();
    headerRow->setSpacing(16);

    m_photoButton = new QPushButton(this);
    m_photoButton->setFixedSize(96, 96);
    m_photoButton->setIconSize(QSize(96, 96));
    m_photoButton->setCursor(Qt::PointingHandCursor);
    m_photoButton->setToolTip("Cliquer pour changer la photo");
    m_photoButton->setFlat(true);
    m_photoButton->setStyleSheet("QPushButton { background: transparent; border: none; }");
    headerRow->addWidget(m_photoButton);

    QVBoxLayout* nameStatsCol = new QVBoxLayout();
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("color: " + kWhite + "; font-size: 22px; font-weight: bold;");
    nameStatsCol->addWidget(m_nameLabel);

    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("color: " + kGray + "; font-size: 12px;");
    m_statsLabel->setWordWrap(true);
    nameStatsCol->addWidget(m_statsLabel);
    nameStatsCol->addStretch();

    headerRow->addLayout(nameStatsCol, 1);
    rightCol->addLayout(headerRow);

    QLabel* historyTitle = new QLabel("HISTORIQUE DES MATCHS", this);
    historyTitle->setStyleSheet("color: " + kGray + "; font-size: 11px; letter-spacing: 1px; margin-top: 12px;");
    rightCol->addWidget(historyTitle);

    m_historyList = new QListWidget(this);
    m_historyList->setStyleSheet(
        "QListWidget { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid " + kBorder + "; }"
    );
    rightCol->addWidget(m_historyList, 1);

    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(smallButtonStyle);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    rightCol->addWidget(closeButton, 0, Qt::AlignRight);

    outer->addLayout(rightCol, 1);

    connect(m_playerList, &QListWidget::currentTextChanged, this, [this](const QString& name)
        {
            if (!name.isEmpty())
            {
                showPlayer(name);
            }
        });
    connect(addButton, &QPushButton::clicked, this, &PlayersDialog::addPlayer);
    connect(removeButton, &QPushButton::clicked, this, &PlayersDialog::removeSelectedPlayer);
    connect(m_photoButton, &QPushButton::clicked, this, &PlayersDialog::changePhoto);

    refreshPlayerList();
}

void PlayersDialog::refreshPlayerList()
{
    QString previouslySelected = m_currentPlayer;
    m_playerList->clear();
    m_playerList->addItems(allKnownPlayerNames());

    if (m_playerList->count() == 0)
    {
        m_nameLabel->setText("Aucun joueur enregistre");
        m_statsLabel->setText("Ajoutez un joueur pour commencer, ou jouez un match.");
        m_photoButton->setIcon(QIcon(roundAvatar(QPixmap(), 96, "?")));
        m_historyList->clear();
        m_currentPlayer.clear();
        return;
    }

    QList<QListWidgetItem*> matches = m_playerList->findItems(previouslySelected, Qt::MatchExactly);
    if (!matches.isEmpty())
    {
        m_playerList->setCurrentItem(matches.first());
    }
    else
    {
        m_playerList->setCurrentRow(0);
    }
}

void PlayersDialog::showPlayer(const QString& name)
{
    m_currentPlayer = name;
    m_nameLabel->setText(name);

    QString photoPath = photoPathFor(name);
    QPixmap photo;
    if (!photoPath.isEmpty())
    {
        photo.load(photoPath);
    }
    m_photoButton->setIcon(QIcon(roundAvatar(photo, 96, name.isEmpty() ? "?" : QString(name.at(0)))));

    PlayerStats stats;
    QVector<HistoryEntry> history;
    computePlayerData(name, stats, history);

    int winRate = stats.played > 0 ? (stats.won * 100 / stats.played) : 0;
    m_statsLabel->setText(
        QString("Parties jouees : %1    Victoires : %2 (%3%)    Frames : %4 - %5")
            .arg(stats.played).arg(stats.won).arg(winRate).arg(stats.framesWon).arg(stats.framesLost)
    );

    m_historyList->clear();
    if (history.isEmpty())
    {
        m_historyList->addItem("Aucun match joue pour l'instant.");
    }
    for (const HistoryEntry& entry : history)
    {
        QString dateText = entry.date;
        dateText.replace('T', ' ');
        int dotIndex = dateText.indexOf('.');
        if (dotIndex > 0)
        {
            dateText = dateText.left(dotIndex);
        }
        QString resultText = entry.won ? "Victoire" : "Defaite";
        QListWidgetItem* item = new QListWidgetItem(
            dateText + "  -  vs " + entry.opponent + "  -  " + entry.scoreText + "  -  " + resultText
        );
        item->setForeground(entry.won ? QColor(kGreen) : QColor(kRed));
        m_historyList->addItem(item);
    }
}

void PlayersDialog::addPlayer()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, "Ajouter un joueur", "Nom du joueur :", QLineEdit::Normal, QString(), &ok);
    name = name.trimmed();
    if (!ok || name.isEmpty())
    {
        return;
    }

    QStringList names = loadSavedPlayers();
    if (!names.contains(name))
    {
        names.append(name);
        savePlayerList(names);
    }
    refreshPlayerList();
    QList<QListWidgetItem*> matches = m_playerList->findItems(name, Qt::MatchExactly);
    if (!matches.isEmpty())
    {
        m_playerList->setCurrentItem(matches.first());
    }
}

void PlayersDialog::removeSelectedPlayer()
{
    if (m_currentPlayer.isEmpty())
    {
        return;
    }

    QStringList names = loadSavedPlayers();
    names.removeAll(m_currentPlayer);
    savePlayerList(names);

    QJsonObject profiles = loadPlayerProfiles();
    profiles.remove(m_currentPlayer);
    savePlayerProfiles(profiles);

    refreshPlayerList();
}

void PlayersDialog::clearAllPlayers()
{
    QMessageBox box(QMessageBox::Warning, "Effacer tous les joueurs",
        "Effacer definitivement la liste des joueurs enregistres et leurs photos ?\n\n"
        "L'historique des matchs deja joues n'est pas touche.",
        QMessageBox::Yes | QMessageBox::No, this);
    box.setStyleSheet(
        "QMessageBox { background-color: " + kBg + "; }"
        "QLabel { color: " + kWhite + "; background: transparent; }"
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
    );
    if (box.exec() != QMessageBox::Yes)
    {
        return;
    }

    savePlayerList(QStringList());
    savePlayerProfiles(QJsonObject());
    QDir photosDir(QCoreApplication::applicationDirPath() + "/players_photos");
    if (photosDir.exists())
    {
        photosDir.removeRecursively();
    }
    refreshPlayerList();
}

void PlayersDialog::changePhoto()
{
    if (m_currentPlayer.isEmpty())
    {
        return;
    }

    QString sourcePath = QFileDialog::getOpenFileName(
        this, "Choisir une photo pour " + m_currentPlayer, QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)"
    );
    if (sourcePath.isEmpty())
    {
        return;
    }

    // Copie la photo dans un dossier local (plutot que de garder le
    // chemin d'origine tel quel) : si le fichier source est deplace ou
    // supprime plus tard (cle USB, dossier telechargements vide...), la
    // photo de la fiche joueur reste disponible.
    QString photosDir = QCoreApplication::applicationDirPath() + "/players_photos";
    QDir().mkpath(photosDir);

    QString extension = QFileInfo(sourcePath).suffix();
    QString sanitizedName = m_currentPlayer;
    sanitizedName.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");
    QString destPath = photosDir + "/" + sanitizedName + "." + extension;

    QFile::remove(destPath);
    if (!QFile::copy(sourcePath, destPath))
    {
        QMessageBox box(QMessageBox::Warning, "Photo", "Impossible de copier cette image.", QMessageBox::Ok, this);
        box.setStyleSheet(
            "QMessageBox { background-color: " + kBg + "; }"
            "QLabel { color: " + kWhite + "; background: transparent; }"
            "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
        );
        box.exec();
        return;
    }

    setPhotoPathFor(m_currentPlayer, destPath);
    showPlayer(m_currentPlayer);
}
