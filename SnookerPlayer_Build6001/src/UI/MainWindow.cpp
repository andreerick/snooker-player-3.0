#include "MainWindow.h"

#include "ShotHistory.h"
#include "../Storage/MatchStorage.h"
#include "ShareSessionDialog.h"
#include "PlayersDialog.h"
#include "TournamentDialog.h"
#include "TournamentManager.h"
#include "SettingsDialog.h"
#include "TutorialDialog.h"
#include "TrainingChoiceDialog.h"
#include "ExerciseDialog.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QFont>
#include <QTimer>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QSettings>
#include <QCoreApplication>
#include <QColor>
#include <QVector>
#include <QPair>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include <QListWidget>
#include <QJsonArray>
#include <QScreen>
#include <QGuiApplication>
#include <QShortcut>
#include <QJsonObject>
#include <QTableWidget>
#include <QHeaderView>
#include <QMap>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <QPixmap>
#include <algorithm>
#include <memory>
#include <map>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kOrange = "#f5a623";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";
    // Vert vif utilise pour les pastilles de statut (cameras, break, etc.),
    // distinct du vert plus sombre kGreen utilise pour le theme joueur 1.
    const QString kStatusGreen = "#22c55e";
    const QString kRecordingRed = "#ef4444";

    // Affiche une boite de message (info/avertissement) stylee coherente
    // avec le theme sombre de l'appli. QMessageBox natif (Windows) peut
    // rendre le texte illisible (blanc sur fond blanc) selon le theme
    // clair/sombre de l'utilisateur ; on force donc un style explicite
    // plutot que de compter sur l'apparence par defaut de l'OS.
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

    // Applique une ombre portee douce a un widget, pour donner du relief
    // aux billes et pastilles (rendu plus proche de la maquette).
    void applySoftShadow(QWidget* widget, int blurRadius = 12, int yOffset = 3)
    {
        auto* shadow = new QGraphicsDropShadowEffect(widget);
        shadow->setBlurRadius(blurRadius);
        shadow->setOffset(0, yOffset);
        shadow->setColor(QColor(0, 0, 0, 160));
        widget->setGraphicsEffect(shadow);
    }

    // Correspondance nom de bille -> couleur d'affichage de la pastille.
    QString ballColorHex(const QString& ballName)
    {
        if (ballName == "Rouge") return "#ff3b30";
        if (ballName == "Jaune") return "#ffcc00";
        if (ballName == "Verte") return "#00c853";
        if (ballName == "Marron") return "#8b4513";
        if (ballName == "Bleue") return "#1565ff";
        if (ballName == "Rose") return "#ff4fa3";
        if (ballName == "Noire") return "#161616";
        // "Couleur" (n'importe laquelle est legale) ou cas non prevu.
        return "#7a7f87";
    }

    // Style QSS d'une bille "brillante" (degrade radial + reflet), a la
    // place d'un simple aplat colore, pour un rendu plus realiste.
    QString ballGlossStyle(const QString& ballName, int diameterPx)
    {
        QColor base(ballColorHex(ballName));
        QColor highlight = base.lighter(230);
        QColor shadow = base.darker(200);
        QColor deepShadow = base.darker(280);
        int radius = diameterPx / 2;

        return QString(
            "background: qradialgradient(cx:0.32, cy:0.28, radius:0.9, fx:0.32, fy:0.28,"
            " stop:0 #ffffff, stop:0.12 %1, stop:0.45 %2, stop:0.8 %3, stop:1 %4);"
            "border-radius: %5px;"
            "border: 1px solid %6;"
        ).arg(highlight.name(), base.name(), shadow.name(), deepShadow.name(),
            QString::number(radius), deepShadow.darker(130).name());
    }

    // Liste des joueurs deja utilises, sauvegardee dans un fichier
    // local a cote de l'executable (pas d'historique de parties,
    // juste les noms pour les retrouver rapidement au demarrage).
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

    // Formate un nombre de secondes en "hh:mm:ss" pour l'affichage des durees.
    QString formatDuration(qint64 totalSeconds)
    {
        qint64 hours = totalSeconds / 3600;
        qint64 minutes = (totalSeconds % 3600) / 60;
        qint64 seconds = totalSeconds % 60;
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    // Texte du badge "MEILLEUR DES X FRAMES" : cas particulier pour 1
    // frame ("FRAME UNIQUE" plutot que le grammaticalement bancal
    // "MEILLEUR DES 1 FRAMES"), voir framesToWin dans promptPlayerNames().
    QString formatBestOfLabel(int framesToWin)
    {
        if (framesToWin <= 1)
        {
            return "FRAME UNIQUE";
        }
        return "MEILLEUR DES " + QString::number(2 * framesToWin - 1) + " FRAMES";
    }

    // Calcule, dans l'ordre chronologique, la succession des breaks de ce
    // joueur (le total de chaque serie de coups d'affilee sans
    // interruption) et la succession des points de chaque faute adverse
    // dont il a beneficie -- pour l'affichage "detail des points" (voir
    // buildDetailBox()/refreshDisplay()), qui montre desormais le detail
    // coup par coup plutot qu'une simple somme. Un Miss ou une Faute
    // termine le break en cours (celui-ci est ajoute a breaksOut si non
    // vide) ; Bille touchante/Remettre en place n'interrompent pas le
    // break (le meme joueur continue son tour).
    void computePointsBreakdown(
        const ShotHistory& history,
        const std::string& playerName,
        std::vector<int>& breaksOut,
        std::vector<int>& foulsReceivedOut
    )
    {
        breaksOut.clear();
        foulsReceivedOut.clear();

        std::string currentBreakPlayer;
        int currentBreakTotal = 0;

        for (const LogEntry& entry : history.getLog())
        {
            if (entry.type == LogEntry::Type::Shot)
            {
                if (entry.playerName == currentBreakPlayer)
                {
                    currentBreakTotal += entry.points;
                }
                else
                {
                    if (currentBreakTotal > 0 && currentBreakPlayer == playerName)
                    {
                        breaksOut.push_back(currentBreakTotal);
                    }
                    currentBreakPlayer = entry.playerName;
                    currentBreakTotal = entry.points;
                }
            }
            else if (entry.type == LogEntry::Type::Foul || entry.type == LogEntry::Type::Miss)
            {
                if (currentBreakTotal > 0 && currentBreakPlayer == playerName)
                {
                    breaksOut.push_back(currentBreakTotal);
                }
                currentBreakPlayer.clear();
                currentBreakTotal = 0;

                if (entry.type == LogEntry::Type::Foul && entry.playerName != playerName)
                {
                    foulsReceivedOut.push_back(entry.foulPoints);
                }
            }
            // TouchingBall/Replay : ignores ici, ils n'interrompent pas le
            // tour du meme joueur et ne comptent aucun point en propre.
        }

        if (currentBreakTotal > 0 && currentBreakPlayer == playerName)
        {
            breaksOut.push_back(currentBreakTotal);
        }
    }

    // Description courte et lisible d'une entree du journal (Shot ou Foul
    // uniquement -- les seuls types qu'on peut raisonnablement "corriger"),
    // utilisee par le bouton "Correction arbitre" pour batir le texte
    // "avant -> apres" (voir Frame::logCorrection()).
    QString describeLogEntry(const LogEntry& entry)
    {
        if (entry.type == LogEntry::Type::Shot)
        {
            return QString::fromStdString(entry.ballName)
                + " (+" + QString::number(entry.points) + ")";
        }
        if (entry.type == LogEntry::Type::Foul)
        {
            return "FAUTE (bille jouee : "
                + QString::fromStdString(entry.touchedBall)
                + ") -- adverse +" + QString::number(entry.foulPoints);
        }
        return "?";
    }

    // Joint une liste d'entiers avec "." comme separateur (ex. "15.25.36"),
    // ou "0" si la liste est vide -- voir computePointsBreakdown().
    QString joinPointsList(const std::vector<int>& values)
    {
        if (values.empty())
        {
            return "0";
        }
        QStringList parts;
        for (int v : values)
        {
            parts << QString::number(v);
        }
        // Separateur ". " (avec espace) plutot que "." seul : QLabel ne
        // peut retourner a la ligne qu'aux espaces (voir setWordWrap() sur
        // valueLabelOut dans buildDetailRow()), indispensable des que la
        // liste s'allonge au fil de la frame et deborde de la largeur fixe
        // du panneau (280px, voir buildDetailBox()).
        return parts.join(". ");
    }

    // Une ligne du panneau "detail des points" : libelle a gauche, valeur
    // a droite. Retourne le widget de ligne ; ecrit le label de valeur
    // (a mettre a jour dans refreshDisplay) dans valueLabelOut.
    QWidget* buildDetailRow(QWidget* parent, const QString& label, QLabel*& valueLabelOut)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        QLabel* labelWidget = new QLabel(label, row);
        labelWidget->setStyleSheet("color: " + kGray + "; font-size: 11px;");

        valueLabelOut = new QLabel("0", row);
        valueLabelOut->setAlignment(Qt::AlignRight);
        // La succession des breaks/fautes (voir computePointsBreakdown())
        // peut s'allonger bien au-dela de la largeur fixe du panneau au
        // fil d'une longue frame : sans le retour a la ligne, le texte
        // deborderait silencieusement hors du cadre.
        valueLabelOut->setWordWrap(true);
        valueLabelOut->setStyleSheet("color: " + kWhite + "; font-size: 11px; font-weight: bold;");

        rowLayout->addWidget(labelWidget);
        rowLayout->addStretch();
        rowLayout->addWidget(valueLabelOut);
        return row;
    }

    // Panneau "detail des points" : billes empochees normalement et
    // fautes adverses (le Free ball n'est plus affiche separement ici,
    // mais ses points restent inclus dans le score total du joueur
    // affiche dans le cadre "points marques" au-dessus).
    QFrame* buildDetailBox(
        QWidget* parent,
        const QString& borderColor,
        QLabel*& pottedOut,
        QLabel*& foulsOut
    )
    {
        QFrame* detailBox = new QFrame(parent);
        detailBox->setFixedWidth(280);
        detailBox->setStyleSheet("background: transparent; border: none;");
        QVBoxLayout* detailLayout = new QVBoxLayout(detailBox);
        detailLayout->setContentsMargins(16, 12, 16, 12);
        detailLayout->setSpacing(4);

        QLabel* detailTitle = new QLabel("DETAIL DES POINTS", detailBox);
        detailTitle->setStyleSheet("color: " + kGray + "; font-size: 10px; letter-spacing: 1px;");
        detailLayout->addWidget(detailTitle);

        detailLayout->addWidget(buildDetailRow(detailBox, "Break", pottedOut));
        detailLayout->addWidget(buildDetailRow(detailBox, "Fautes adverses", foulsOut));

        return detailBox;
    }

    // Affiche une boite de dialogue listant l'historique des matchs
    // sauvegardes (le plus recent en premier), lu depuis matchs.json.
    void showMatchHistoryDialog(QWidget* parent)
    {
        QJsonArray history = MatchStorage::loadHistory();

        QDialog dialog(parent);
        dialog.setWindowTitle("Historique des matchs");
        dialog.setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");
        dialog.resize(560, 480);

        QVBoxLayout* layout = new QVBoxLayout(&dialog);

        QListWidget* list = new QListWidget(&dialog);
        list->setStyleSheet(
            "QListWidget {"
            "  background-color: " + kPanel + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  color: " + kWhite + ";"
            "  font-size: 12px;"
            "}"
            "QListWidget::item {"
            "  padding: 8px;"
            "  border-bottom: 1px solid " + kBorder + ";"
            "}"
        );
        layout->addWidget(list);

        if (history.isEmpty())
        {
            list->addItem("Aucun match sauvegarde pour l'instant.");
        }
        else
        {
            for (int i = history.size() - 1; i >= 0; --i)
            {
                QJsonObject matchObj = history[i].toObject();
                QString joueur1 = matchObj["joueur1"].toString();
                QString joueur2 = matchObj["joueur2"].toString();
                int frames1 = matchObj["frames_joueur1"].toInt();
                int frames2 = matchObj["frames_joueur2"].toInt();
                QString date = matchObj["date"].toString();

                QString text = date + "\n" + joueur1 + " " + QString::number(frames1)
                    + " - " + QString::number(frames2) + " " + joueur2;

                QJsonArray frames = matchObj["frames"].toArray();
                for (int f = 0; f < frames.size(); ++f)
                {
                    QJsonObject frameObj = frames[f].toObject();
                    text += "\n  Frame " + QString::number(f + 1) + " : "
                        + frameObj["vainqueur"].toString() + " ("
                        + QString::number(frameObj["score_joueur1"].toInt()) + " - "
                        + QString::number(frameObj["score_joueur2"].toInt()) + ")";
                }

                list->addItem(text);
            }
        }

        QPushButton* closeButton = new QPushButton("Fermer", &dialog);
        closeButton->setStyleSheet(
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  padding: 8px 10px;"
            "}"
        );
        QObject::connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        layout->addWidget(closeButton);

        dialog.exec();
    }

    // Statistiques cumulees d'un joueur sur tous les matchs sauvegardes
    // (identifie par son nom).
    struct PlayerStats
    {
        int matchesPlayed = 0;
        int matchesWon = 0;
        int framesWon = 0;
        int bestFrameScore = 0;
    };

    // Affiche une boite de dialogue de statistiques, calculees a la volee
    // a partir de l'historique des matchs sauvegardes (matchs.json).
    void showStatsDialog(QWidget* parent)
    {
        QJsonArray history = MatchStorage::loadHistory();

        QMap<QString, PlayerStats> stats;
        int totalFrames = 0;

        for (const QJsonValue& matchVal : history)
        {
            QJsonObject matchObj = matchVal.toObject();
            QString joueur1 = matchObj["joueur1"].toString();
            QString joueur2 = matchObj["joueur2"].toString();
            int frames1 = matchObj["frames_joueur1"].toInt();
            int frames2 = matchObj["frames_joueur2"].toInt();

            stats[joueur1].matchesPlayed++;
            stats[joueur2].matchesPlayed++;

            if (frames1 > frames2)
            {
                stats[joueur1].matchesWon++;
            }
            else if (frames2 > frames1)
            {
                stats[joueur2].matchesWon++;
            }

            QJsonArray frames = matchObj["frames"].toArray();
            for (const QJsonValue& frameVal : frames)
            {
                QJsonObject frameObj = frameVal.toObject();
                totalFrames++;

                QString winner = frameObj["vainqueur"].toString();
                if (stats.contains(winner))
                {
                    stats[winner].framesWon++;
                }

                int score1 = frameObj["score_joueur1"].toInt();
                int score2 = frameObj["score_joueur2"].toInt();
                stats[joueur1].bestFrameScore = std::max(stats[joueur1].bestFrameScore, score1);
                stats[joueur2].bestFrameScore = std::max(stats[joueur2].bestFrameScore, score2);
            }
        }

        QDialog dialog(parent);
        dialog.setWindowTitle("Statistiques");
        dialog.setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");
        dialog.resize(560, 420);

        QVBoxLayout* layout = new QVBoxLayout(&dialog);

        QLabel* summary = new QLabel(
            QString::number(history.size()) + " match(s) joue(s) - "
            + QString::number(totalFrames) + " frame(s) jouee(s)",
            &dialog
        );
        summary->setStyleSheet("color: " + kGray + "; font-size: 12px;");
        layout->addWidget(summary);

        QTableWidget* table = new QTableWidget(&dialog);
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels(
            { "Joueur", "Matchs", "Matchs gagnes", "Frames gagnees", "Meilleur score" }
        );
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget {"
            "  background-color: " + kPanel + ";"
            "  border: 1px solid " + kBorder + ";"
            "  color: " + kWhite + ";"
            "  font-size: 12px;"
            "  gridline-color: " + kBorder + ";"
            "}"
            "QHeaderView::section {"
            "  background-color: " + kBg + ";"
            "  color: " + kGray + ";"
            "  border: none;"
            "  padding: 4px;"
            "  font-size: 11px;"
            "}"
        );
        layout->addWidget(table);

        // Classement par nombre de matchs gagnes decroissant.
        QVector<QPair<QString, PlayerStats>> sortedStats;
        for (auto it = stats.constBegin(); it != stats.constEnd(); ++it)
        {
            sortedStats.append({ it.key(), it.value() });
        }
        std::sort(sortedStats.begin(), sortedStats.end(), [](const auto& a, const auto& b)
            {
                return a.second.matchesWon > b.second.matchesWon;
            });

        table->setRowCount(sortedStats.size());
        for (int row = 0; row < sortedStats.size(); ++row)
        {
            const QString& name = sortedStats[row].first;
            const PlayerStats& s = sortedStats[row].second;

            table->setItem(row, 0, new QTableWidgetItem(name));
            table->setItem(row, 1, new QTableWidgetItem(QString::number(s.matchesPlayed)));
            table->setItem(row, 2, new QTableWidgetItem(QString::number(s.matchesWon)));
            table->setItem(row, 3, new QTableWidgetItem(QString::number(s.framesWon)));
            table->setItem(row, 4, new QTableWidgetItem(QString::number(s.bestFrameScore)));
        }

        if (history.isEmpty())
        {
            QLabel* empty = new QLabel("Aucun match sauvegarde pour l'instant.", &dialog);
            empty->setStyleSheet("color: " + kGray + ";");
            layout->addWidget(empty);
        }

        QPushButton* closeStatsButton = new QPushButton("Fermer", &dialog);
        closeStatsButton->setStyleSheet(
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  padding: 8px 10px;"
            "}"
        );
        QObject::connect(closeStatsButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        layout->addWidget(closeStatsButton);

        dialog.exec();
    }

    // Affiche le guide de repositionnement (image generee par
    // BallMapRecorder::renderRepositioningGuide) en PLEIN ECRAN :
    // pointille = position cible, plein = position actuelle, fleche = a
    // deplacer, "MANQUANTE" = bille absente. Plein ecran a la demande de
    // l'utilisateur (visibilite depuis l'autre bout de la table).
    // NON MODAL (setWindowModality(Qt::NonModal) + show(), pas exec()) et
    // alloue sur le tas avec WA_DeleteOnClose : la fenetre principale
    // (et donc les telecommandes 1.0/2.0) reste utilisable pendant que
    // le guide est affiche, notamment sur une installation a deux ecrans
    // (guide plein ecran sur l'un, telecommande sur l'autre). Le pointeur
    // retourne est stocke par l'appelant (MainWindow::m_repositionGuideDialog)
    // pour permettre sa fermeture depuis N'IMPORTE QUELLE telecommande :
    // bouton "Fermer" sur le dialogue lui-meme, Echap au clavier, action
    // telephone "closeRepositionGuide" (voir kPageHtml), OU bouton
    // "Fermer le guide" des telecommandes 1.0/2.0 (voir
    // MainWindow::closeRepositioningGuide()).
    QDialog* showRepositioningGuideDialog(QWidget* parent, const cv::Mat& guideImage, MatchWebServer* webServer)
    {
        cv::Mat rgb;
        cv::cvtColor(guideImage, rgb, cv::COLOR_BGR2RGB);
        QImage qImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);

        QDialog* dialog = new QDialog(parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowModality(Qt::NonModal);
        dialog->setWindowTitle("Guide de repositionnement");
        dialog->setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

        QVBoxLayout* layout = new QVBoxLayout(dialog);
        layout->setAlignment(Qt::AlignCenter);

        QLabel* imageLabel = new QLabel(dialog);
        QPixmap pixmap = QPixmap::fromImage(qImage.copy());
        QSize screenSize = QGuiApplication::primaryScreen()
            ? QGuiApplication::primaryScreen()->availableSize()
            : QSize(1280, 720);
        imageLabel->setPixmap(pixmap.scaled(
            screenSize * 0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation
        ));
        imageLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(imageLabel);

        QPushButton* closeGuideButton = new QPushButton("Fermer", dialog);
        closeGuideButton->setStyleSheet(
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  padding: 12px 24px;"
            "  font-size: 15px;"
            "}"
        );
        QObject::connect(closeGuideButton, &QPushButton::clicked, dialog, &QDialog::close);
        layout->addWidget(closeGuideButton, 0, Qt::AlignHCenter);

        QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), dialog);
        QObject::connect(escShortcut, &QShortcut::activated, dialog, &QDialog::close);

        if (webServer)
        {
            QObject::connect(webServer, &MatchWebServer::controlActionRequested, dialog,
                [dialog](const QString& action, const QJsonObject&)
                {
                    if (action == "closeRepositionGuide")
                    {
                        dialog->close();
                    }
                });
        }

        dialog->showFullScreen();
        return dialog;
    }

    // Exporte l'etat courant (scores, joueur au tir, break) et le journal
    // complet des coups/fautes dans un fichier texte horodate, dans un
    // dossier dedie "scenarios_test" a cote de l'executable, pour pouvoir
    // signaler un scenario/bug precisement (pas de captures d'ecran ni de
    // copier-coller a decouper a la main). Un nom different a chaque export
    // (horodatage a la seconde pres) : aucun scenario precedent n'est ecrase.
    // Ecrit le journal des coups de la frame dans le flux donne (en-tete +
    // toutes les lignes) : logique commune a l'export manuel
    // (exportMoveLogToFile) et a la sauvegarde automatique silencieuse
    // (autoSaveMoveLog), pour ne pas dupliquer le format.
    void writeMoveLog(QTextStream& out, Match& match, Frame& frame)
    {
        out << "=== Export du journal des coups ===\n";
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
        out << "Joueur 1 : " << QString::fromStdString(frame.getPlayer1().getName()) << "\n";
        out << "Joueur 2 : " << QString::fromStdString(frame.getPlayer2().getName()) << "\n";
        out << "Joueur au tir : " << QString::fromStdString(frame.currentPlayer().getName()) << "\n\n";

        const std::vector<LogEntry>& log = frame.getHistory().getLog();
        for (size_t i = 0; i < log.size(); ++i)
        {
            const LogEntry& entry = log[i];
            if (entry.type == LogEntry::Type::Shot)
            {
                out << (i + 1) << ". " << QString::fromStdString(entry.playerName)
                    << " -- " << QString::fromStdString(entry.ballName)
                    << " (+" << entry.points << ")\n";
            }
            else if (entry.type == LogEntry::Type::Miss)
            {
                // Mot-cle "MISS" en majuscules, seul sur cette position,
                // pour que "Rejouer le scenario" puisse le reperer sans
                // ambiguite en reparcourant ce meme fichier.
                out << (i + 1) << ". " << QString::fromStdString(entry.playerName)
                    << " -- MISS (fin de tour, aucune bille jouee)\n";
            }
            else if (entry.type == LogEntry::Type::TouchingBall)
            {
                // Meme principe que MISS ci-dessus : mot-cle repere sans
                // ambiguite par parseScenarioFile().
                out << (i + 1) << ". " << QString::fromStdString(entry.playerName)
                    << " -- BILLE TOUCHANTE (premier contact deja valide pour le prochain coup)\n";
            }
            else if (entry.type == LogEntry::Type::Replay)
            {
                // Meme principe : mot-cle repere sans ambiguite par
                // parseScenarioFile(). playerName est deja celui qui
                // REJOUE (voir Frame::requestReplay()/ShotHistory::addReplay()).
                out << (i + 1) << ". " << QString::fromStdString(entry.playerName)
                    << " -- REMETTRE EN PLACE (rejoue depuis la position)\n";
            }
            else if (entry.type == LogEntry::Type::Correction)
            {
                // Trace d'une correction d'arbitre (voir Frame::logCorrection()).
                // Non rejouable via "Rejouer le scenario" (parseScenarioFile()
                // l'ignore volontairement) : reproduire fidelement une
                // correction demanderait de rejouer l'annulation ET le bon
                // coup, ce qui n'apporte rien de plus qu'exporter directement
                // le bon coup -- seule la trace visuelle "avant -> apres"
                // compte ici, pour l'arbitre humain qui relit le journal.
                out << (i + 1) << ". CORRECTION ARBITRE : "
                    << QString::fromStdString(entry.correctionBefore)
                    << " -> " << QString::fromStdString(entry.correctionAfter) << "\n";
            }
            else
            {
                out << (i + 1) << ". FAUTE -- " << QString::fromStdString(entry.playerName)
                    << " : " << QString::fromStdString(entry.reason)
                    << " (bille demandee : " << QString::fromStdString(entry.requiredBall)
                    << ", bille jouee : " << QString::fromStdString(entry.touchedBall)
                    << ") -- adverse +" << entry.foulPoints << "\n";
            }
        }

        // Resume en fin de journal : score de CETTE frame (deja visible
        // dans l'en-tete, repete ici pour une lecture rapide en bas de
        // fichier) et score du match en nombre de frames gagnees.
        //
        // Match::checkFrameEnd() incremente le tally des frames DES que
        // Frame::isFinished() devient vrai (avant meme que Match bascule
        // sur l'objet Frame suivant, qui n'arrive que plus tard via
        // proceedToNextFrame()) : match.getFramesPlayer1()/2() est donc
        // deja a jour ici, y compris pour le tout dernier fichier de la
        // frame qui vient de se terminer -- pas besoin de l'anticiper.
        out << "\n";
        out << "Break en cours : " << frame.currentPlayer().getBreak() << "\n";
        out << "Score de la frame : " << QString::fromStdString(frame.getPlayer1().getName())
            << " " << frame.getPlayer1().getScore() << " - "
            << QString::fromStdString(frame.getPlayer2().getName())
            << " " << frame.getPlayer2().getScore() << "\n";
        out << "Score du match (frames) : " << QString::fromStdString(frame.getPlayer1().getName())
            << " " << match.getFramesPlayer1() << " - "
            << QString::fromStdString(frame.getPlayer2().getName())
            << " " << match.getFramesPlayer2() << "\n";
    }

    void exportMoveLogToFile(QWidget* parent, Match& match, Frame& frame)
    {
        QString folder = QCoreApplication::applicationDirPath() + "/scenarios_test";
        QDir().mkpath(folder);

        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString path = folder + "/scenario_" + timestamp + ".txt";

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            showStyledMessage(parent, QMessageBox::Warning, "Export du journal", "Impossible d'ecrire le fichier :\n" + path);
            return;
        }

        QTextStream out(&file);
        writeMoveLog(out, match, frame);

        file.close();
        showStyledMessage(parent, QMessageBox::Information, "Export du journal", "Journal exporte :\n" + path);
    }

    // Rendu du contenu d'UNE frame (numero + journal complet + score),
    // prefixe d'un separateur, pour etre concatene dans le fichier unique
    // du match par autoSaveMoveLog() ci-dessous.
    QString renderFrameSection(int frameNumber, Match& match, Frame& frame)
    {
        QString section;
        QTextStream out(&section);
        out << "\n\n========== FRAME " << frameNumber << " ==========\n";
        writeMoveLog(out, match, frame);
        return section;
    }

    // Etat de la sauvegarde automatique du match en cours, regroupe ici
    // pour pouvoir tout reinitialiser proprement au debut d'un nouveau
    // match (voir resetAutoSaveTracking(), appelee depuis
    // beginMatch()/restartMatch()) : sans cette remise a zero, un second
    // match dans la meme session continuerait a ecrire dans le fichier
    // (et a numeroter les frames) du match precedent.
    QString s_autoSaveMatchFilePath;
    QString s_autoSaveMatchLog;              // Sections des frames definitivement closes.
    int s_autoSaveFrameNumber = 0;
    bool s_autoSaveWasAwaitingNextFrame = false;
    int s_autoSaveLastTallySum = 0;
    bool s_autoSaveFrameSectionClosed = false; // La frame en cours a-t-elle deja ete ajoutee a s_autoSaveMatchLog ?
    int s_autoSaveLastClosedSectionLength = 0; // Longueur de la derniere section ajoutee (pour annulation via "Retour").

    void resetAutoSaveTracking()
    {
        s_autoSaveMatchFilePath.clear();
        s_autoSaveMatchLog.clear();
        s_autoSaveFrameNumber = 0;
        s_autoSaveWasAwaitingNextFrame = false;
        s_autoSaveLastTallySum = 0;
        s_autoSaveFrameSectionClosed = false;
        s_autoSaveLastClosedSectionLength = 0;
    }

    // Sauvegarde automatique et silencieuse du journal du MATCH en cours,
    // appelee apres chaque coup (voir refreshDisplay()) : un seul fichier
    // par match, cree au premier coup et reecrit en entier a chaque appel
    // (pas de fichiers intermediaires a nettoyer). Il accumule la section
    // de chaque frame terminee, plus la section (encore provisoire) de la
    // frame en cours. Aucune boite de dialogue : un echec d'ecriture ne
    // doit jamais interrompre la partie.
    void autoSaveMoveLog(Match& match, Frame& frame)
    {
        QString folder = QCoreApplication::applicationDirPath() + "/scenarios_test";
        QDir().mkpath(folder);

        if (s_autoSaveMatchFilePath.isEmpty())
        {
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            s_autoSaveMatchFilePath = folder + "/auto_match_" + timestamp + ".txt";
        }
        if (s_autoSaveFrameNumber == 0)
        {
            s_autoSaveFrameNumber = 1;
        }

        // Signal utilise pour detecter qu'une frame vient de se terminer
        // (ou qu'une telle fin vient d'etre annulee via "Retour") : voir
        // [[project_frame_lifecycle_timing]] en memoire pour le detail --
        // en resume, Match::isFrameJustFinished() passe a vrai des que la
        // frame est gagnee (avant meme que Match bascule sur la frame
        // suivante), et repasse a faux soit lors d'une vraie avancee, soit
        // lors d'une annulation via Match::undoFrameConclusion() -- les
        // deux se distinguent par le tally, qui redescend uniquement dans
        // le second cas.
        bool isAwaitingNextFrame = match.isFrameJustFinished();
        int tallySum = match.getFramesPlayer1() + match.getFramesPlayer2();

        if (isAwaitingNextFrame && !s_autoSaveWasAwaitingNextFrame)
        {
            // La frame en cours vient de se terminer : sa section devient
            // definitive et rejoint le journal du match.
            QString section = renderFrameSection(s_autoSaveFrameNumber, match, frame);
            s_autoSaveMatchLog += section;
            s_autoSaveLastClosedSectionLength = section.length();
            s_autoSaveFrameSectionClosed = true;
        }
        else if (!isAwaitingNextFrame && s_autoSaveWasAwaitingNextFrame)
        {
            if (tallySum < s_autoSaveLastTallySum)
            {
                // "Retour" vient d'annuler le coup qui terminait la frame :
                // on retire la section qu'on venait d'ajouter, la frame
                // reprend sous le meme numero.
                s_autoSaveMatchLog.chop(s_autoSaveLastClosedSectionLength);
                s_autoSaveFrameSectionClosed = false;
            }
            else if (!match.isMatchFinished())
            {
                // Avancee reelle sur la frame suivante.
                s_autoSaveFrameSectionClosed = false;
                ++s_autoSaveFrameNumber;
            }
            // Sinon (match termine) : m_awaitingNextFrame repasse aussi a
            // faux quand proceedToNextFrame() s'execute (elle le fait
            // inconditionnellement), mais sans demarrer de nouvelle frame
            // puisque le match est fini -- getCurrentFrame() continue de
            // pointer sur la MEME frame, deja close ci-dessus. Ne rien
            // faire ici : re-ouvrir sa section dupliquerait son contenu
            // (observe en test : une "FRAME 4" identique a la "FRAME 3").
        }
        s_autoSaveWasAwaitingNextFrame = isAwaitingNextFrame;
        s_autoSaveLastTallySum = tallySum;

        QString fullText = s_autoSaveMatchLog;
        if (!s_autoSaveFrameSectionClosed)
        {
            fullText += renderFrameSection(s_autoSaveFrameNumber, match, frame);
        }

        QFile file(s_autoSaveMatchFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            return;
        }
        QTextStream out(&file);
        out << fullText;
    }

    // Valeur standard d'une bille par son nom (regles du snooker), utilisee
    // par la telecommande (boutons de billes, choix du Free Ball).
    int standardBallValue(const QString& ballName)
    {
        if (ballName == "Rouge") return 1;
        if (ballName == "Jaune") return 2;
        if (ballName == "Verte") return 3;
        if (ballName == "Marron") return 4;
        if (ballName == "Bleue") return 5;
        if (ballName == "Rose") return 6;
        if (ballName == "Noire") return 7;
        return 0;
    }

    // Une action extraite d'un fichier scenario_*.txt, dans l'ordre.
    struct ReplayAction
    {
        // TouchingBall = arme Frame::setTouchingBall(true) sans jouer de
        // coup (voir bouton "Bille touchante"). BlancheOffTable = faute
        // directe sur la blanche (voir triggerBlancheOffTableFoul()) --
        // distinct de Shot car Frame::playShot() ne sait pas traiter la
        // blanche comme une bille jouable normale. DirectFoul = faute
        // "Miss" pure ou la bille jouee EST la bille demandee (aucune
        // bille distincte touchee, ex. raté complet) : rejouer via
        // Frame::playShot() la compterait a tort comme un coup legal,
        // puisque la bille jouee est justement celle qui est due a cet
        // instant -- voir Frame::foul() appele directement au lieu de
        // playShot() dans l'executeur de rejeu. FreeBallShot = bille
        // empochee pendant un Free Ball (voir Frame::playFreeBall()) : la
        // valeur enregistree au journal ("BallName (+X)") ne correspond
        // PAS a la valeur standard de cette bille (ex. "Marron (+1)" au
        // lieu de "+4") quand elle remplace une rouge -- rejouer via
        // playShot() compterait la mauvaise valeur ET jugerait ce coup
        // fautif a tort (playShot() ignore totalement le Free Ball).
        // FreeBallFoul = faute survenue PENDANT un Free Ball (bille
        // designee jouee fautivement, ex. touche une autre bille en
        // premier) -- reperee via le motif "Faute pendant un Free Ball"
        // (voir Frame::playFreeBall()) : rejouer via Frame::foul() seul
        // laisserait a tort m_freeBall arme pour le joueur suivant,
        // puisque playShot()/foul() ignorent l'etat Free Ball -- seul
        // Frame::playFreeBall() sait le desarmer correctement.
        // Replay = "Faire rejouer" (voir Frame::requestReplay()).
        enum class Kind { Shot, Miss, TouchingBall, BlancheOffTable, DirectFoul, FreeBallShot, FreeBallFoul, Replay };
        Kind kind;
        QString ballName; // vide sauf Kind::Shot/DirectFoul/FreeBallShot ; bille DESIGNEE pour FreeBallFoul
        QString touchedBallName; // uniquement Kind::FreeBallFoul : bille reellement touchee/jouee
    };

    // Recharge la liste des scenarios enregistres dans le menu deroulant
    // (les plus recents en premier), pour choisir lequel rejouer au lieu
    // de toujours rejouer le dernier. Le chemin complet est stocke en
    // donnee de chaque entree (userData), le texte affiche est horodate
    // lisiblement (le nom de fichier brut reste dans le chemin stocke).
    void refreshScenarioFileList(QComboBox* combo)
    {
        combo->clear();

        QString folder = QCoreApplication::applicationDirPath() + "/scenarios_test";
        QDir dir(folder);
        QStringList files = dir.entryList(QStringList() << "scenario_*.txt", QDir::Files, QDir::Name);

        if (files.isEmpty())
        {
            combo->addItem("Aucun scenario enregistre", QString());
            return;
        }

        // Plus recent en premier (les noms sont horodates "yyyyMMdd_HHmmss",
        // donc l'ordre alphabetique inverse = ordre chronologique inverse).
        for (int i = files.size() - 1; i >= 0; --i)
        {
            const QString& fileName = files[i];
            QString path = dir.filePath(fileName);

            // "scenario_20260821_185406.txt" -> "2026-08-21 18:54:06"
            QString stem = fileName;
            stem.remove("scenario_");
            stem.remove(".txt");
            QString display = stem;
            QStringList parts = stem.split('_');
            if (parts.size() == 2 && parts[0].length() == 8 && parts[1].length() == 6)
            {
                const QString& d = parts[0];
                const QString& t = parts[1];
                display = d.left(4) + "-" + d.mid(4, 2) + "-" + d.mid(6, 2)
                    + " " + t.left(2) + ":" + t.mid(2, 2) + ":" + t.mid(4, 2);
            }

            combo->addItem(display, path);
        }
    }

    // Reparse un fichier scenario_*.txt (voir exportMoveLogToFile) en une
    // sequence d'actions rejouables. Ne distingue pas coup legal/faute a
    // la lecture : la bille jouee est simplement re-tentee via
    // Frame::playShot(), qui redetermine elle-meme la legalite a partir
    // de l'etat du jeu reconstruit pas a pas -> reproduit fidelement le
    // meme resultat (legal ou faute) que lors de l'enregistrement original.
    std::vector<ReplayAction> parseScenarioFile(const QString& path)
    {
        std::vector<ReplayAction> actions;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return actions;
        }

        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();

            // Ne garde que les lignes numerotees du journal ("N. ...") :
            // ignore l'en-tete (date, scores, joueur au tir) et les lignes vides.
            int dotIdx = line.indexOf(". ");
            if (dotIdx <= 0)
            {
                continue;
            }
            QString prefix = line.left(dotIdx);
            bool allDigits = true;
            for (const QChar& c : prefix)
            {
                if (!c.isDigit()) { allDigits = false; break; }
            }
            if (!allDigits)
            {
                continue;
            }
            QString rest = line.mid(dotIdx + 2);

            if (rest.contains(" -- MISS"))
            {
                actions.push_back({ ReplayAction::Kind::Miss, QString() });
                continue;
            }

            if (rest.contains(" -- BILLE TOUCHANTE"))
            {
                actions.push_back({ ReplayAction::Kind::TouchingBall, QString() });
                continue;
            }

            if (rest.contains(" -- REMETTRE EN PLACE"))
            {
                actions.push_back({ ReplayAction::Kind::Replay, QString() });
                continue;
            }

            if (rest.startsWith("FAUTE -- "))
            {
                QString marker = "bille jouee : ";
                int idx = rest.indexOf(marker);
                if (idx < 0)
                {
                    continue;
                }
                QString remainder = rest.mid(idx + marker.length());
                int endIdx = remainder.indexOf(')');
                QString ballName = (endIdx >= 0) ? remainder.left(endIdx) : remainder;
                ballName = ballName.trimmed();

                // Bille demandee (voir writeMoveLog()), pour reperer le cas
                // "Miss" pur ci-dessous : la comparaison ne sert qu'a ca,
                // pas a rejouer la bille demandee elle-meme.
                QString requiredMarker = "bille demandee : ";
                QString requiredName;
                int reqIdx = rest.indexOf(requiredMarker);
                if (reqIdx >= 0)
                {
                    QString reqRemainder = rest.mid(reqIdx + requiredMarker.length());
                    int reqEndIdx = reqRemainder.indexOf(',');
                    requiredName = ((reqEndIdx >= 0) ? reqRemainder.left(reqEndIdx) : reqRemainder).trimmed();
                }

                // Motif explicite pose par Frame::playFreeBall() (voir
                // Kind::FreeBallFoul) : une faute survenue PENDANT un Free
                // Ball, ou "bille demandee" est en realite la bille
                // DESIGNEE (pas forcement une Rouge/Couleur ordinaire) et
                // "bille jouee" la bille reellement touchee a la place.
                bool isFreeBallFoul = rest.contains(": Faute pendant un Free Ball (");

                // Une faute "bille jouee : Blanche" n'est pas un coup
                // normal rejouable via Frame::playShot() (voir
                // triggerBlancheOffTableFoul(), la blanche n'est pas une
                // bille jouable comme les 7 autres) : traitement distinct.
                if (isFreeBallFoul && !requiredName.isEmpty())
                {
                    actions.push_back({ ReplayAction::Kind::FreeBallFoul, requiredName, ballName });
                }
                else if (ballName == "Blanche")
                {
                    actions.push_back({ ReplayAction::Kind::BlancheOffTable, QString() });
                }
                // Bille jouee == bille demandee : aucune bille distincte
                // n'a ete touchee (ex. Miss pur, "il ne touche aucune
                // rouge"). Rejouer via playShot() la compterait a tort
                // comme un coup legal, puisque cette bille EST justement
                // celle qui est due a cet instant -- voir Kind::DirectFoul.
                else if (!requiredName.isEmpty() && requiredName == ballName)
                {
                    actions.push_back({ ReplayAction::Kind::DirectFoul, ballName });
                }
                else
                {
                    actions.push_back({ ReplayAction::Kind::Shot, ballName });
                }
                continue;
            }

            // Ligne normale : "PlayerName -- BallName (+X)".
            int sep = rest.indexOf(" -- ");
            if (sep < 0)
            {
                continue;
            }
            QString afterSep = rest.mid(sep + 4);
            int parenIdx = afterSep.indexOf(" (+");
            QString ballName = (parenIdx >= 0) ? afterSep.left(parenIdx) : afterSep;
            ballName = ballName.trimmed();

            // Valeur reellement comptee pour ce coup (voir writeMoveLog()) :
            // si elle ne correspond pas a la valeur standard de la bille,
            // c'est qu'elle a ete jouee en Free Ball (voir Kind::FreeBallShot)
            // -- la valeur standard EST le cas normal (aucun Free Ball).
            int loggedPoints = -1;
            if (parenIdx >= 0)
            {
                int closeParen = afterSep.indexOf(')', parenIdx);
                QString pointsStr = afterSep.mid(parenIdx + 3, (closeParen >= 0 ? closeParen : afterSep.length()) - (parenIdx + 3));
                bool ok = false;
                int parsed = pointsStr.trimmed().toInt(&ok);
                if (ok)
                {
                    loggedPoints = parsed;
                }
            }

            if (loggedPoints >= 0 && loggedPoints != standardBallValue(ballName))
            {
                actions.push_back({ ReplayAction::Kind::FreeBallShot, ballName });
            }
            else
            {
                actions.push_back({ ReplayAction::Kind::Shot, ballName });
            }
        }

        return actions;
    }

    // QStackedWidget dimensionne par defaut sa sizeHint()/minimumSizeHint()
    // sur la PLUS GRANDE de ses pages, meme cachee : ici la vue de match
    // (page 0, dense en widgets) forcait ainsi la fenetre a etre plus
    // grande que l'ecran des le demarrage, alors que c'est l'accueil (page
    // 1, plus petit) qui est affiche en premier -- coupant le bas de
    // l'accueil hors ecran. On ne considere donc que la page courante.
    class CurrentPageStackedWidget : public QStackedWidget
    {
    public:
        explicit CurrentPageStackedWidget(QWidget* parent = nullptr) : QStackedWidget(parent) {}

        QSize sizeHint() const override
        {
            return currentWidget() ? currentWidget()->sizeHint() : QStackedWidget::sizeHint();
        }

        QSize minimumSizeHint() const override
        {
            return currentWidget() ? currentWidget()->minimumSizeHint() : QStackedWidget::minimumSizeHint();
        }
    };

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Snooker Player");

    setStyleSheet("background-color: " + kBg + ";");

    // Construit tot (avant la demande des noms de joueurs, voir plus bas) :
    // le partage Wi-Fi peut etre propose et demarre avant meme que le
    // premier match n'existe.
    m_webServer = new MatchWebServer(this);
    // La requete HTTP arrive sur le thread du serveur (voir MatchWebServer),
    // mais ce signal est emis vers un QObject qui vit sur le thread Qt/GUI
    // (m_webServer a `this` comme parent) : Qt met donc automatiquement la
    // livraison en file d'attente sur ce thread (AutoConnection resolue en
    // QueuedConnection), donc handleRemoteControlAction() s'execute bien
    // sur le thread GUI, jamais concurremment avec les clics de souris.
    connect(m_webServer, &MatchWebServer::controlActionRequested, this, &MainWindow::handleRemoteControlAction);

    // Demarre le partage Wi-Fi des l'ouverture de l'appli, pas seulement
    // quand l'utilisateur clique "Smartphone"/"Nouveau match" : le lien
    // (jeton + port persistes, voir MatchWebServer) reste le meme d'un
    // lancement a l'autre, donc un QR code imprime une seule fois et
    // colle sur la table fonctionne immediatement, meme si personne n'a
    // encore touche au PC. Echec silencieux ici (rare : port deja pris
    // par autre chose) -- la tuile "Smartphone" retentera plus tard.
    m_webServer->start();

    QWidget* central = new QWidget(this);
    QHBoxLayout* outerLayout = new QHBoxLayout(central);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(14);

    QWidget* scorePanel = new QWidget(central);
    QVBoxLayout* layout = new QVBoxLayout(scorePanel);
    layout->setSpacing(14);
    layout->setContentsMargins(24, 24, 24, 24);

    QFont scoreFont;
    scoreFont.setPointSize(22);
    scoreFont.setBold(true);

    m_player1Frame = new QFrame(central);
    m_player2Frame = new QFrame(central);

    // Nom du joueur centre au-dessus de son cadre "points marques"
    // (plus d'avatar : juste le role et le nom, alignes au centre).
    QVBoxLayout* p1Layout = new QVBoxLayout(m_player1Frame);
    p1Layout->setSpacing(2);
    p1Layout->setAlignment(Qt::AlignHCenter);

    QVBoxLayout* p2Layout = new QVBoxLayout(m_player2Frame);
    p2Layout->setSpacing(2);
    p2Layout->setAlignment(Qt::AlignHCenter);

    QFont roleFont;
    roleFont.setPointSize(10);
    roleFont.setBold(false);

    QFont nameFont;
    nameFont.setPointSize(32);
    nameFont.setBold(true);

    QFont scoreLineFont;
    scoreLineFont.setPointSize(11);

    m_player1RoleLabel = new QLabel("JOUEUR 1", m_player1Frame);
    m_player1RoleLabel->setFont(roleFont);
    m_player1RoleLabel->setAlignment(Qt::AlignHCenter);
    m_player1RoleLabel->setStyleSheet("color: " + kGreen + "; letter-spacing: 1px;");

    m_player2RoleLabel = new QLabel("JOUEUR 2", m_player2Frame);
    m_player2RoleLabel->setFont(roleFont);
    m_player2RoleLabel->setAlignment(Qt::AlignHCenter);
    m_player2RoleLabel->setStyleSheet("color: " + kOrange + "; letter-spacing: 1px;");

    m_player1NameLabel = new QLabel(m_player1Frame);
    m_player1NameLabel->setFont(nameFont);
    m_player1NameLabel->setAlignment(Qt::AlignHCenter);
    m_player1NameLabel->setStyleSheet("color: " + kWhite + ";");

    m_player2NameLabel = new QLabel(m_player2Frame);
    m_player2NameLabel->setFont(nameFont);
    m_player2NameLabel->setAlignment(Qt::AlignHCenter);
    m_player2NameLabel->setStyleSheet("color: " + kWhite + ";");

    p1Layout->addWidget(m_player1RoleLabel, 0, Qt::AlignHCenter);
    p1Layout->addWidget(m_player1NameLabel, 0, Qt::AlignHCenter);

    p2Layout->addWidget(m_player2RoleLabel, 0, Qt::AlignHCenter);
    p2Layout->addWidget(m_player2NameLabel, 0, Qt::AlignHCenter);

    // ---------------------------------------------------
    // Panneau centre : "Meilleur des X frames", score de
    // frames et frame en cours, comme sur la maquette.
    // ---------------------------------------------------
    QWidget* framesPanel = new QWidget(central);
    QVBoxLayout* framesLayout = new QVBoxLayout(framesPanel);
    framesLayout->setSpacing(4);
    framesLayout->setContentsMargins(0, 0, 0, 0);
    framesLayout->setAlignment(Qt::AlignHCenter);

    m_bestOfLabel = new QLabel(framesPanel);
    m_bestOfLabel->setAlignment(Qt::AlignHCenter);
    m_bestOfLabel->setStyleSheet("color: " + kWhite + "; font-size: 14px; letter-spacing: 1px;");

    m_frameScoreLabel = new QLabel(framesPanel);
    m_frameScoreLabel->setAlignment(Qt::AlignHCenter);
    QFont frameScoreFont;
    frameScoreFont.setPointSize(60); // +50% (demande utilisateur, sauf "points restants")
    frameScoreFont.setBold(true);
    m_frameScoreLabel->setFont(frameScoreFont);

    QLabel* frameWordLabel = new QLabel("FRAMES", framesPanel);
    frameWordLabel->setAlignment(Qt::AlignHCenter);
    frameWordLabel->setStyleSheet("color: " + kGray + "; font-size: 10px; letter-spacing: 1px;");

    QFrame* currentFrameBox = new QFrame(framesPanel);
    currentFrameBox->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kGreen + "; border-radius: 5px;"
    );
    QVBoxLayout* currentFrameLayout = new QVBoxLayout(currentFrameBox);
    currentFrameLayout->setContentsMargins(14, 8, 14, 8);
    currentFrameLayout->setSpacing(2);

    QLabel* currentFrameTitle = new QLabel("FRAME EN COURS", currentFrameBox);
    currentFrameTitle->setAlignment(Qt::AlignHCenter);
    currentFrameTitle->setStyleSheet("color: " + kGreen + "; font-size: 10px; letter-spacing: 1px; border: none; background: transparent;");

    m_currentFrameLabel = new QLabel(currentFrameBox);
    m_currentFrameLabel->setAlignment(Qt::AlignHCenter);
    QFont currentFrameFont;
    currentFrameFont.setPointSize(33); // +50% (demande utilisateur, sauf "points restants")
    currentFrameFont.setBold(true);
    m_currentFrameLabel->setFont(currentFrameFont);
    m_currentFrameLabel->setStyleSheet("color: " + kWhite + "; border: none; background: transparent;");

    currentFrameLayout->addWidget(currentFrameTitle);
    currentFrameLayout->addWidget(m_currentFrameLabel);

    framesLayout->addWidget(m_bestOfLabel);
    framesLayout->addWidget(m_frameScoreLabel);
    framesLayout->addWidget(frameWordLabel);
    framesLayout->addSpacing(6);
    framesLayout->addWidget(currentFrameBox, 0, Qt::AlignHCenter);

    // ---------------------------------------------------
    // Panneau Break / bille a jouer, comme sur la maquette.
    // ---------------------------------------------------
    QFrame* breakPanel = new QFrame(central);
    breakPanel->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 5px;"
    );
    breakPanel->setFixedWidth(280);
    QVBoxLayout* breakLayout = new QVBoxLayout(breakPanel);
    breakLayout->setContentsMargins(16, 20, 16, 20);
    breakLayout->setSpacing(6);
    breakLayout->setAlignment(Qt::AlignCenter);

    QLabel* breakTitle = new QLabel("BREAK", breakPanel);
    breakTitle->setAlignment(Qt::AlignHCenter);
    breakTitle->setStyleSheet("color: " + kGray + "; font-size: 11px; letter-spacing: 1px; border: none; background: transparent;");

    m_breakValueLabel = new QLabel(breakPanel);
    m_breakValueLabel->setAlignment(Qt::AlignHCenter);
    QFont breakFont;
    breakFont.setPointSize(72); // +50% (demande utilisateur, sauf "points restants")
    breakFont.setBold(true);
    m_breakValueLabel->setFont(breakFont);
    m_breakValueLabel->setStyleSheet("color: " + kWhite + "; border: none; background: transparent;");

    QLabel* nextBallTitle = new QLabel("BILLE A JOUER", breakPanel);
    nextBallTitle->setAlignment(Qt::AlignHCenter);
    nextBallTitle->setStyleSheet("color: " + kGray + "; font-size: 11px; letter-spacing: 1px; border: none; background: transparent;");

    m_nextBallDot = new QLabel(breakPanel);
    m_nextBallDot->setFixedSize(36, 36);
    m_nextBallDot->setStyleSheet(
        "background-color: " + kGray + "; border-radius: 18px;"
    );
    applySoftShadow(m_nextBallDot, 10, 3);

    breakLayout->addWidget(breakTitle);
    breakLayout->addWidget(m_breakValueLabel);
    breakLayout->addSpacing(4);
    breakLayout->addWidget(nextBallTitle);
    breakLayout->addWidget(m_nextBallDot, 0, Qt::AlignHCenter);

    // ---------------------------------------------------
    // Billes restantes sur la table : une pastille par couleur, avec le
    // nombre restant affiche dedans. Cachee automatiquement des qu'une
    // couleur est retiree definitivement (voir refreshDisplay()).
    // ---------------------------------------------------
    QLabel* remainingTitle = new QLabel("BILLES RESTANTES", breakPanel);
    remainingTitle->setAlignment(Qt::AlignHCenter);
    remainingTitle->setStyleSheet("color: " + kGray + "; font-size: 11px; letter-spacing: 1px; border: none; background: transparent;");

    QWidget* remainingBallsRow = new QWidget(breakPanel);
    QHBoxLayout* remainingBallsLayout = new QHBoxLayout(remainingBallsRow);
    remainingBallsLayout->setContentsMargins(0, 0, 0, 0);
    remainingBallsLayout->setSpacing(6);

    QVector<QString> remainingBallNames;
    remainingBallNames.append("Rouge");
    remainingBallNames.append("Jaune");
    remainingBallNames.append("Verte");
    remainingBallNames.append("Marron");
    remainingBallNames.append("Bleue");
    remainingBallNames.append("Rose");
    remainingBallNames.append("Noire");

    for (const QString& remainingBallName : remainingBallNames)
    {
        QLabel* remainingBallLabel = new QLabel(remainingBallsRow);
        remainingBallLabel->setFixedSize(28, 28);
        remainingBallLabel->setAlignment(Qt::AlignCenter);

        QColor base(ballColorHex(remainingBallName));
        QColor textColor = (base.lightness() > 150) ? QColor(kBg) : QColor(kWhite);
        remainingBallLabel->setStyleSheet(
            ballGlossStyle(remainingBallName, 28)
            + "color: " + textColor.name() + "; font-weight: bold; font-size: 11px;"
        );
        applySoftShadow(remainingBallLabel, 6, 2);

        remainingBallsLayout->addWidget(remainingBallLabel);
        m_remainingBallLabels[remainingBallName] = remainingBallLabel;
    }

    breakLayout->addSpacing(4);
    breakLayout->addWidget(remainingTitle);
    breakLayout->addWidget(remainingBallsRow, 0, Qt::AlignHCenter);

    // ---------------------------------------------------
    // Panneaux "score" dedies, un par joueur, positionnes
    // de part et d'autre du panneau Break/Bille a jouer.
    // ---------------------------------------------------
    m_player1ScoreBox = new QFrame(central);
    // Style de base (border fine verte) ; refreshDisplay() le remplace par
    // un style en surbrillance quand ce joueur est au tir.
    m_player1ScoreBox->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kGreen + "; border-radius: 5px;"
    );
    m_player1ScoreBox->setFixedWidth(280);
    QVBoxLayout* player1ScoreLayout = new QVBoxLayout(m_player1ScoreBox);
    player1ScoreLayout->setContentsMargins(16, 20, 16, 20);
    player1ScoreLayout->setAlignment(Qt::AlignCenter);

    m_player1ScoreBoxValue = new QLabel(m_player1ScoreBox);
    m_player1ScoreBoxValue->setAlignment(Qt::AlignHCenter);
    QFont scoreBoxFont;
    scoreBoxFont.setPointSize(72); // +50% (demande utilisateur, sauf "points restants")
    scoreBoxFont.setBold(true);
    m_player1ScoreBoxValue->setFont(scoreBoxFont);
    m_player1ScoreBoxValue->setStyleSheet("color: " + kWhite + "; border: none; background: transparent;");

    QLabel* player1ScoreTitle = new QLabel("POINTS MARQUES", m_player1ScoreBox);
    player1ScoreTitle->setAlignment(Qt::AlignHCenter);
    player1ScoreTitle->setStyleSheet("color: " + kGreen + "; font-size: 11px; letter-spacing: 1px; border: none; background: transparent;");

    player1ScoreLayout->addWidget(m_player1ScoreBoxValue);
    player1ScoreLayout->addWidget(player1ScoreTitle);

    m_player2ScoreBox = new QFrame(central);
    m_player2ScoreBox->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kOrange + "; border-radius: 5px;"
    );
    m_player2ScoreBox->setFixedWidth(280);
    QVBoxLayout* player2ScoreLayout = new QVBoxLayout(m_player2ScoreBox);
    player2ScoreLayout->setContentsMargins(16, 20, 16, 20);
    player2ScoreLayout->setAlignment(Qt::AlignCenter);

    m_player2ScoreBoxValue = new QLabel(m_player2ScoreBox);
    m_player2ScoreBoxValue->setAlignment(Qt::AlignHCenter);
    m_player2ScoreBoxValue->setFont(scoreBoxFont);
    m_player2ScoreBoxValue->setStyleSheet("color: " + kWhite + "; border: none; background: transparent;");

    QLabel* player2ScoreTitle = new QLabel("POINTS MARQUES", m_player2ScoreBox);
    player2ScoreTitle->setAlignment(Qt::AlignHCenter);
    player2ScoreTitle->setStyleSheet("color: " + kOrange + "; font-size: 11px; letter-spacing: 1px; border: none; background: transparent;");

    player2ScoreLayout->addWidget(m_player2ScoreBoxValue);
    player2ScoreLayout->addWidget(player2ScoreTitle);

    QWidget* scoreRow = new QWidget(central);
    QHBoxLayout* scoreRowLayout = new QHBoxLayout(scoreRow);
    scoreRowLayout->setContentsMargins(0, 0, 0, 0);
    scoreRowLayout->setSpacing(14);
    scoreRowLayout->addWidget(m_player1ScoreBox, 1);
    scoreRowLayout->addWidget(breakPanel, 0);
    scoreRowLayout->addWidget(m_player2ScoreBox, 1);

    // ---------------------------------------------------
    // Rangee "detail des points" : un cadre par joueur, place entre le
    // cadre "points marques" et la succession des billes (au lieu d'etre
    // imbrique dans les cadres de score, pour combler l'espace vide).
    // ---------------------------------------------------
    QFrame* player1DetailBox = buildDetailBox(
        central, kGreen, m_player1DetailPotted, m_player1DetailFouls
    );
    QFrame* player2DetailBox = buildDetailBox(
        central, kOrange, m_player2DetailPotted, m_player2DetailFouls
    );

    // Espace invisible de la meme largeur que breakPanel, pour que les
    // deux cadres restent alignes sous m_player1ScoreBox / m_player2ScoreBox.
    QWidget* detailRowSpacer = new QWidget(central);
    detailRowSpacer->setFixedWidth(280);

    QWidget* detailRow = new QWidget(central);
    QHBoxLayout* detailRowLayout = new QHBoxLayout(detailRow);
    detailRowLayout->setContentsMargins(0, 0, 0, 0);
    detailRowLayout->setSpacing(14);
    detailRowLayout->addWidget(player1DetailBox, 1);
    detailRowLayout->addWidget(detailRowSpacer, 0);
    detailRowLayout->addWidget(player2DetailBox, 1);

    // ---------------------------------------------------
    // Panneau "succession des billes empochees" : une rangee
    // de pastilles colorees dans l'ordre reel des coups joues
    // pendant la frame en cours.
    // ---------------------------------------------------
    QWidget* successionPanel = new QWidget(central);
    QVBoxLayout* successionOuterLayout = new QVBoxLayout(successionPanel);
    successionOuterLayout->setContentsMargins(0, 0, 0, 0);
    successionOuterLayout->setSpacing(6);

    // Titre au-dessus des DEUX boites (succession + points restants), pas
    // seulement de la premiere : garde les deux boites a la meme hauteur
    // (150px chacune) sans bricolage d'alignement. Espacement resserre vers
    // sa boite via successionGroupLayout plus bas (pas le 14px habituel du
    // layout principal) ; police doublee (11px -> 22px) pour plus de poids visuel.
    QLabel* successionTitle = new QLabel("SUCCESSION DES BILLES EMPOCHEES", central);
    successionTitle->setAlignment(Qt::AlignHCenter);
    successionTitle->setStyleSheet("color: " + kGreen + "; font-size: 22px; letter-spacing: 1px;");

    QScrollArea* successionScroll = new QScrollArea(successionPanel);
    successionScroll->setWidgetResizable(true);
    successionScroll->setFixedHeight(150);
    successionScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    successionScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    successionScroll->setStyleSheet(
        "QScrollArea { background-color: " + kPanel + "; border: 1px solid " + kGreen + "; border-radius: 5px; }"
    );

    m_successionRow = new QWidget(successionScroll);
    QGridLayout* successionGridLayout = new QGridLayout(m_successionRow);
    successionGridLayout->setContentsMargins(10, 8, 10, 8);
    successionGridLayout->setSpacing(8);
    successionGridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    successionScroll->setWidget(m_successionRow);

    successionOuterLayout->addWidget(successionScroll);

    // ---------------------------------------------------
    // Cadre "Points restants sur la table", a cote de la
    // succession des billes, comme sur la maquette.
    // ---------------------------------------------------
    QFrame* pointsRemainingBox = new QFrame(central);
    pointsRemainingBox->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kGreen + "; border-radius: 5px;"
    );
    pointsRemainingBox->setFixedWidth(180);
    pointsRemainingBox->setFixedHeight(150);
    QVBoxLayout* pointsRemainingLayout = new QVBoxLayout(pointsRemainingBox);
    pointsRemainingLayout->setContentsMargins(12, 4, 12, 4);
    pointsRemainingLayout->setSpacing(4);
    pointsRemainingLayout->setAlignment(Qt::AlignCenter);

    QLabel* pointsRemainingTitle = new QLabel("POINTS\nRESTANTS", pointsRemainingBox);
    pointsRemainingTitle->setAlignment(Qt::AlignHCenter);
    pointsRemainingTitle->setStyleSheet("color: " + kGreen + "; font-size: 11px; letter-spacing: 1px; border: none; background: transparent;");

    m_pointsRemainingLabel = new QLabel(pointsRemainingBox);
    m_pointsRemainingLabel->setAlignment(Qt::AlignHCenter);
    QFont pointsRemainingFont;
    pointsRemainingFont.setPointSize(48);
    pointsRemainingFont.setBold(true);
    m_pointsRemainingLabel->setFont(pointsRemainingFont);
    m_pointsRemainingLabel->setStyleSheet("color: " + kWhite + "; border: none; background: transparent;");

    pointsRemainingLayout->addWidget(pointsRemainingTitle);
    pointsRemainingLayout->addWidget(m_pointsRemainingLabel);

    QWidget* successionRow2 = new QWidget(central);
    QHBoxLayout* successionRow2Layout = new QHBoxLayout(successionRow2);
    successionRow2Layout->setContentsMargins(0, 0, 0, 0);
    successionRow2Layout->setSpacing(14);
    successionRow2Layout->addWidget(successionPanel, 1);
    successionRow2Layout->addWidget(pointsRemainingBox, 0);

    // Groupe titre + rangee de boites avec un espacement resserre (6px, au
    // lieu des 14px du layout principal), pour que le titre soit "colle" a
    // ses boites plutot qu'a equidistance entre les cadres voisins.
    QWidget* successionGroup = new QWidget(central);
    QVBoxLayout* successionGroupLayout = new QVBoxLayout(successionGroup);
    successionGroupLayout->setContentsMargins(0, 0, 0, 0);
    successionGroupLayout->setSpacing(6);
    successionGroupLayout->addWidget(successionTitle, 0, Qt::AlignHCenter);
    successionGroupLayout->addWidget(successionRow2);

    m_moveLogWidget = new MoveLogWidget(central);
    m_moveLogWidget->setVisible(false);

    // ---------------------------------------------------
    // Pied de page : statut cameras a gauche, duree du match au
    // centre, statut d'enregistrement a droite, comme sur la maquette.
    // Le statut cameras est statique pour l'instant : la connexion
    // reelle aux cameras (module MesureVision) n'est pas encore
    // branchee a cette UI de telecommande/score.
    // ---------------------------------------------------
    QFrame* footerPanel = new QFrame(central);
    footerPanel->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 5px;"
    );
    QHBoxLayout* footerLayout = new QHBoxLayout(footerPanel);
    footerLayout->setContentsMargins(18, 10, 18, 10);

    QWidget* camerasSection = new QWidget(footerPanel);
    QHBoxLayout* camerasLayout = new QHBoxLayout(camerasSection);
    camerasLayout->setContentsMargins(0, 0, 0, 0);
    camerasLayout->setSpacing(8);
    QLabel* camerasTitle = new QLabel("CAMERAS :", camerasSection);
    camerasTitle->setStyleSheet("color: " + kGray + "; font-size: 12px; letter-spacing: 1px;");
    camerasLayout->addWidget(camerasTitle);
    for (int i = 0; i < 3; ++i)
    {
        QLabel* cameraDot = new QLabel(camerasSection);
        cameraDot->setFixedSize(10, 10);
        cameraDot->setStyleSheet("background-color: " + kStatusGreen + "; border-radius: 5px;");
        camerasLayout->addWidget(cameraDot);
    }

    QWidget* durationSection = new QWidget(footerPanel);
    QHBoxLayout* durationLayout = new QHBoxLayout(durationSection);
    durationLayout->setContentsMargins(0, 0, 0, 0);
    durationLayout->setSpacing(8);
    QLabel* durationTitle = new QLabel("DUREE :", durationSection);
    durationTitle->setStyleSheet("color: " + kGray + "; font-size: 12px; letter-spacing: 1px;");
    m_durationLabel = new QLabel("00:00:00", durationSection);
    m_durationLabel->setStyleSheet("color: " + kWhite + "; font-size: 12px; font-weight: bold;");
    durationLayout->addWidget(durationTitle);
    durationLayout->addWidget(m_durationLabel);

    QWidget* recordingSection = new QWidget(footerPanel);
    QHBoxLayout* recordingLayout = new QHBoxLayout(recordingSection);
    recordingLayout->setContentsMargins(0, 0, 0, 0);
    recordingLayout->setSpacing(8);
    QLabel* recordingTitle = new QLabel("ENREGISTREMENT :", recordingSection);
    recordingTitle->setStyleSheet("color: " + kGray + "; font-size: 12px; letter-spacing: 1px;");
    m_recordingDot = new QLabel(recordingSection);
    m_recordingDot->setFixedSize(10, 10);
    m_recordingStatusLabel = new QLabel(recordingSection);
    m_recordingStatusLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    recordingLayout->addWidget(recordingTitle);
    recordingLayout->addWidget(m_recordingDot);
    recordingLayout->addWidget(m_recordingStatusLabel);

    // Bouton toujours visible (hors du panneau de controles) pour
    // afficher/masquer la telecommande : sans lui, une fois le panneau
    // cache, il n'y aurait plus aucun moyen de le rafficher.
    m_toggleRemoteButton = new QPushButton("Masquer telecommande", footerPanel);
    m_toggleRemoteButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kGray + ";"
        "border: 1px solid " + kBorder + "; border-radius: 4px; padding: 4px 10px; font-size: 11px; }"
        "QPushButton:hover { color: " + kWhite + "; }"
    );

    footerLayout->addWidget(camerasSection);
    footerLayout->addStretch();
    footerLayout->addWidget(durationSection);
    footerLayout->addStretch();
    footerLayout->addWidget(recordingSection);
    footerLayout->addStretch();
    footerLayout->addWidget(m_toggleRemoteButton);

    QWidget* topRow = new QWidget(central);
    QHBoxLayout* topRowLayout = new QHBoxLayout(topRow);
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(14);
    topRowLayout->addWidget(m_player1Frame, 1);
    topRowLayout->addWidget(framesPanel, 0);
    topRowLayout->addWidget(m_player2Frame, 1);

    // Bandeau "test en cours" (voir m_replayInProgressLabel) : tout en
    // haut, au-dessus meme des noms/scores, pour etre impossible a
    // manquer. Cache par defaut, affiche uniquement pendant le rejeu
    // automatique d'un scenario (voir le bouton "Rejouer le scenario").
    m_replayInProgressLabel = new QLabel("TEST EN COURS — NE PAS FERMER NI CLIQUER", central);
    m_replayInProgressLabel->setAlignment(Qt::AlignHCenter);
    m_replayInProgressLabel->setStyleSheet(
        "background-color: #dc2626; color: " + kWhite + ";"
        "font-size: 13px; font-weight: bold; letter-spacing: 1px;"
        "border-radius: 5px; padding: 8px;"
    );
    m_replayInProgressLabel->setVisible(false);

    layout->addWidget(m_replayInProgressLabel);
    layout->addWidget(topRow);
    layout->addWidget(scoreRow);
    layout->addWidget(detailRow);
    layout->addWidget(successionGroup);
    layout->addWidget(m_moveLogWidget, /*stretch=*/1);
    layout->addWidget(footerPanel);

    // ---------------------------------------------------
    // Telecommande : boutons de billes (toujours actifs, une bille
    // hors sequence est automatiquement traitee comme une faute par
    // Frame::playShot) + bouton "Fin de break" (coup rate volontaire,
    // Frame::missShot), pour simuler des coups a la main et observer
    // le comportement du moteur de jeu en direct.
    // ---------------------------------------------------
    QFrame* remotePanel = new QFrame(central);
    remotePanel->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 5px;"
    );
    remotePanel->setFixedWidth(220);
    QVBoxLayout* remotePanelOuterLayout = new QVBoxLayout(remotePanel);
    remotePanelOuterLayout->setContentsMargins(0, 0, 0, 0);
    remotePanelOuterLayout->setSpacing(0);

    // La telecommande contient beaucoup plus de boutons que la hauteur de
    // l'ecran ne peut en afficher d'un coup. Sans defilement, Qt "starve"
    // les sous-dispositions imbriquees (les paires de billes) d'espace
    // vertical et leur donne une hauteur NEGATIVE, ce qui les fait toutes
    // s'empiler au meme endroit au lieu de simplement deborder en bas.
    QScrollArea* remoteScrollArea = new QScrollArea(remotePanel);
    remoteScrollArea->setWidgetResizable(true);
    remoteScrollArea->setFrameShape(QFrame::NoFrame);
    remoteScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    remoteScrollArea->setStyleSheet("background: transparent; border: none;");
    remotePanelOuterLayout->addWidget(remoteScrollArea);

    // Section fixe, SOUS la zone qui defile : ne bouge jamais, meme si on
    // fait defiler le reste de la telecommande, et reste au niveau du
    // bas de l'ecran (pres de "Masquer telecommande" dans le pied de
    // page) -- demande par l'utilisateur (2026-09-16) pour les 4 outils
    // de test scenario (voir plus bas : m_toggleLogButton, exportLogButton,
    // m_scenarioFileCombo, replayScenarioButton), pour qu'ils restent
    // toujours visibles sans avoir a faire defiler la liste.
    QWidget* remoteFixedBottom = new QWidget(remotePanel);
    QVBoxLayout* remoteFixedBottomLayout = new QVBoxLayout(remoteFixedBottom);
    remoteFixedBottomLayout->setContentsMargins(14, 8, 14, 14);
    remoteFixedBottomLayout->setSpacing(10);
    remotePanelOuterLayout->addWidget(remoteFixedBottom);

    QWidget* remoteContent = new QWidget(remoteScrollArea);
    remoteContent->setStyleSheet("background: transparent;");
    QVBoxLayout* remoteLayout = new QVBoxLayout(remoteContent);
    remoteLayout->setContentsMargins(14, 14, 14, 14);
    remoteLayout->setSpacing(10);
    remoteScrollArea->setWidget(remoteContent);

    // Alerte precoce : une bille encore sur la table (pas empochee) est
    // detectee dangereusement proche du bord (voir VisionGameBridge::
    // ballsNearEdge()), donc a risque de sortie imminente. Purement
    // informatif, mis a jour a chaque image du suivi camera.
    m_edgeWarningLabel = new QLabel(remotePanel);
    m_edgeWarningLabel->setAlignment(Qt::AlignHCenter);
    m_edgeWarningLabel->setWordWrap(true);
    m_edgeWarningLabel->setStyleSheet(
        "color: " + kWhite + "; background-color: #e74c3c;"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_edgeWarningLabel->setVisible(false);
    remoteLayout->addWidget(m_edgeWarningLabel);

    m_freeBallStatusLabel = new QLabel(remotePanel);
    m_freeBallStatusLabel->setAlignment(Qt::AlignHCenter);
    m_freeBallStatusLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kOrange + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_freeBallStatusLabel->setVisible(false);
    remoteLayout->addWidget(m_freeBallStatusLabel);

    m_blackReplayStatusLabel = new QLabel(remotePanel);
    m_blackReplayStatusLabel->setAlignment(Qt::AlignHCenter);
    m_blackReplayStatusLabel->setWordWrap(true);
    m_blackReplayStatusLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kOrange + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_blackReplayStatusLabel->setVisible(false);
    remoteLayout->addWidget(m_blackReplayStatusLabel);

    m_touchingBallStatusLabel = new QLabel(remotePanel);
    m_touchingBallStatusLabel->setAlignment(Qt::AlignHCenter);
    m_touchingBallStatusLabel->setWordWrap(true);
    m_touchingBallStatusLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kOrange + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_touchingBallStatusLabel->setVisible(false);
    remoteLayout->addWidget(m_touchingBallStatusLabel);

    // Bandeau d'instruction : affiche quelle action est en attente d'une
    // bille cliquee sur la telecommande (voir enum PendingAction), avec un
    // bouton pour annuler si l'utilisateur change d'avis.
    m_pendingActionLabel = new QLabel(remotePanel);
    m_pendingActionLabel->setAlignment(Qt::AlignHCenter);
    m_pendingActionLabel->setWordWrap(true);
    m_pendingActionLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kWhite + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_pendingActionLabel->setVisible(false);
    remoteLayout->addWidget(m_pendingActionLabel);

    m_cancelPendingButton = new QPushButton("Annuler", remotePanel);
    m_cancelPendingButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + ";"
        "  color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + ";"
        "  border-radius: 5px;"
        "  padding: 4px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kGray + "; }"
    );
    m_cancelPendingButton->setVisible(false);
    connect(m_cancelPendingButton, &QPushButton::clicked, this, [this]()
        {
            m_pendingAction = PendingAction::None;
            refreshDisplay();
        });
    remoteLayout->addWidget(m_cancelPendingButton);

    // Choix suivant un Miss (voir PendingAction::MissChoice) : la faute est
    // deja appliquee et la main deja passee, ce panneau ne fait que
    // demander confirmation du parti pris par l'adversaire.
    m_missChoicePanel = new QWidget(remotePanel);
    {
        QString missChoiceButtonStyle =
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  padding: 8px 10px;"
            "}"
            "QPushButton:hover { border-color: " + kGreen + "; }";
        QHBoxLayout* missChoiceLayout = new QHBoxLayout(m_missChoicePanel);
        missChoiceLayout->setContentsMargins(0, 0, 0, 0);
        QPushButton* replayButton = new QPushButton("Remettre en place", m_missChoicePanel);
        replayButton->setStyleSheet(missChoiceButtonStyle);
        connect(replayButton, &QPushButton::clicked, this, [this]()
            {
                m_gameManager.getMatch().getCurrentFrame().requestReplay();
                m_pendingAction = PendingAction::None;
                refreshDisplay();
                showRepositioningGuide(/*silentIfUnavailable=*/true);
            });
        QPushButton* continueButton = new QPushButton("Prendre la table", m_missChoicePanel);
        continueButton->setStyleSheet(missChoiceButtonStyle);
        connect(continueButton, &QPushButton::clicked, this, [this]()
            {
                m_pendingAction = PendingAction::None;
                refreshDisplay();
            });
        missChoiceLayout->addWidget(replayButton);
        missChoiceLayout->addWidget(continueButton);
    }
    m_missChoicePanel->setVisible(false);
    remoteLayout->addWidget(m_missChoicePanel);

    const int ballButtonWidth = 88;
    const int ballButtonHeight = 56;
    const int ballButtonSpacing = 8;

    // Cree un bouton de bille stylise (couleur pleine, texte "Nom\n(valeur)").
    // Extrait en lambda pour eviter de dupliquer ce bloc 7 fois : la
    // disposition (rangee pleine largeur pour la rouge, paires pour les
    // couleurs) est geree par l'appelant via des QHBoxLayout, pas par un
    // QGridLayout -- un QGridLayout avec span de colonnes melange a des
    // cellules simples ecrasait les lignes les unes sur les autres.
    auto makeBallButton = [&](const QString& ballName, int width) -> QPushButton*
    {
        int ballValue = standardBallValue(ballName);

        QColor base(ballColorHex(ballName));
        QColor textColor = (base.lightness() > 150) ? QColor(kBg) : QColor(kWhite);

        QPushButton* ballButton = new QPushButton(
            ballName + "\n(" + QString::number(ballValue) + ")", remotePanel
        );
        ballButton->setFixedSize(width, ballButtonHeight);
        ballButton->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  color: %2;"
            "  border: 1px solid %3;"
            "  border-radius: 8px;"
            "  font-weight: bold;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover { border: 2px solid " + kWhite + "; }"
            "QPushButton:pressed { background-color: %4; }"
            "QPushButton:disabled { background-color: #2a2d31; color: #6a6d71; border-color: #2a2d31; }"
        ).arg(base.name(), textColor.name(), base.darker(150).name(), base.darker(130).name()));

        connect(ballButton, &QPushButton::clicked, this, [this, ballName, ballValue]()
            {
                handleBallAction(ballName, ballValue);
            });

        m_ballButtons[ballName] = ballButton;
        return ballButton;
    };

    QVBoxLayout* ballButtonsLayout = new QVBoxLayout();
    ballButtonsLayout->setSpacing(ballButtonSpacing);

    // La rouge occupe seule toute la largeur de la telecommande (une seule
    // bille rouge peut etre jouee a la fois, contrairement aux billes de
    // couleur, d'ou sa mise en avant visuelle).
    ballButtonsLayout->addWidget(makeBallButton("Rouge", ballButtonWidth * 2 + ballButtonSpacing));

    const QList<QPair<QString, QString>> colorPairs = {
        { "Jaune", "Verte" },
        { "Marron", "Bleue" },
        { "Rose", "Noire" }
    };
    for (const auto& pair : colorPairs)
    {
        QHBoxLayout* pairRow = new QHBoxLayout();
        pairRow->setSpacing(ballButtonSpacing);
        pairRow->addWidget(makeBallButton(pair.first, ballButtonWidth));
        pairRow->addWidget(makeBallButton(pair.second, ballButtonWidth));
        ballButtonsLayout->addLayout(pairRow);
    }

    remoteLayout->addLayout(ballButtonsLayout);
    remoteLayout->addSpacing(6);

    // ---------------------------------------------------
    // Grille d'actions : meme disposition (2 colonnes) et meme ordre que
    // la page telephone (voir kPageHtml, .actiongrid) -- Faute / Fin de
    // break / Free ball / Miss / Retour / Fin de frame / Esc / Nouveau
    // match, puis Guide de repositionnement et Fermer le guide en pleine
    // largeur. Les boutons sont crees plus bas (avec le reste de la
    // telecommande 1.0) mais places ICI via actionsGrid->addWidget() --
    // le reste des boutons (avances/tests) continue d'aller directement
    // dans remoteLayout et s'affiche donc EN DESSOUS de cette grille.
    // ---------------------------------------------------
    QGridLayout* actionsGrid = new QGridLayout();
    actionsGrid->setSpacing(8);
    remoteLayout->addLayout(actionsGrid);
    remoteLayout->addSpacing(6);

    QPushButton* missShotButton = new QPushButton("Fin de break", remotePanel);
    missShotButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + ";"
        "  color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + ";"
        "  border-radius: 5px;"
        "  padding: 8px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(missShotButton, &QPushButton::clicked, this, [this]()
        {
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            frame.missShot();
            m_gameManager.afterShot();
            refreshDisplay();
        });
    actionsGrid->addWidget(missShotButton, 0, 0);

    // ---------------------------------------------------
    // Game : force la fin de la frame en cours (le vainqueur est
    // determine avec le score actuel), pour sauter directement a la
    // frame suivante sans jouer toutes les billes restantes. Meme
    // libelle que sur le telephone ("Game"), pour rester coherent.
    // ---------------------------------------------------
    QPushButton* finishFrameButton = new QPushButton("Game", remotePanel);
    finishFrameButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + ";"
        "  color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + ";"
        "  border-radius: 5px;"
        "  padding: 8px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(finishFrameButton, &QPushButton::clicked, this, [this]()
        {
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            if (!frame.forceFinishFrame())
            {
                showStyledMessage(this, QMessageBox::Information, "Fin de frame",
                    "Impossible : les scores sont a egalite. Il faut d'abord departager "
                    "l'egalite (billes suivantes) avant de pouvoir terminer la frame.");
                return;
            }
            m_gameManager.afterShot();
            refreshDisplay();
        });
    actionsGrid->addWidget(finishFrameButton, 1, 1);

    // ---------------------------------------------------
    // Faute : arme une action en attente. Le prochain bouton de bille
    // clique dans la telecommande fournit la bille fautee ; la penalite
    // (regle du snooker : minimum 4 points, 5 pour Bleue, 6 pour Rose,
    // 7 pour Noire) est alors calculee et appliquee automatiquement.
    // ---------------------------------------------------
    QString secondaryButtonStyle =
        "QPushButton {"
        "  background-color: " + kPanel + ";"
        "  color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + ";"
        "  border-radius: 5px;"
        "  padding: 8px 10px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }";

    // ---------------------------------------------------
    // Nouveau match : redemande les noms des joueurs et repart d'un
    // match tout neuf (scores et frames a zero), sans relancer l'appli.
    // ---------------------------------------------------
    QPushButton* newMatchButton = new QPushButton("Nouveau match", remotePanel);
    // Libelle borderline pour la moitie de la grille (110px) : police
    // reduite, meme technique que clearHistoryButton/freeBallButton plus
    // bas, sinon le texte se retrouve rogne ("Nouveau matc").
    newMatchButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 2px; font-size: 12px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(newMatchButton, &QPushButton::clicked, this, [this]()
        {
            QString player1Name;
            QString player2Name;
            int framesToWin = 2;
            promptPlayerNames(player1Name, player2Name, framesToWin);
            restartMatch(player1Name, player2Name, framesToWin);
        });
    actionsGrid->addWidget(newMatchButton, 4, 0);

    QPushButton* foulButton = new QPushButton("Faute", remotePanel);
    foulButton->setStyleSheet(secondaryButtonStyle);
    connect(foulButton, &QPushButton::clicked, this, [this]()
        {
            m_pendingAction = PendingAction::Foul;
            refreshDisplay();
        });
    actionsGrid->addWidget(foulButton, 0, 1);

    // "Bille sortie de table", "Blanche sortie de table", "Recommencer la
    // frame", "Conceder la frame" et "Correction arbitre" regroupees avec
    // "Bille touchante" sous un seul bouton menu "Autre" (voir plus bas,
    // meme emplacement que "Bille touchante" avant ce regroupement,
    // demande par l'utilisateur le 2026-09-16 pour alleger la liste) --
    // le contenu de chaque action est inchange, seul l'acces change.

    // ---------------------------------------------------
    // Free ball : menu a 2 choix pour le joueur qui vient de recevoir la
    // main apres une faute adverse (Miss ou Faute) et se retrouve snooke :
    //  - Remettre en place : plutot que de jouer la position, il prefere
    //    faire rejouer le fautif -- action immediate (pas d'attente de
    //    bille), repasse la main et ouvre le guide de repositionnement
    //    (implique de replacer physiquement les billes).
    //  - Choisir la bille de depart : il joue lui-meme mais est snooke,
    //    donc nomme une bille de remplacement (voir Frame::playFreeBall).
    // ---------------------------------------------------
    QPushButton* freeBallButton = new QPushButton("Free ball", remotePanel);
    freeBallButton->setStyleSheet(secondaryButtonStyle);

    QMenu* freeBallMenu = new QMenu(freeBallButton);
    freeBallMenu->setStyleSheet(
        "QMenu { background-color: " + kPanel + "; color: " + kWhite + "; border: 1px solid " + kBorder + "; }"
        "QMenu::item:selected { background-color: " + kBorder + "; }"
    );

    QAction* missReplayAction = freeBallMenu->addAction("Remettre en place");
    QAction* chooseStartingBallAction = freeBallMenu->addAction("Choisir la bille de depart");

    connect(missReplayAction, &QAction::triggered, this, [this]()
        {
            m_gameManager.getMatch().getCurrentFrame().requestReplay();
            refreshDisplay();
            showRepositioningGuide(/*silentIfUnavailable=*/true);
        });

    connect(chooseStartingBallAction, &QAction::triggered, this, [this]()
        {
            m_pendingAction = PendingAction::ArmFreeBall;
            refreshDisplay();
        });

    freeBallButton->setMenu(freeBallMenu);
    actionsGrid->addWidget(freeBallButton, 2, 1);

    // ---------------------------------------------------
    // Miss : le referee juge que le joueur n'a pas veritablement tente
    // la bille demandee. Arme une action en attente, exactement comme
    // "Faute" : le prochain bouton de bille clique fournit la bille
    // fautee et la penalite est calculee automatiquement -- cela passe
    // la main a l'adversaire, qui joue alors directement. S'il decouvre
    // qu'il est snooke, il utilise le bouton "Free ball" ci-dessus.
    // ---------------------------------------------------
    QPushButton* missButton = new QPushButton("Miss", remotePanel);
    missButton->setStyleSheet(secondaryButtonStyle);
    connect(missButton, &QPushButton::clicked, this, [this]()
        {
            m_pendingAction = PendingAction::Miss;
            refreshDisplay();
        });
    actionsGrid->addWidget(missButton, 2, 0);

    // ---------------------------------------------------
    // Retour : annule le dernier coup (bille empochee, faute ou "Fin de
    // break"), voir snapshotFrameForUndo(). Un seul niveau d'annulation.
    // Meme mecanique que sur les telecommandes 2.0 et telephone.
    // ---------------------------------------------------
    QPushButton* undoButton = new QPushButton("Retour", remotePanel);
    undoButton->setStyleSheet(secondaryButtonStyle);
    connect(undoButton, &QPushButton::clicked, this, [this]()
        {
            if (!m_hasUndoSnapshot)
            {
                return;
            }
            m_gameManager.getMatch().getCurrentFrame() = m_undoSnapshot;
            m_gameManager.getMatch().undoFrameConclusion();
            m_hasUndoSnapshot = false;
            m_pendingAction = PendingAction::None;
            refreshDisplay();
        });
    actionsGrid->addWidget(undoButton, 1, 0);

    // ---------------------------------------------------
    // Esc : quitte l'ecran de match et revient a l'accueil (voir
    // m_rootStack, index 1). Meme mecanique que sur les telecommandes
    // 2.0 et telephone.
    // ---------------------------------------------------
    QPushButton* exitButton = new QPushButton("Esc", remotePanel);
    exitButton->setStyleSheet(secondaryButtonStyle);
    connect(exitButton, &QPushButton::clicked, this, [this]()
        {
            m_rootStack->setCurrentIndex(1);
        });
    actionsGrid->addWidget(exitButton, 4, 1);

    // ---------------------------------------------------
    // "Autre" : regroupe 6 actions d'arbitrage peu frequentes (nom
    // provisoire, demande par l'utilisateur le 2026-09-16) sous un seul
    // bouton-menu plutot que 6 boutons separes -- meme technique que
    // "Free ball" ci-dessus (QMenu attache). Chaque action garde
    // exactement le meme comportement qu'avant ce regroupement.
    // ---------------------------------------------------
    QPushButton* otherActionsButton = new QPushButton("Autre", remotePanel);
    otherActionsButton->setStyleSheet(secondaryButtonStyle);

    QMenu* otherActionsMenu = new QMenu(otherActionsButton);
    otherActionsMenu->setStyleSheet(
        "QMenu { background-color: " + kPanel + "; color: " + kWhite + "; border: 1px solid " + kBorder + "; }"
        "QMenu::item:selected { background-color: " + kBorder + "; }"
    );

    // Bille touchante : l'arbitre l'annonce quand la blanche est deja au
    // repos en contact avec une bille jouable, AVANT que le coup suivant
    // ne soit joue (voir Frame::setTouchingBall()). Action immediate (pas
    // de bille a choisir ensuite) : elle arme juste l'etat pour le
    // prochain coup.
    QAction* touchingBallAction = otherActionsMenu->addAction("Bille touchante");
    connect(touchingBallAction, &QAction::triggered, this, [this]()
        {
            m_gameManager.getMatch().getCurrentFrame().setTouchingBall(true);
            refreshDisplay();
        });

    // Bille sortie de table : meme mecanique que "Faute" (meme calcul de
    // penalite), mais motif distinct enregistre dans le journal des coups.
    QAction* ballOffTableAction = otherActionsMenu->addAction("Bille sortie de table");
    connect(ballOffTableAction, &QAction::triggered, this, [this]()
        {
            m_pendingAction = PendingAction::BallOffTable;
            refreshDisplay();
        });

    // Blanche sortie de table : meme mecanique que "Bille sortie de table"
    // ci-dessus, mais pour la bille de choc elle-meme -- non couverte par
    // cette action (qui ne propose que les 7 billes objet), ni par
    // "Faute" (meme limitation). Action immediate (la bille concernee est
    // deja connue), voir triggerBlancheOffTableFoul().
    QAction* whiteOffTableAction = otherActionsMenu->addAction("Blanche sortie de table");
    connect(whiteOffTableAction, &QAction::triggered, this, [this]()
        {
            triggerBlancheOffTableFoul();
        });

    // Recommencer la frame : regle du "Pat" (Sect. 3 §17 du reglement) --
    // sur jugement de l'arbitre (blocage ou risque de blocage persistant),
    // annule tous les points de la frame en cours et remet les billes en
    // position de depart, SANS toucher au score du match (frames gagnees).
    // Match::startNewFrame() fait deja exactement ca : elle determine qui
    // ouvre a partir des frames deja TERMINEES (pas de la frame abandonnee),
    // donc le meme joueur rouvre automatiquement, comme l'exige la regle.
    // Confirmation demandee (action destructive pour la frame en cours).
    QAction* restartFrameAction = otherActionsMenu->addAction("Recommencer la frame");
    connect(restartFrameAction, &QAction::triggered, this, [this]()
        {
            QMessageBox box(QMessageBox::Warning, "Recommencer la frame",
                "Annuler tous les points de cette frame et remettre les billes en place\n"
                "(regle du Pat, meme joueur rouvre) ?",
                QMessageBox::Yes | QMessageBox::No, this);
            box.setStyleSheet(
                "QMessageBox { background-color: " + kBg + "; }"
                "QLabel { color: " + kWhite + "; background: transparent; }"
                "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
                "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
            );
            if (box.exec() == QMessageBox::Yes)
            {
                m_gameManager.getMatch().startNewFrame();
                refreshDisplay();
            }
        });

    // Conceder la frame : un joueur abandonne la frame en cours sur
    // decision de l'arbitre -- l'ADVERSAIRE gagne immediatement, quel
    // que soit le score actuel (voir Frame::concedeFrame(), different de
    // "Game" qui se contente de regarder qui est devant). Le choix du
    // joueur qui concede se fait via les boutons nommes de la boite de
    // dialogue plutot qu'en devinant a partir du joueur au tir.
    QAction* concedeFrameAction = otherActionsMenu->addAction("Conceder la frame");
    connect(concedeFrameAction, &QAction::triggered, this, [this]()
        {
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            QString name1 = QString::fromStdString(frame.getPlayer1().getName());
            QString name2 = QString::fromStdString(frame.getPlayer2().getName());

            QMessageBox box(QMessageBox::Warning, "Conceder la frame",
                "Quel joueur concede la frame en cours ?\n"
                "L'adversaire gagne la frame immediatement, quel que soit le score actuel.",
                QMessageBox::NoButton, this);
            QPushButton* p1Button = box.addButton(name1 + " concede", QMessageBox::AcceptRole);
            QPushButton* p2Button = box.addButton(name2 + " concede", QMessageBox::AcceptRole);
            box.addButton("Annuler", QMessageBox::RejectRole);
            box.setStyleSheet(
                "QMessageBox { background-color: " + kBg + "; }"
                "QLabel { color: " + kWhite + "; background: transparent; }"
                "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
                "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
            );
            box.exec();

            Player* conceder = nullptr;
            if (box.clickedButton() == p1Button)
            {
                conceder = &frame.getPlayer1();
            }
            else if (box.clickedButton() == p2Button)
            {
                conceder = &frame.getPlayer2();
            }
            if (conceder == nullptr)
            {
                return;
            }

            if (!frame.concedeFrame(*conceder))
            {
                return;
            }
            m_gameManager.afterShot();
            refreshDisplay();
        });

    // Correction arbitre : annule le dernier coup enregistre (meme
    // mecanisme que "Retour") puis attend que l'arbitre clique la bille
    // REELLEMENT concernee, pour la rejouer et garder une trace explicite
    // "avant -> apres" dans le journal (voir Frame::logCorrection()) au
    // lieu de faire disparaitre l'erreur silencieusement comme le ferait
    // un simple "Retour". Utile typiquement quand la detection automatique
    // par camera s'est trompee de bille. Meme limite que "Retour" : un
    // seul niveau d'annulation, donc uniquement le TOUT dernier coup.
    QAction* correctionAction = otherActionsMenu->addAction("Correction arbitre");
    connect(correctionAction, &QAction::triggered, this, [this]()
        {
            if (!m_hasUndoSnapshot)
            {
                showStyledMessage(this, QMessageBox::Information, "Correction arbitre",
                    "Aucun coup a corriger (un seul niveau d'annulation disponible, "
                    "meme limite que \"Retour\").");
                return;
            }

            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            const std::vector<LogEntry>& log = frame.getHistory().getLog();
            if (log.empty()
                || (log.back().type != LogEntry::Type::Shot && log.back().type != LogEntry::Type::Foul))
            {
                showStyledMessage(this, QMessageBox::Information, "Correction arbitre",
                    "Le dernier evenement du journal n'est pas un coup ou une faute "
                    "(rien a corriger de cette maniere).");
                return;
            }

            QString before = describeLogEntry(log.back());

            QMessageBox box(QMessageBox::Warning, "Correction arbitre",
                "Dernier coup enregistre : " + before + "\n\n"
                "Annuler ce coup et cliquer ensuite la bille reellement concernee ?",
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

            m_pendingCorrectionBefore = before.toStdString();
            m_gameManager.getMatch().getCurrentFrame() = m_undoSnapshot;
            m_gameManager.getMatch().undoFrameConclusion();
            m_hasUndoSnapshot = false;
            m_pendingAction = PendingAction::CorrectionBall;
            refreshDisplay();
        });

    // Fermer le guide : re-ajoutee ici (2026-09-16) sur demande de
    // l'utilisateur -- le guide de repositionnement lui-meme a deja son
    // propre bouton "Fermer" et repond a Echap (voir
    // showRepositioningGuideDialog()), mais une fermeture depuis la
    // telecommande reste utile si le guide est affiche sur un autre
    // ecran (ex. projete cote table). Sans effet si aucun guide n'est
    // actuellement affiche (voir closeRepositioningGuide()).
    QAction* closeGuideAction = otherActionsMenu->addAction("Fermer le guide");
    connect(closeGuideAction, &QAction::triggered, this, [this]()
        {
            closeRepositioningGuide();
        });

    otherActionsButton->setMenu(otherActionsMenu);
    actionsGrid->addWidget(otherActionsButton, 3, 0, 1, 2);

    // "Reglement" et "Partager en Wi-Fi" retires de cette telecommande
    // (2026-09-16) : doublons exacts de l'accueil ("Regles" et la tuile
    // "Smartphone", tous deux deja connectes a RulesReferenceDialog /
    // ShareSessionDialog via m_homeScreen ci-dessous) -- accessibles
    // avant meme de lancer un match, pas besoin de les repeter ici.
    // m_shareButton reste nullptr ; son seul autre usage (tuile
    // "Smartphone") est deja protege par un test null.

    m_toggleLogButton = new QPushButton("Afficher le journal des coups", remotePanel);
    m_toggleLogButton->setStyleSheet(secondaryButtonStyle);
    remoteFixedBottomLayout->addWidget(m_toggleLogButton);

    // ---------------------------------------------------
    // Enregistrer le scenario : ecrit l'etat courant + le journal complet
    // dans un fichier texte horodate (dossier scenarios_test), pour signaler
    // un scenario/bug precisement des qu'un probleme est repere en testant.
    // ---------------------------------------------------
    QPushButton* exportLogButton = new QPushButton("Enregistrer le scenario", remotePanel);
    exportLogButton->setStyleSheet(secondaryButtonStyle);
    connect(exportLogButton, &QPushButton::clicked, this, [this]()
        {
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            exportMoveLogToFile(this, m_gameManager.getMatch(), frame);
            refreshScenarioFileList(m_scenarioFileCombo);
        });
    remoteFixedBottomLayout->addWidget(exportLogButton);

    // ---------------------------------------------------
    // Choix du scenario a rejouer (le plus recent enregistre est en tete).
    // ---------------------------------------------------
    m_scenarioFileCombo = new QComboBox(remotePanel);
    m_scenarioFileCombo->setStyleSheet(
        "QComboBox {"
        "  background-color: " + kPanel + ";"
        "  color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + ";"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
    );
    refreshScenarioFileList(m_scenarioFileCombo);
    remoteFixedBottomLayout->addWidget(m_scenarioFileCombo);

    // ---------------------------------------------------
    // Rejouer le scenario : relit le scenario_*.txt choisi ci-dessus et
    // reproduit exactement la meme sequence sur une frame fraiche, pour
    // retester un scenario suspect apres une correction du moteur. Le
    // choix de vitesse (menu deroulant a part) a ete retire de la
    // telecommande principale a la demande de l'utilisateur (outil de
    // test, jamais utilise en vraie partie) -- vitesse normale fixe.
    // ---------------------------------------------------
    QPushButton* replayScenarioButton = new QPushButton("Rejouer le scenario", remotePanel);
    replayScenarioButton->setStyleSheet(secondaryButtonStyle);
    connect(replayScenarioButton, &QPushButton::clicked, this, [this]()
        {
            QString path = m_scenarioFileCombo->currentData().toString();
            if (path.isEmpty())
            {
                showStyledMessage(this, QMessageBox::Information, "Rejouer le scenario",
                    "Aucun scenario enregistre pour l'instant (bouton \"Enregistrer le scenario\").");
                return;
            }

            std::vector<ReplayAction> actions = parseScenarioFile(path);
            if (actions.empty())
            {
                showStyledMessage(this, QMessageBox::Warning, "Rejouer le scenario", "Fichier illisible ou vide :\n" + path);
                return;
            }

            // Repart d'une frame fraiche (memes joueurs), puis rejoue coup
            // par coup (comme "Scenario de test") pour suivre le deroule a
            // l'ecran au lieu de sauter directement au resultat final.
            m_gameManager.getMatch().startNewFrame();
            refreshDisplay();

            m_replayTimer->stop();
            m_replayTimer->disconnect();
            m_replayTimer->setInterval(700); // vitesse normale fixe (voir plus haut)

            auto actionsPtr = std::make_shared<std::vector<ReplayAction>>(std::move(actions));
            auto indexPtr = std::make_shared<size_t>(0);

            connect(m_replayTimer, &QTimer::timeout, this, [this, actionsPtr, indexPtr]()
                {
                    if (*indexPtr >= actionsPtr->size())
                    {
                        m_replayTimer->stop();
                        m_replayInProgressLabel->setVisible(false);
                        return;
                    }

                    const ReplayAction& action = (*actionsPtr)[*indexPtr];
                    Frame& frame = m_gameManager.getMatch().getCurrentFrame();
                    if (action.kind == ReplayAction::Kind::Miss)
                    {
                        frame.missShot();
                    }
                    else if (action.kind == ReplayAction::Kind::TouchingBall)
                    {
                        frame.setTouchingBall(true);
                    }
                    else if (action.kind == ReplayAction::Kind::Replay)
                    {
                        frame.requestReplay();
                    }
                    else if (action.kind == ReplayAction::Kind::BlancheOffTable)
                    {
                        Ball required = frame.getRequiredBall();
                        Ball blanche("Blanche", 0);
                        Referee foulReferee;
                        int penalty = foulReferee.calculateFoul(required, blanche);
                        frame.foul(required, blanche, penalty, "Blanche sortie de la table");
                    }
                    else if (action.kind == ReplayAction::Kind::DirectFoul)
                    {
                        // Voir Kind::DirectFoul : la bille jouee EST la
                        // bille demandee (Miss pur), donc appelle
                        // Frame::foul() directement au lieu de playShot()
                        // pour ne pas que ce coup soit relu comme legal.
                        Ball required = frame.getRequiredBall();
                        int value = standardBallValue(action.ballName);
                        Ball touched(action.ballName.toStdString(), value);
                        Referee foulReferee;
                        int penalty = foulReferee.calculateFoul(required, touched);
                        frame.foul(required, touched, penalty, "Absence de veritable tentative (Miss)");
                    }
                    else if (action.kind == ReplayAction::Kind::FreeBallShot)
                    {
                        // Voir Kind::FreeBallShot : arme le Free Ball juste
                        // avant de le jouer, exactement comme le bouton
                        // "Choisir la bille de depart" en direct -- capture
                        // la valeur a compter (frame.setFreeBall) a partir
                        // de l'etat courant, PUIS joue la bille reellement
                        // empochee via playFreeBall() (pas playShot()).
                        // Limite connue : ne gere pas le cas ambigu (voir
                        // Frame::isFreeBallValueAmbiguous(), "n'importe
                        // quelle couleur" due) -- pas necessaire pour les
                        // scenarios ecrits jusqu'ici, a traiter si besoin.
                        Ball designated(action.ballName.toStdString(), standardBallValue(action.ballName));
                        frame.setFreeBall(true);
                        frame.setFreeBallColor(designated);
                        frame.playFreeBall(designated);
                    }
                    else if (action.kind == ReplayAction::Kind::FreeBallFoul)
                    {
                        // Voir Kind::FreeBallFoul : arme le Free Ball avec la
                        // bille DESIGNEE (action.ballName), puis "joue" la
                        // bille reellement touchee (action.touchedBallName,
                        // differente) via Frame::playFreeBall() -- c'est ce
                        // meme appel qui reconnait le mismatch et declenche
                        // la faute en interne (voir Frame::playFreeBall()),
                        // avec le meme calcul de penalite et le meme
                        // desarmement propre du Free Ball qu'en jeu reel.
                        Ball designated(action.ballName.toStdString(), standardBallValue(action.ballName));
                        Ball touched(action.touchedBallName.toStdString(), standardBallValue(action.touchedBallName));
                        frame.setFreeBall(true);
                        frame.setFreeBallColor(designated);
                        frame.playFreeBall(touched);
                    }
                    else
                    {
                        int value = standardBallValue(action.ballName);
                        frame.playShot(Ball(action.ballName.toStdString(), value));
                    }
                    m_gameManager.afterShot();
                    ++(*indexPtr);
                    refreshDisplay();
                });

            m_replayInProgressLabel->setVisible(true);
            m_replayTimer->start();
        });
    remoteFixedBottomLayout->addWidget(replayScenarioButton);

    // "Statistiques" retiree de cette telecommande (2026-09-16) : pas
    // utilisee pendant une vraie partie, voir showStatsDialog() (reste
    // appelable si un autre point d'entree en a besoin plus tard).

    // "Scenario de test" (demo codee en dur, distincte du systeme de
    // fichiers scenario_*.txt ci-dessus) retiree de cette telecommande
    // sur demande de l'utilisateur (2026-09-16) : jamais utilisee, les
    // tests passent par "Enregistrer le scenario"/"Rejouer le scenario".
    // m_scenarioButton reste nullptr, m_scenarioRunner construit mais
    // jamais demarre (plus aucun bouton connecte a son clicked).

    // ---------------------------------------------------
    // Suivi camera en direct et bascule "Son" : retires de cette
    // telecommande sur demande de l'utilisateur (2026-09-16) -- la camera
    // n'a pas encore de vraie calibration utilisable, et le son se regle
    // deja depuis Parametres (voir SettingsDialog). m_visionButton et
    // m_speechToggleButton restent nullptr : toggleVisionTracking() ne
    // peut plus etre appelee (plus aucun bouton connecte a son clicked),
    // et le rappel SettingsDialog::speechEnabledChanged qui met a jour
    // m_speechToggleButton est deja protege par un test null (voir plus
    // bas) -- rien ne deref un pointeur nul.

    // "Effacer l'historique des matchs" retire de cette telecommande sur
    // demande de l'utilisateur (2026-09-16). Pas d'autre acces a cette
    // action pour l'instant (voir PlayersDialog/TournamentDialog pour
    // leurs propres actions d'effacement, distinctes de celle-ci).

    // "Guide de repositionnement" et "Fermer le guide" retires de cette
    // telecommande (2026-09-16) : devenus inutilisables des que "Demarrer
    // suivi camera" a ete retire ci-dessus (showRepositioningGuide()
    // exige m_visionTimer actif, qui ne peut plus jamais demarrer depuis
    // cette telecommande) -- ne rien laisser qui ne fait plus qu'afficher
    // "Demarrez d'abord le suivi camera". A remettre en meme temps que le
    // suivi camera, une fois une vraie calibration disponible.

    remoteLayout->addStretch();

    outerLayout->addWidget(scorePanel, 1);
    outerLayout->addWidget(remotePanel, 0);
    m_remotePanel = remotePanel;

    // ---------------------------------------------------
    // Telecommande "2.0" : version simplifiee (billes + Faute + Fin de
    // break seulement), pour l'usage courant sans scenarios/tests.
    // ETAPE EN COURS : construite ici juste a cote de la 1.0 pour
    // comparaison visuelle -- pas encore branchee a un choix dans les
    // parametres ni a la detection d'un telephone connecte.
    // ---------------------------------------------------
    QFrame* remotePanelSimple = new QFrame(central);
    remotePanelSimple->setStyleSheet(
        "background-color: " + kPanel + "; border: 1px solid " + kBorder + "; border-radius: 5px;"
    );
    remotePanelSimple->setFixedWidth(220);
    QVBoxLayout* simpleLayout = new QVBoxLayout(remotePanelSimple);
    simpleLayout->setContentsMargins(14, 14, 14, 14);
    simpleLayout->setSpacing(10);

    // Bandeau FramePhase::BlackReplay (meme role que m_blackReplayStatusLabel
    // sur la 1.0, absent jusqu'ici sur cette telecommande).
    m_simpleBlackReplayStatusLabel = new QLabel(remotePanelSimple);
    m_simpleBlackReplayStatusLabel->setAlignment(Qt::AlignHCenter);
    m_simpleBlackReplayStatusLabel->setWordWrap(true);
    m_simpleBlackReplayStatusLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kOrange + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_simpleBlackReplayStatusLabel->setVisible(false);
    simpleLayout->addWidget(m_simpleBlackReplayStatusLabel);

    // Bandeau "Bille touchante" (meme role que m_touchingBallStatusLabel
    // sur la 1.0).
    m_simpleTouchingBallStatusLabel = new QLabel(remotePanelSimple);
    m_simpleTouchingBallStatusLabel->setAlignment(Qt::AlignHCenter);
    m_simpleTouchingBallStatusLabel->setWordWrap(true);
    m_simpleTouchingBallStatusLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kOrange + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_simpleTouchingBallStatusLabel->setVisible(false);
    simpleLayout->addWidget(m_simpleTouchingBallStatusLabel);

    // Bandeau d'instruction (meme role que m_pendingActionLabel sur la
    // 1.0, absent jusqu'ici sur cette telecommande) + choix suivant un
    // Miss (voir PendingAction::MissChoice) : la faute est deja appliquee
    // et la main deja passee, ce panneau ne fait que demander confirmation
    // du parti pris par l'adversaire.
    m_simplePendingActionLabel = new QLabel(remotePanelSimple);
    m_simplePendingActionLabel->setAlignment(Qt::AlignHCenter);
    m_simplePendingActionLabel->setWordWrap(true);
    m_simplePendingActionLabel->setStyleSheet(
        "color: " + kBg + "; background-color: " + kWhite + ";"
        "border-radius: 4px; font-size: 11px; font-weight: bold; padding: 4px;"
    );
    m_simplePendingActionLabel->setVisible(false);
    simpleLayout->addWidget(m_simplePendingActionLabel);

    m_simpleMissChoicePanel = new QWidget(remotePanelSimple);
    {
        QString missChoiceButtonStyle =
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 5px;"
            "  padding: 8px 10px;"
            "}"
            "QPushButton:hover { border-color: " + kGreen + "; }";
        QVBoxLayout* simpleMissChoiceLayout = new QVBoxLayout(m_simpleMissChoicePanel);
        simpleMissChoiceLayout->setContentsMargins(0, 0, 0, 0);
        simpleMissChoiceLayout->setSpacing(6);
        QPushButton* replayButton = new QPushButton("Remettre en place", m_simpleMissChoicePanel);
        replayButton->setStyleSheet(missChoiceButtonStyle);
        connect(replayButton, &QPushButton::clicked, this, [this]()
            {
                m_gameManager.getMatch().getCurrentFrame().requestReplay();
                m_pendingAction = PendingAction::None;
                refreshDisplay();
                showRepositioningGuide(/*silentIfUnavailable=*/true);
            });
        QPushButton* continueButton = new QPushButton("Prendre la table", m_simpleMissChoicePanel);
        continueButton->setStyleSheet(missChoiceButtonStyle);
        connect(continueButton, &QPushButton::clicked, this, [this]()
            {
                m_pendingAction = PendingAction::None;
                refreshDisplay();
            });
        simpleMissChoiceLayout->addWidget(replayButton);
        simpleMissChoiceLayout->addWidget(continueButton);
    }
    m_simpleMissChoicePanel->setVisible(false);
    simpleLayout->addWidget(m_simpleMissChoicePanel);

    auto makeSimpleBallButton = [&](const QString& ballName, int width) -> QPushButton*
    {
        int ballValue = standardBallValue(ballName);
        QColor base(ballColorHex(ballName));
        QColor textColor = (base.lightness() > 150) ? QColor(kBg) : QColor(kWhite);

        QPushButton* ballButton = new QPushButton(
            ballName + "\n(" + QString::number(ballValue) + ")", remotePanelSimple
        );
        ballButton->setFixedSize(width, ballButtonHeight);
        ballButton->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  color: %2;"
            "  border: 1px solid %3;"
            "  border-radius: 8px;"
            "  font-weight: bold;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover { border: 2px solid " + kWhite + "; }"
            "QPushButton:pressed { background-color: %4; }"
            "QPushButton:disabled { background-color: #2a2d31; color: #6a6d71; border-color: #2a2d31; }"
        ).arg(base.name(), textColor.name(), base.darker(150).name(), base.darker(130).name()));

        connect(ballButton, &QPushButton::clicked, this, [this, ballName, ballValue]()
            {
                handleBallAction(ballName, ballValue);
            });

        m_simpleBallButtons[ballName] = ballButton;
        return ballButton;
    };

    QVBoxLayout* simpleBallButtonsLayout = new QVBoxLayout();
    simpleBallButtonsLayout->setSpacing(ballButtonSpacing);
    simpleBallButtonsLayout->addWidget(
        makeSimpleBallButton("Rouge", ballButtonWidth * 2 + ballButtonSpacing)
    );
    for (const auto& pair : colorPairs)
    {
        QHBoxLayout* simplePairRow = new QHBoxLayout();
        simplePairRow->setSpacing(ballButtonSpacing);
        simplePairRow->addWidget(makeSimpleBallButton(pair.first, ballButtonWidth));
        simplePairRow->addWidget(makeSimpleBallButton(pair.second, ballButtonWidth));
        simpleBallButtonsLayout->addLayout(simplePairRow);
    }
    simpleLayout->addLayout(simpleBallButtonsLayout);
    simpleLayout->addSpacing(6);

    // Grille d'actions 2 colonnes : meme ordre et memes libelles que sur
    // la telecommande 1.0 et le telephone (voir actionsGrid), pour que
    // les 3 telecommandes soient identiques a l'exception du compteur
    // (uniquement sur le telephone).
    QGridLayout* simpleActionsGrid = new QGridLayout();
    simpleActionsGrid->setSpacing(8);
    simpleLayout->addLayout(simpleActionsGrid);

    QPushButton* simpleMissShotButton = new QPushButton("Fin de break", remotePanelSimple);
    simpleMissShotButton->setStyleSheet(secondaryButtonStyle);
    connect(simpleMissShotButton, &QPushButton::clicked, this, [this]()
        {
            snapshotFrameForUndo();
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            frame.missShot();
            m_gameManager.afterShot();
            refreshDisplay();
        });
    simpleActionsGrid->addWidget(simpleMissShotButton, 0, 0);

    QPushButton* simpleFoulButton = new QPushButton("Faute", remotePanelSimple);
    simpleFoulButton->setStyleSheet(secondaryButtonStyle);
    connect(simpleFoulButton, &QPushButton::clicked, this, [this]()
        {
            m_pendingAction = PendingAction::Foul;
            refreshDisplay();
        });
    simpleActionsGrid->addWidget(simpleFoulButton, 0, 1);

    // Free ball : menu a 2 choix, meme mecanique que sur la telecommande
    // 1.0 (voir freeBallMenu plus haut) et le telephone.
    QPushButton* simpleFreeBallButton = new QPushButton("Free ball", remotePanelSimple);
    simpleFreeBallButton->setStyleSheet(secondaryButtonStyle);

    QMenu* simpleFreeBallMenu = new QMenu(simpleFreeBallButton);
    simpleFreeBallMenu->setStyleSheet(
        "QMenu { background-color: " + kPanel + "; color: " + kWhite + "; border: 1px solid " + kBorder + "; }"
        "QMenu::item:selected { background-color: " + kBorder + "; }"
    );

    QAction* simpleMissReplayAction = simpleFreeBallMenu->addAction("Remettre en place");
    QAction* simpleChooseStartingBallAction = simpleFreeBallMenu->addAction("Choisir la bille de depart");

    connect(simpleMissReplayAction, &QAction::triggered, this, [this]()
        {
            m_gameManager.getMatch().getCurrentFrame().requestReplay();
            refreshDisplay();
            showRepositioningGuide(/*silentIfUnavailable=*/true);
        });

    connect(simpleChooseStartingBallAction, &QAction::triggered, this, [this]()
        {
            m_pendingAction = PendingAction::ArmFreeBall;
            refreshDisplay();
        });

    simpleFreeBallButton->setMenu(simpleFreeBallMenu);
    simpleActionsGrid->addWidget(simpleFreeBallButton, 2, 1);

    // Miss : arme une action en attente, meme mecanique que "Faute" (voir
    // missButton sur la 1.0). Passe la main a l'adversaire des la bille
    // fautee cliquee ; s'il est snooke, il utilise "Free ball" ci-dessus.
    QPushButton* simpleMissButton = new QPushButton("Miss", remotePanelSimple);
    simpleMissButton->setStyleSheet(secondaryButtonStyle);
    connect(simpleMissButton, &QPushButton::clicked, this, [this]()
        {
            m_pendingAction = PendingAction::Miss;
            refreshDisplay();
        });
    simpleActionsGrid->addWidget(simpleMissButton, 2, 0);

    // Retour : annule le dernier coup (bille empochee, faute ou "Fin de
    // break"), voir snapshotFrameForUndo(). Un seul niveau d'annulation.
    QPushButton* simpleUndoButton = new QPushButton("Retour", remotePanelSimple);
    simpleUndoButton->setStyleSheet(secondaryButtonStyle);
    connect(simpleUndoButton, &QPushButton::clicked, this, [this]()
        {
            if (!m_hasUndoSnapshot)
            {
                return;
            }
            m_gameManager.getMatch().getCurrentFrame() = m_undoSnapshot;
            m_gameManager.getMatch().undoFrameConclusion();
            m_hasUndoSnapshot = false;
            m_pendingAction = PendingAction::None;
            refreshDisplay();
        });
    simpleActionsGrid->addWidget(simpleUndoButton, 1, 0);

    // Game : valide la fin de la frame en cours, meme logique que "Fin
    // de frame" sur la telecommande 1.0 (refuse sur une egalite stricte).
    QPushButton* simpleGameButton = new QPushButton("Game", remotePanelSimple);
    simpleGameButton->setStyleSheet(secondaryButtonStyle);
    connect(simpleGameButton, &QPushButton::clicked, this, [this]()
        {
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            if (!frame.forceFinishFrame())
            {
                showStyledMessage(this, QMessageBox::Information, "Fin de frame",
                    "Impossible : les scores sont a egalite. Il faut d'abord departager "
                    "l'egalite (billes suivantes) avant de pouvoir terminer la frame.");
                return;
            }
            m_gameManager.afterShot();
            refreshDisplay();
        });
    simpleActionsGrid->addWidget(simpleGameButton, 1, 1);

    // Nouveau match : redemande les noms des joueurs et repart d'un
    // match tout neuf, meme mecanique que sur la telecommande 1.0 (voir
    // newMatchButton).
    QPushButton* simpleNewMatchButton = new QPushButton("Nouveau match", remotePanelSimple);
    // Meme correction que newMatchButton sur la 1.0 (voir plus haut).
    simpleNewMatchButton->setStyleSheet(
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 2px; font-size: 12px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(simpleNewMatchButton, &QPushButton::clicked, this, [this]()
        {
            QString player1Name;
            QString player2Name;
            int framesToWin = 2;
            promptPlayerNames(player1Name, player2Name, framesToWin);
            restartMatch(player1Name, player2Name, framesToWin);
        });
    simpleActionsGrid->addWidget(simpleNewMatchButton, 4, 0);

    // Esc : quitte l'ecran de match et revient a l'accueil (voir
    // m_rootStack, index 1). Le match reste construit et en l'etat en
    // arriere-plan -- pas de moyen pour l'instant d'y revenir autrement
    // qu'en relancant un nouveau match depuis l'accueil.
    QPushButton* simpleExitButton = new QPushButton("Esc", remotePanelSimple);
    simpleExitButton->setStyleSheet(secondaryButtonStyle);
    connect(simpleExitButton, &QPushButton::clicked, this, [this]()
        {
            m_rootStack->setCurrentIndex(1);
        });
    simpleActionsGrid->addWidget(simpleExitButton, 4, 1);

    // "Autre" : meme regroupement que sur la telecommande 1.0
    // (otherActionsButton/otherActionsMenu plus haut), demande par
    // l'utilisateur le 2026-09-16 pour que les deux telecommandes aient
    // la meme disposition. Regroupe 6 actions d'arbitrage peu frequentes
    // sous un seul bouton-menu : les 5 qui n'existaient pas du tout sur
    // cette telecommande simplifiee avant ce regroupement (Bille sortie
    // de table, Blanche sortie de table, Recommencer la frame, Conceder
    // la frame, Correction arbitre) rejoignent Bille touchante, qui elle
    // existait deja ici. "Guide de repositionnement"/"Fermer le guide"
    // retires en meme temps que sur la 1.0 : devenus inutilisables des
    // que "Demarrer suivi camera" en a ete retire (aucune telecommande
    // ne peut plus demarrer m_visionTimer).
    QPushButton* simpleOtherActionsButton = new QPushButton("Autre", remotePanelSimple);
    simpleOtherActionsButton->setStyleSheet(secondaryButtonStyle);

    QMenu* simpleOtherActionsMenu = new QMenu(simpleOtherActionsButton);
    simpleOtherActionsMenu->setStyleSheet(
        "QMenu { background-color: " + kPanel + "; color: " + kWhite + "; border: 1px solid " + kBorder + "; }"
        "QMenu::item:selected { background-color: " + kBorder + "; }"
    );

    QAction* simpleTouchingBallAction = simpleOtherActionsMenu->addAction("Bille touchante");
    connect(simpleTouchingBallAction, &QAction::triggered, this, [this]()
        {
            m_gameManager.getMatch().getCurrentFrame().setTouchingBall(true);
            refreshDisplay();
        });

    QAction* simpleBallOffTableAction = simpleOtherActionsMenu->addAction("Bille sortie de table");
    connect(simpleBallOffTableAction, &QAction::triggered, this, [this]()
        {
            m_pendingAction = PendingAction::BallOffTable;
            refreshDisplay();
        });

    QAction* simpleWhiteOffTableAction = simpleOtherActionsMenu->addAction("Blanche sortie de table");
    connect(simpleWhiteOffTableAction, &QAction::triggered, this, [this]()
        {
            triggerBlancheOffTableFoul();
        });

    QAction* simpleRestartFrameAction = simpleOtherActionsMenu->addAction("Recommencer la frame");
    connect(simpleRestartFrameAction, &QAction::triggered, this, [this]()
        {
            QMessageBox box(QMessageBox::Warning, "Recommencer la frame",
                "Annuler tous les points de cette frame et remettre les billes en place\n"
                "(regle du Pat, meme joueur rouvre) ?",
                QMessageBox::Yes | QMessageBox::No, this);
            box.setStyleSheet(
                "QMessageBox { background-color: " + kBg + "; }"
                "QLabel { color: " + kWhite + "; background: transparent; }"
                "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
                "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
            );
            if (box.exec() == QMessageBox::Yes)
            {
                m_gameManager.getMatch().startNewFrame();
                refreshDisplay();
            }
        });

    QAction* simpleConcedeFrameAction = simpleOtherActionsMenu->addAction("Conceder la frame");
    connect(simpleConcedeFrameAction, &QAction::triggered, this, [this]()
        {
            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            QString name1 = QString::fromStdString(frame.getPlayer1().getName());
            QString name2 = QString::fromStdString(frame.getPlayer2().getName());

            QMessageBox box(QMessageBox::Warning, "Conceder la frame",
                "Quel joueur concede la frame en cours ?\n"
                "L'adversaire gagne la frame immediatement, quel que soit le score actuel.",
                QMessageBox::NoButton, this);
            QPushButton* p1Button = box.addButton(name1 + " concede", QMessageBox::AcceptRole);
            QPushButton* p2Button = box.addButton(name2 + " concede", QMessageBox::AcceptRole);
            box.addButton("Annuler", QMessageBox::RejectRole);
            box.setStyleSheet(
                "QMessageBox { background-color: " + kBg + "; }"
                "QLabel { color: " + kWhite + "; background: transparent; }"
                "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
                "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 16px; }"
            );
            box.exec();

            Player* conceder = nullptr;
            if (box.clickedButton() == p1Button)
            {
                conceder = &frame.getPlayer1();
            }
            else if (box.clickedButton() == p2Button)
            {
                conceder = &frame.getPlayer2();
            }
            if (conceder == nullptr)
            {
                return;
            }

            if (!frame.concedeFrame(*conceder))
            {
                return;
            }
            m_gameManager.afterShot();
            refreshDisplay();
        });

    QAction* simpleCorrectionAction = simpleOtherActionsMenu->addAction("Correction arbitre");
    connect(simpleCorrectionAction, &QAction::triggered, this, [this]()
        {
            if (!m_hasUndoSnapshot)
            {
                showStyledMessage(this, QMessageBox::Information, "Correction arbitre",
                    "Aucun coup a corriger (un seul niveau d'annulation disponible, "
                    "meme limite que \"Retour\").");
                return;
            }

            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            const std::vector<LogEntry>& log = frame.getHistory().getLog();
            if (log.empty()
                || (log.back().type != LogEntry::Type::Shot && log.back().type != LogEntry::Type::Foul))
            {
                showStyledMessage(this, QMessageBox::Information, "Correction arbitre",
                    "Le dernier evenement du journal n'est pas un coup ou une faute "
                    "(rien a corriger de cette maniere).");
                return;
            }

            QString before = describeLogEntry(log.back());

            QMessageBox box(QMessageBox::Warning, "Correction arbitre",
                "Dernier coup enregistre : " + before + "\n\n"
                "Annuler ce coup et cliquer ensuite la bille reellement concernee ?",
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

            m_pendingCorrectionBefore = before.toStdString();
            m_gameManager.getMatch().getCurrentFrame() = m_undoSnapshot;
            m_gameManager.getMatch().undoFrameConclusion();
            m_hasUndoSnapshot = false;
            m_pendingAction = PendingAction::CorrectionBall;
            refreshDisplay();
        });

    // Fermer le guide : voir closeGuideAction sur la telecommande 1.0
    // pour le detail -- meme raisonnement, ajoutee ici aussi (2026-09-16).
    QAction* simpleCloseGuideAction = simpleOtherActionsMenu->addAction("Fermer le guide");
    connect(simpleCloseGuideAction, &QAction::triggered, this, [this]()
        {
            closeRepositioningGuide();
        });

    simpleOtherActionsButton->setMenu(simpleOtherActionsMenu);
    simpleActionsGrid->addWidget(simpleOtherActionsButton, 3, 0, 1, 2);

    simpleLayout->addStretch();

    outerLayout->addWidget(remotePanelSimple, 0);
    m_remotePanelSimple = remotePanelSimple;

    {
        QSettings settings(QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);
        m_remoteManualVisible = settings.value("ui/remoteVisible", true).toBool();
    }
    applyRemotePanelVisibility();

    // La vue de match (score + telecommande, tous deux tres denses en
    // widgets empiles verticalement) est plus haute que ne le sont la
    // plupart des ecrans : sans scroll, le bas (compteur de points,
    // derniers boutons de la telecommande) se retrouve hors ecran. On
    // l'enveloppe donc dans un QScrollArea plutot que de l'ajouter tel
    // quel au stack.
    QScrollArea* matchScrollArea = new QScrollArea(this);
    matchScrollArea->setWidget(central);
    matchScrollArea->setWidgetResizable(true);
    matchScrollArea->setFrameShape(QFrame::NoFrame);
    matchScrollArea->setStyleSheet("background-color: " + kBg + ";");

    // Ecran d'accueil (voir HomeScreen) : affiche en premier, avant que
    // le match (deja entierement construit ci-dessus, juste cache) ne
    // soit reellement demarre. Voir startNewMatchFromHome().
    m_rootStack = new CurrentPageStackedWidget(this);
    m_rootStack->addWidget(matchScrollArea); // index 0 : vue du match (cachee au demarrage)

    m_homeScreen = new HomeScreen(this);
    m_homeScreen->setWifiStatus(m_webServer->isRunning());
    m_rootStack->addWidget(m_homeScreen);  // index 1 : ecran d'accueil (affiche au demarrage)

    setCentralWidget(m_rootStack);
    m_rootStack->setCurrentWidget(m_homeScreen);

    connect(m_homeScreen, &HomeScreen::newMatchRequested, this, &MainWindow::startNewMatchFromHome);
    connect(m_homeScreen, &HomeScreen::playersRequested, this, [this]()
        {
            PlayersDialog dialog(this);
            dialog.exec();
        });
    connect(m_homeScreen, &HomeScreen::settingsRequested, this, [this]()
        {
            SettingsDialog dialog(this);
            connect(&dialog, &SettingsDialog::speechEnabledChanged, this, [this](bool enabled)
                {
                    if (m_speech)
                    {
                        m_speech->setEnabled(enabled);
                    }
                    if (m_speechToggleButton)
                    {
                        m_speechToggleButton->setText(enabled ? "Son : Actif" : "Son : Coupe");
                    }
                });
            connect(&dialog, &SettingsDialog::speechGenderChanged, this, [this](QVoice::Gender gender)
                {
                    if (m_speech)
                    {
                        m_speech->setPreferredGender(gender);
                    }
                });
            connect(&dialog, &SettingsDialog::remoteVersionChanged, this, [this](const QString&)
                {
                    // Sans effet tant qu'aucun match n'existe (les deux
                    // panneaux ne sont construits que dans beginMatch()) --
                    // applyRemotePanelVisibility() se garde contre les
                    // pointeurs nuls. Utile si Parametres devient un jour
                    // accessible pendant un match en cours.
                    applyRemotePanelVisibility();
                });
            dialog.exec();
        });
    connect(m_homeScreen, &HomeScreen::tutorialsRequested, this, [this]()
        {
            TutorialDialog dialog(this);
            dialog.exec();
        });
    connect(m_homeScreen, &HomeScreen::tournamentRequested, this, [this]()
        {
            TournamentDialog dialog(this);
            connect(&dialog, &TournamentDialog::matchRequested, this, &MainWindow::startTournamentMatch);
            dialog.exec();
        });
    connect(m_homeScreen, &HomeScreen::rulesRequested, this, [this]()
        {
            RulesReferenceDialog dialog(this);
            dialog.exec();
        });
    connect(m_homeScreen, &HomeScreen::trainingRequested, this, [this]()
        {
            TrainingChoiceDialog dialog(this);
            connect(&dialog, &TrainingChoiceDialog::cueSenseRequested, this, [this]()
                {
                    if (!m_cueSenseLauncher.launch())
                    {
                        showStyledMessage(this, QMessageBox::Warning, "Entrainement",
                            "Impossible de lancer CueSense (dossier introuvable ou aucun port disponible).");
                    }
                });
            connect(&dialog, &TrainingChoiceDialog::exerciseRequested, this, [this]()
                {
                    ExerciseDialog exerciseDialog(this);
                    exerciseDialog.exec();
                });
            dialog.exec();
        });
    connect(m_homeScreen, &HomeScreen::smartphoneRequested, this, [this]()
        {
            // Deja demarre au lancement de l'appli (voir le constructeur) ;
            // repli defensif au cas ou (port indisponible ce jour-la...).
            if (!m_webServer->isRunning() && !m_webServer->start())
            {
                showStyledMessage(this, QMessageBox::Warning, "Smartphone",
                    "Impossible de demarrer le serveur local (aucun port disponible).");
                return;
            }
            if (m_shareButton)
            {
                m_shareButton->setText("Partager en Wi-Fi (actif)");
            }
            ShareSessionDialog dialog(m_webServer, this);
            // Si le telephone envoie les noms des joueurs pendant que ce
            // dialogue est ouvert, handleRemoteControlAction() demarre le
            // match (beginMatch()) mais ne sait rien de ce dialogue local
            // -- sans ceci, le QR code resterait affiche par-dessus la
            // vue du match fraichement construite, exigeant une fermeture
            // manuelle. La connexion est portee par `dialog` : elle se
            // deconnecte toute seule des que le dialogue est detruit.
            connect(m_webServer, &MatchWebServer::controlActionRequested, &dialog,
                [&dialog](const QString& action, const QJsonObject&)
                {
                    if (action == "submitNames")
                    {
                        dialog.accept();
                    }
                });
            dialog.exec();
        });

    // Taille "confortable" de reference (ecran de dev Windows) ramenee a
    // l'espace ecran reellement disponible, et placee explicitement sur
    // l'ecran principal : sur un ecran plus petit (ex. 1440x900) ou avec
    // plusieurs ecrans, 1500x860 deborde et/ou l'OS replace la fenetre sur
    // un ecran secondaire plus petit dont on herite la position au prochain
    // lancement, ce qui coupe le bas de l'accueil (tuiles hors ecran). Fait
    // en tout dernier (apres construction de toutes les pages) : sinon le
    // systeme de layout de Qt peut regrandir la fenetre juste apres, en
    // fonction du sizeHint de la plus grande page (ex. l'ecran de match).
    QScreen* targetScreen = QGuiApplication::primaryScreen();
    QRect availableGeometry = targetScreen ? targetScreen->availableGeometry() : QRect(0, 0, 1500, 860);
    QSize desiredSize(1500, 860);
    QSize windowSize = desiredSize.boundedTo(availableGeometry.size());
    setGeometry(QRect(availableGeometry.topLeft(), windowSize));
}

void MainWindow::startNewMatchFromHome()
{
    QString player1Name;
    QString player2Name;
    int framesToWin = 2;
    promptPlayerNames(player1Name, player2Name, framesToWin);
    beginMatch(player1Name, player2Name, framesToWin);
}

void MainWindow::beginMatch(const QString& player1Name, const QString& player2Name, int framesToWin)
{
    m_gameManager.startNewMatch(player1Name.toStdString(), player2Name.toStdString(), framesToWin);
    resetAutoSaveTracking();
    // A partir d'ici, handleRemoteControlAction() traite les actions du
    // telephone normalement (bille, faute...) -- voir sa garde en debut
    // de fonction.
    m_matchStarted = true;

    m_matchStartTime = QDateTime::currentDateTime();

    m_durationTimer = new QTimer(this);
    m_durationTimer->setInterval(1000);
    connect(m_durationTimer, &QTimer::timeout, this, [this]()
        {
            qint64 matchSecs = m_matchStartTime.secsTo(QDateTime::currentDateTime());
            m_durationLabel->setText(formatDuration(matchSecs));

            // Verifie ici aussi (pas seulement dans refreshDisplay()) pour
            // reagir a une connexion telephone sans attendre le prochain
            // coup joue (ex. le telephone se connecte avant le tout
            // premier coup de la frame).
            applyRemotePanelVisibility();
        });
    m_durationTimer->start();

    m_bestOfLabel->setText(formatBestOfLabel(m_gameManager.getMatch().getFramesToWin()));

    m_nextFrameTimer = new QTimer(this);
    m_nextFrameTimer->setSingleShot(true);
    m_nextFrameTimer->setInterval(3000);
    connect(m_nextFrameTimer, &QTimer::timeout, this, [this]()
        {
            m_gameManager.getMatch().proceedToNextFrame();
            refreshDisplay();
        });

    // Rejeu d'un scenario_*.txt coup par coup (voir bouton "Rejouer le
    // scenario") ; la connexion timeout() est refaite a chaque clic sur
    // le bouton (voir plus bas), donc rien a connecter ici.
    m_replayTimer = new QTimer(this);
    m_replayTimer->setInterval(700);

    m_speech = new SpeechAnnouncer(this);
    m_speech->setEnabled(SettingsDialog::loadSpeechEnabled());
    m_speech->setPreferredGender(SettingsDialog::loadSpeechGender());
    if (m_speechToggleButton)
    {
        m_speechToggleButton->setText(m_speech->isEnabled() ? "Son : Actif" : "Son : Coupe");
    }

    m_visionBridge = new VisionGameBridge();
    // Dossier "photos" a cote de l'executable : une photo de preuve de
    // la table y est sauvegardee automatiquement apres chaque coup
    // confirme par le suivi camera (voir VisionGameBridge::processImage).
    QString photosPath = QCoreApplication::applicationDirPath() + "/photos";
    QDir().mkpath(photosPath);
    m_visionBridge->setSnapshotFolder(photosPath.toStdString());
    m_visionTimer = new QTimer(this);
    m_visionTimer->setInterval(200); // ~5 images/seconde
    connect(m_visionTimer, &QTimer::timeout, this, [this]()
        {
            cv::Mat image;
            if (m_multiCameraMode)
            {
                // Installation complete : assemble les 3 flux en une
                // seule image de la table (voir TableCapture) avant
                // de la passer au pont vision, exactement comme pour
                // une image de camera unique.
                cv::Mat img0, img1, img2;
                if (!m_camera.read(img0) || img0.empty() ||
                    !m_camera1.read(img1) || img1.empty() ||
                    !m_camera2.read(img2) || img2.empty())
                {
                    return;
                }
                image = m_tableCapture.buildFullTableImage(img0, img1, img2);
                if (image.empty())
                {
                    return;
                }
            }
            else if (!m_camera.read(image) || image.empty())
            {
                return;
            }

            Frame& frame = m_gameManager.getMatch().getCurrentFrame();
            m_visionBridge->processImage(image, frame);

            const std::vector<std::string>& nearEdge = m_visionBridge->ballsNearEdge();
            if (nearEdge.empty())
            {
                m_edgeWarningLabel->setVisible(false);
            }
            else
            {
                QString text;
                for (const auto& warning : nearEdge)
                {
                    if (!text.isEmpty())
                    {
                        text += "\n";
                    }
                    text += QString::fromStdString(warning);
                }
                m_edgeWarningLabel->setText(text);
                m_edgeWarningLabel->setVisible(true);
            }

            refreshDisplay();
        });

    connect(m_toggleLogButton, &QPushButton::clicked, this, [this]()
        {
            bool nowVisible = !m_moveLogWidget->isVisible();
            m_moveLogWidget->setVisible(nowVisible);
            m_toggleLogButton->setText(
                nowVisible ? "Masquer le journal des coups" : "Afficher le journal des coups"
            );
        });

    connect(m_toggleRemoteButton, &QPushButton::clicked, this, [this]()
        {
            m_remoteManualVisible = !m_remoteManualVisible;

            QSettings settings(QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);
            settings.setValue("ui/remoteVisible", m_remoteManualVisible);

            applyRemotePanelVisibility();
        });

    m_scenarioRunner = new TestScenarioRunner(m_gameManager, this);
    connect(m_scenarioRunner, &TestScenarioRunner::stepPlayed, this, &MainWindow::refreshDisplay);

    // Le match est pret : on quitte l'ecran d'accueil pour la vue du
    // match (voir m_rootStack, construit dans le constructeur).
    m_rootStack->setCurrentIndex(0);

    refreshDisplay();
}

void MainWindow::restartMatch(const QString& player1Name, const QString& player2Name, int framesToWin)
{
    if (m_nextFrameTimer)
    {
        m_nextFrameTimer->stop();
    }
    m_pendingAction = PendingAction::None;

    m_gameManager.startNewMatch(player1Name.toStdString(), player2Name.toStdString(), framesToWin);
    resetAutoSaveTracking();
    m_bestOfLabel->setText(formatBestOfLabel(m_gameManager.getMatch().getFramesToWin()));

    m_matchStartTime = QDateTime::currentDateTime();
    m_matchSaved = false;
    m_frameEndAnnounced = false;
    m_matchEndAnnounced = false;
    m_lastAnnouncedLogSize = 0;
    m_lastAnnouncedPlayer = nullptr;

    refreshDisplay();
}

void MainWindow::startTournamentMatch(const QString& player1Name, const QString& player2Name)
{
    m_tournamentMatchActive = true;
    if (m_matchStarted)
    {
        restartMatch(player1Name, player2Name);
    }
    else
    {
        beginMatch(player1Name, player2Name);
    }
}

void MainWindow::handleTournamentMatchFinished()
{
    Match& match = m_gameManager.getMatch();
    QString player1Name = QString::fromStdString(match.getPlayer1().getName());
    QString player2Name = QString::fromStdString(match.getPlayer2().getName());
    int framesP1 = match.getFramesPlayer1();
    int framesP2 = match.getFramesPlayer2();
    QString winnerName = (framesP1 > framesP2) ? player1Name : player2Name;
    int winnerFrames = std::max(framesP1, framesP2);
    int loserFrames = std::min(framesP1, framesP2);

    TournamentManager tournament = TournamentManager::load();
    tournament.recordResult(player1Name, player2Name, winnerName, winnerFrames, loserFrames);
    tournament.save();
    m_tournamentMatchActive = false;

    const TournamentManager::Matchup* next = tournament.nextMatch();
    QString message = winnerName + " remporte ce match du tournoi (" + QString::number(winnerFrames)
        + " - " + QString::number(loserFrames) + ").";
    if (tournament.isFinished())
    {
        message += "\n\nTournoi termine ! Champion : " + tournament.champion();
        showStyledMessage(this, QMessageBox::Information, "Tournoi", message);
        return;
    }
    if (!next)
    {
        // Round d'elimination pas encore entierement joue (l'autre
        // demi-finale, par exemple) : rien a proposer pour l'instant.
        showStyledMessage(this, QMessageBox::Information, "Tournoi", message);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Tournoi");
    dialog.setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    QLabel* label = new QLabel(message + "\n\nProchain match : " + next->player1 + " vs " + next->player2, &dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    QHBoxLayout* buttonRow = new QHBoxLayout();
    QPushButton* laterButton = new QPushButton("Plus tard", &dialog);
    QPushButton* launchButton = new QPushButton("Lancer maintenant", &dialog);
    QString dialogButtonStyle =
        "QPushButton {"
        "  background-color: " + kPanel + "; color: " + kWhite + ";"
        "  border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px;"
        "}"
        "QPushButton:hover { border-color: " + kGreen + "; }";
    laterButton->setStyleSheet(dialogButtonStyle);
    launchButton->setStyleSheet(dialogButtonStyle);
    buttonRow->addWidget(laterButton);
    buttonRow->addWidget(launchButton);
    layout->addLayout(buttonRow);
    connect(laterButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(launchButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        startTournamentMatch(next->player1, next->player2);
    }
}

void MainWindow::promptPlayerNames(QString& player1Name, QString& player2Name, int& framesToWin)
{
    // Demande les noms des joueurs, avec un menu deroulant proposant les
    // joueurs deja enregistres en plus de la saisie libre, et un bouton
    // pour retirer un nom de la liste. Utilise au demarrage et par le
    // bouton "Nouveau match".
    player1Name = "Joueur 1";
    player2Name = "Joueur 2";
    framesToWin = 2;
    QStringList savedNames = loadSavedPlayers();
    {
        QDialog nameDialog(this);
        nameDialog.setWindowTitle("Noms des joueurs");
        nameDialog.setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

        QFormLayout* formLayout = new QFormLayout(&nameDialog);

        auto populateCombo = [](QComboBox* combo, const QStringList& names)
            {
                combo->blockSignals(true);
                combo->clear();
                combo->addItem("Saisir un nouveau nom...");
                for (const QString& name : names)
                {
                    combo->addItem(name);
                }
                combo->blockSignals(false);
            };

        const QString comboStyle =
            "QComboBox { background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid %1; border-radius: 4px; padding: 4px; }";
        const QString smallButtonStyle =
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  color: " + kWhite + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 4px;"
            "  padding: 4px 10px;"
            "}"
            "QPushButton:hover { border-color: " + kGray + "; }";

        // --- Joueur 1 ---
        QLabel* label1 = new QLabel("Nom du joueur 1 (vert) :", &nameDialog);
        label1->setStyleSheet("color: " + kGreen + ";");

        QComboBox* player1Combo = new QComboBox(&nameDialog);
        player1Combo->setStyleSheet(comboStyle.arg(kGreen));
        player1Combo->setMinimumWidth(220);

        QPushButton* player1DeleteButton = new QPushButton("Supprimer", &nameDialog);
        player1DeleteButton->setStyleSheet(smallButtonStyle);

        QWidget* player1ComboRow = new QWidget(&nameDialog);
        QHBoxLayout* player1ComboRowLayout = new QHBoxLayout(player1ComboRow);
        player1ComboRowLayout->setContentsMargins(0, 0, 0, 0);
        player1ComboRowLayout->addWidget(player1Combo, 1);
        player1ComboRowLayout->addWidget(player1DeleteButton, 0);

        QLineEdit* player1Edit = new QLineEdit(&nameDialog);
        player1Edit->setPlaceholderText("Ou saisir un nouveau nom");
        player1Edit->setStyleSheet(
            "background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kGreen + "; border-radius: 4px; padding: 6px;"
        );

        // --- Joueur 2 ---
        QLabel* label2 = new QLabel("Nom du joueur 2 (ambre) :", &nameDialog);
        label2->setStyleSheet("color: " + kOrange + ";");

        QComboBox* player2Combo = new QComboBox(&nameDialog);
        player2Combo->setStyleSheet(comboStyle.arg(kOrange));
        player2Combo->setMinimumWidth(220);

        QPushButton* player2DeleteButton = new QPushButton("Supprimer", &nameDialog);
        player2DeleteButton->setStyleSheet(smallButtonStyle);

        QWidget* player2ComboRow = new QWidget(&nameDialog);
        QHBoxLayout* player2ComboRowLayout = new QHBoxLayout(player2ComboRow);
        player2ComboRowLayout->setContentsMargins(0, 0, 0, 0);
        player2ComboRowLayout->addWidget(player2Combo, 1);
        player2ComboRowLayout->addWidget(player2DeleteButton, 0);

        QLineEdit* player2Edit = new QLineEdit(&nameDialog);
        player2Edit->setPlaceholderText("Ou saisir un nouveau nom");
        player2Edit->setStyleSheet(
            "background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kOrange + "; border-radius: 4px; padding: 6px;"
        );

        populateCombo(player1Combo, savedNames);
        populateCombo(player2Combo, savedNames);

        auto deleteSelected = [&savedNames, player1Combo, player2Combo, populateCombo](QComboBox* combo)
            {
                int idx = combo->currentIndex();
                if (idx <= 0)
                {
                    return;
                }
                QString nameToRemove = combo->itemText(idx);
                savedNames.removeAll(nameToRemove);
                savePlayerList(savedNames);
                populateCombo(player1Combo, savedNames);
                populateCombo(player2Combo, savedNames);
            };

        QObject::connect(player1Combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &nameDialog,
            [player1Combo, player1Edit](int idx)
            {
                if (idx > 0)
                {
                    player1Edit->setText(player1Combo->itemText(idx));
                }
            });

        QObject::connect(player2Combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &nameDialog,
            [player2Combo, player2Edit](int idx)
            {
                if (idx > 0)
                {
                    player2Edit->setText(player2Combo->itemText(idx));
                }
            });

        QObject::connect(player1DeleteButton, &QPushButton::clicked, &nameDialog,
            [deleteSelected, player1Combo]() { deleteSelected(player1Combo); });

        QObject::connect(player2DeleteButton, &QPushButton::clicked, &nameDialog,
            [deleteSelected, player2Combo]() { deleteSelected(player2Combo); });

        formLayout->addRow(label1);
        formLayout->addRow(player1ComboRow);
        formLayout->addRow(player1Edit);
        formLayout->addRow(label2);
        formLayout->addRow(player2ComboRow);
        formLayout->addRow(player2Edit);

        // --- Longueur du match : choisie ici, a chaque match, plutot
        // qu'un reglage global dans "Parametres" (discute avec
        // l'utilisateur -- ca depend du temps disponible ce jour-la,
        // pas une preference fixe).
        QLabel* framesLabel = new QLabel("Longueur du match :", &nameDialog);
        framesLabel->setStyleSheet("color: " + kWhite + ";");
        QComboBox* framesCombo = new QComboBox(&nameDialog);
        framesCombo->setStyleSheet(comboStyle.arg(kBorder));
        framesCombo->addItem("1 frame (partie rapide)", 1);
        framesCombo->addItem("Meilleur des 3 frames", 2);
        framesCombo->addItem("Meilleur des 5 frames", 3);
        framesCombo->addItem("Meilleur des 7 frames", 4);
        framesCombo->addItem("Meilleur des 9 frames", 5);
        formLayout->addRow(framesLabel);
        formLayout->addRow(framesCombo);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &nameDialog);
        buttonBox->setStyleSheet("color: " + kWhite + ";");
        formLayout->addRow(buttonBox);

        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &nameDialog, &QDialog::accept);

        // Si le partage Wi-Fi est actif, le telephone connecte affiche un
        // formulaire de saisie des noms (voir MatchWebServer, etat
        // "needsNames"). Cette connexion est locale a cette boite de
        // dialogue (contexte &nameDialog : se deconnecte automatiquement
        // a sa fermeture) : si les noms arrivent depuis le telephone,
        // on les injecte dans les champs et on valide comme si l'OK avait
        // ete clique sur le PC. Fonctionne meme pendant nameDialog.exec()
        // (boucle d'evenements imbriquee) car le signal est livre en file
        // d'attente sur le thread GUI (voir MatchWebServer).
        if (m_webServer->isRunning())
        {
            QLabel* wifiHint = new QLabel(
                "Ou saisissez les noms depuis le telephone connecte.", &nameDialog
            );
            wifiHint->setStyleSheet("color: " + kGray + "; font-size: 11px;");
            formLayout->addRow(wifiHint);

            QObject::connect(m_webServer, &MatchWebServer::controlActionRequested, &nameDialog,
                [&nameDialog, player1Edit, player2Edit](const QString& action, const QJsonObject& params)
                {
                    if (action != "submitNames")
                    {
                        return;
                    }
                    player1Edit->setText(params.value("p1").toString());
                    player2Edit->setText(params.value("p2").toString());
                    nameDialog.accept();
                });
        }

        m_playerNameDialogOpen = true;
        nameDialog.exec();
        m_playerNameDialogOpen = false;

        framesToWin = framesCombo->currentData().toInt();

        QString enteredP1 = player1Edit->text().trimmed();
        QString enteredP2 = player2Edit->text().trimmed();
        if (!enteredP1.isEmpty())
        {
            player1Name = enteredP1;
            if (!savedNames.contains(player1Name))
            {
                savedNames.append(player1Name);
            }
        }
        if (!enteredP2.isEmpty())
        {
            player2Name = enteredP2;
            if (!savedNames.contains(player2Name))
            {
                savedNames.append(player2Name);
            }
        }
        savePlayerList(savedNames);
    }
}

void MainWindow::refreshDisplay()
{
    Frame& frame = m_gameManager.getMatch().getCurrentFrame();

    // Reagit a une connexion/deconnexion telephone survenue depuis le
    // dernier appel (voir applyRemotePanelVisibility()) : refreshDisplay()
    // est appele apres chaque coup et par le minuteur de duree du match,
    // donc assez souvent pour que le masquage/reaffichage semble immediat.
    applyRemotePanelVisibility();

    // Sauvegarde automatique et silencieuse du journal des coups (voir
    // autoSaveMoveLog()) : toujours a jour, pour garder une trace
    // exploitable en cas de plantage ou de bug pendant le match.
    autoSaveMoveLog(m_gameManager.getMatch(), frame);

    announceNewEvents(frame);

    bool p1Active = (&frame.currentPlayer() == &frame.getPlayer1());

    m_player1Frame->setStyleSheet("background-color: transparent; border: none; padding: 10px;");
    m_player2Frame->setStyleSheet("background-color: transparent; border: none; padding: 10px;");

    // Met en surbrillance la case "POINTS MARQUES" du joueur au tir (fond
    // teinte + bordure plus epaisse), pour identifier au premier coup
    // d'oeil qui doit jouer. Le joueur inactif garde sa bordure fine
    // habituelle (identite de couleur verte/orange, mais discrete).
    QString scoreBoxStyle1 = p1Active
        ? "background-color: rgba(26, 144, 0, 60); border: 3px solid " + kGreen + "; border-radius: 5px;"
        : "background-color: " + kPanel + "; border: 1px solid " + kGreen + "; border-radius: 5px;";
    QString scoreBoxStyle2 = !p1Active
        ? "background-color: rgba(245, 166, 35, 60); border: 3px solid " + kOrange + "; border-radius: 5px;"
        : "background-color: " + kPanel + "; border: 1px solid " + kOrange + "; border-radius: 5px;";

    m_player1ScoreBox->setStyleSheet(scoreBoxStyle1);
    m_player2ScoreBox->setStyleSheet(scoreBoxStyle2);

    QString name1 = QString::fromStdString(frame.getPlayer1().getName());
    QString name2 = QString::fromStdString(frame.getPlayer2().getName());

    m_player1NameLabel->setText(name1);
    m_player2NameLabel->setText(name2);

    m_player1ScoreBoxValue->setText(QString::number(frame.getPlayer1().getScore()));
    m_player2ScoreBoxValue->setText(QString::number(frame.getPlayer2().getScore()));

    // Succession des breaks / fautes adverses (voir computePointsBreakdown())
    // plutot qu'une simple somme, pour repondre a la demande de l'utilisateur
    // de voir le detail coup par coup ("15.25.36.3.21" plutot que "79").
    {
        std::vector<int> breaks1, fouls1, breaks2, fouls2;
        computePointsBreakdown(frame.getHistory(), frame.getPlayer1().getName(), breaks1, fouls1);
        computePointsBreakdown(frame.getHistory(), frame.getPlayer2().getName(), breaks2, fouls2);

        m_player1DetailPotted->setText(joinPointsList(breaks1));
        m_player1DetailFouls->setText(joinPointsList(fouls1));

        m_player2DetailPotted->setText(joinPointsList(breaks2));
        m_player2DetailFouls->setText(joinPointsList(fouls2));
    }

    m_pointsRemainingLabel->setText(
        QString::number(frame.pointsRemaining())
    );

    Match& match = m_gameManager.getMatch();
    int framesToWin = match.getFramesToWin();
    int totalFrames = 2 * framesToWin - 1;
    // Plafonne au nombre total de frames possibles : une fois le match
    // termine (ex. 2-1 sur un match en 3 frames gagnantes), le calcul
    // brut framesPlayer1+framesPlayer2+1 depasse totalFrames (ex. "4/3"),
    // alors qu'il n'existe pas de 4e frame -- la derniere frame jouee
    // (la 3e) reste affichee, pas une frame fantome suivante.
    int currentFrameNumber = std::min(match.getFramesPlayer1() + match.getFramesPlayer2() + 1, totalFrames);

    m_frameScoreLabel->setText(
        "<span style='color:" + kGreen + "'>" + QString::number(match.getFramesPlayer1()) + "</span>"
        + " <span style='color:" + kWhite + "'>|</span> "
        + "<span style='color:" + kOrange + "'>" + QString::number(match.getFramesPlayer2()) + "</span>"
    );

    m_currentFrameLabel->setText(
        QString::number(currentFrameNumber) + " / " + QString::number(totalFrames)
    );

    m_breakValueLabel->setText(QString::number(frame.currentPlayer().getBreak()));

    // Billes restantes sur la table (voir m_remainingBallLabels) :
    // cachee des qu'une couleur est retiree definitivement (les rouges
    // en fin de partie, ou une couleur potee pendant les couleurs
    // finales) plutot que juste temporairement respotee.
    const BallSet& ballSet = frame.getBallSet();
    for (auto it = m_remainingBallLabels.begin(); it != m_remainingBallLabels.end(); ++it)
    {
        std::string ballName = it.key().toStdString();
        QLabel* ballLabel = it.value();
        if (!ballSet.isOnTable(ballName))
        {
            ballLabel->setVisible(false);
            continue;
        }
        ballLabel->setVisible(true);
        // Le chiffre n'a d'interet que pour la rouge (nombre de rouges
        // restantes, decroit au fil de la frame) : les couleurs n'existent
        // qu'en un seul exemplaire, afficher "1" dessus n'apporte rien.
        ballLabel->setText(ballName == "Rouge" ? QString::number(ballSet.countBalls(ballName)) : QString());
    }

    // Boutons de bille (voir m_ballButtons/m_simpleBallButtons) : grises
    // (desactives) selon la meme regle que m_remainingBallLabels
    // ci-dessus, pour rendre impossible de jouer/annoncer une bille qui
    // n'est plus physiquement sur la table (ex. "Rouge" une fois les 15
    // rouges epuisees, ou une couleur deja empochee pendant les couleurs
    // finales).
    for (auto it = m_ballButtons.begin(); it != m_ballButtons.end(); ++it)
    {
        it.value()->setEnabled(ballSet.isOnTable(it.key().toStdString()));
    }
    for (auto it = m_simpleBallButtons.begin(); it != m_simpleBallButtons.end(); ++it)
    {
        it.value()->setEnabled(ballSet.isOnTable(it.key().toStdString()));
    }

    if (frame.isFreeBall())
    {
        m_freeBallStatusLabel->setText(
            "FREE BALL : " + QString::fromStdString(frame.getFreeBallColor().getName())
        );
        m_freeBallStatusLabel->setVisible(true);
    }
    else
    {
        m_freeBallStatusLabel->setVisible(false);
    }

    // Egalite apres la derniere couleur (voir FramePhase::BlackReplay) :
    // la noire est respotee et rejouee, et toute faute a ce stade fait
    // perdre la frame sur-le-champ (regle de "mort subite", voir
    // Frame::foul()) -- crucial a signaler immediatement a l'arbitre.
    bool isBlackReplay = (frame.getPhase() == FramePhase::BlackReplay);
    QString blackReplayText = "EGALITE : noire respotee -- la moindre faute perd la frame";
    m_blackReplayStatusLabel->setText(blackReplayText);
    m_blackReplayStatusLabel->setVisible(isBlackReplay);
    m_simpleBlackReplayStatusLabel->setText(blackReplayText);
    m_simpleBlackReplayStatusLabel->setVisible(isBlackReplay);

    // Bille touchante (voir Frame::setTouchingBall()) : signale que le
    // premier contact du prochain coup est deja repute valide.
    bool isTouchingBall = frame.isTouchingBall();
    QString touchingBallText = "BILLE TOUCHANTE : premier contact deja valide pour le prochain coup";
    m_touchingBallStatusLabel->setText(touchingBallText);
    m_touchingBallStatusLabel->setVisible(isTouchingBall);
    m_simpleTouchingBallStatusLabel->setText(touchingBallText);
    m_simpleTouchingBallStatusLabel->setVisible(isTouchingBall);

    QString pendingText;
    switch (m_pendingAction)
    {
    case PendingAction::Foul:
        pendingText = "FAUTE : cliquez la bille fautee";
        break;
    case PendingAction::BallOffTable:
        pendingText = "BILLE SORTIE : cliquez la bille sortie de la table";
        break;
    case PendingAction::ArmFreeBall:
        pendingText = "FREE BALL : cliquez la bille de depart";
        break;
    case PendingAction::ArmFreeBallValue:
        pendingText = "FREE BALL : quelle valeur cette bille remplace-t-elle ? (annoncez la couleur)";
        break;
    case PendingAction::AnnounceFoulTarget:
        pendingText = "FAUTE : quelle bille visiez-vous ? (annoncez la couleur)";
        break;
    case PendingAction::Miss:
        pendingText = "MISS : cliquez la bille fautee";
        break;
    case PendingAction::MissChoice:
        pendingText = "MISS : remettre en place, ou l'adversaire prend la table ?";
        break;
    case PendingAction::CorrectionBall:
        pendingText = "CORRECTION : cliquez la bille reellement concernee";
        break;
    case PendingAction::None:
    default:
        break;
    }
    bool isMissChoicePending = (m_pendingAction == PendingAction::MissChoice);
    if (!pendingText.isEmpty())
    {
        m_pendingActionLabel->setText(pendingText);
        m_pendingActionLabel->setVisible(true);
        m_simplePendingActionLabel->setText(pendingText);
        m_simplePendingActionLabel->setVisible(true);
        // Le bouton "Annuler" ne veut rien dire une fois la faute deja
        // appliquee (voir MissChoice) : seuls les 2 boutons du panneau de
        // choix ci-dessous permettent de sortir de cet etat.
        m_cancelPendingButton->setVisible(!isMissChoicePending);
    }
    else
    {
        m_pendingActionLabel->setVisible(false);
        m_simplePendingActionLabel->setVisible(false);
        m_cancelPendingButton->setVisible(false);
    }
    m_missChoicePanel->setVisible(isMissChoicePending);
    m_simpleMissChoicePanel->setVisible(isMissChoicePending);

    // Publie l'etat courant au serveur web local (voir MatchWebServer),
    // si le partage Wi-Fi est actif : c'est ce qu'un telephone connecte
    // recupere via /api/state/<jeton> pour suivre le score ET agir sur
    // le match a distance (meme etat "action en attente" que la
    // telecommande de bureau, voir pendingText ci-dessus).
    if (m_webServer->isRunning())
    {
        bool p1ActiveForShare = (&frame.currentPlayer() == &frame.getPlayer1());
        QJsonObject shareState;
        shareState["player1Name"] = QString::fromStdString(frame.getPlayer1().getName());
        shareState["player2Name"] = QString::fromStdString(frame.getPlayer2().getName());
        shareState["player1Score"] = frame.getPlayer1().getScore();
        shareState["player2Score"] = frame.getPlayer2().getScore();
        shareState["p1Active"] = p1ActiveForShare;
        shareState["breakValue"] = frame.currentPlayer().getBreak();
        shareState["framesPlayer1"] = match.getFramesPlayer1();
        shareState["framesPlayer2"] = match.getFramesPlayer2();
        shareState["totalFrames"] = totalFrames;
        shareState["pendingActionText"] = pendingText;
        shareState["isFreeBall"] = frame.isFreeBall();
        shareState["isMissChoicePending"] = isMissChoicePending;
        shareState["isBlackReplay"] = isBlackReplay;
        shareState["isTouchingBall"] = isTouchingBall;
        // Meme regle que m_ballButtons/m_simpleBallButtons cote bureau :
        // le telephone grise un bouton de bille des qu'elle n'est plus
        // physiquement sur la table (voir BallSet::isOnTable()).
        QJsonObject ballsOnTable;
        for (const QString& name : { "Rouge", "Jaune", "Verte", "Marron", "Bleue", "Rose", "Noire" })
        {
            ballsOnTable[name] = ballSet.isOnTable(name.toStdString());
        }
        shareState["ballsOnTable"] = ballsOnTable;
        m_webServer->updateState(shareState);
    }

    Ball requiredBall = frame.getRequiredBall();
    QString requiredBallName = QString::fromStdString(requiredBall.getName());
    m_nextBallDot->setStyleSheet(ballGlossStyle(requiredBallName, 36));

    // ---------------------------------------------------
    // Succession des billes empochees : on repart de zero et
    // on recree une pastille par coup reussi (les fautes ne
    // sont pas des billes empochees, on les ignore ici).
    // ---------------------------------------------------
    QGridLayout* successionGridLayout = qobject_cast<QGridLayout*>(m_successionRow->layout());
    QLayoutItem* item;
    while ((item = successionGridLayout->takeAt(0)) != nullptr)
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const std::vector<LogEntry>& log = frame.getHistory().getLog();
    std::string currentPlayerName = frame.currentPlayer().getName();

    // On ne garde que les coups reussis du joueur actif, depuis la fin
    // de la sequence (remonte tant que ce sont des coups du meme joueur) :
    // ca correspond au break en cours, qui repart de zero des qu'on
    // change de joueur (faute ou tour de l'adversaire).
    int startIndex = static_cast<int>(log.size());
    for (int i = static_cast<int>(log.size()) - 1; i >= 0; --i)
    {
        if (log[i].type == LogEntry::Type::Shot && log[i].playerName == currentPlayerName)
        {
            startIndex = i;
        }
        else
        {
            break;
        }
    }

    // Collecte d'abord les billes a afficher, pour connaitre le nombre
    // total de rangees et inserer un separateur en pointilles entre
    // chaque rangee (comme sur la maquette), sans avoir a deviner a
    // l'avance combien de coups seront joues.
    std::vector<std::string> successionBalls;
    for (int i = startIndex; i < static_cast<int>(log.size()); ++i)
    {
        const LogEntry& entry = log[i];
        if (entry.type == LogEntry::Type::Shot)
        {
            successionBalls.push_back(entry.ballName);
        }
    }

    const int columnsPerRow = 18;
    const int ballDiameter = 40;
    int rowCount = successionBalls.empty()
        ? 0
        : (static_cast<int>(successionBalls.size()) - 1) / columnsPerRow + 1;

    for (int i = 0; i < static_cast<int>(successionBalls.size()); ++i)
    {
        int col = i % columnsPerRow;
        int ballRowIndex = i / columnsPerRow;
        int gridRow = ballRowIndex * 2; // une ligne sur deux est reservee au separateur

        QLabel* dot = new QLabel(m_successionRow);
        dot->setFixedSize(ballDiameter, ballDiameter);
        dot->setStyleSheet(ballGlossStyle(QString::fromStdString(successionBalls[i]), ballDiameter));
        dot->setToolTip(QString::fromStdString(successionBalls[i]));
        applySoftShadow(dot, 10, 3);
        successionGridLayout->addWidget(dot, gridRow, col);
    }

    for (int r = 0; r < rowCount - 1; ++r)
    {
        QFrame* separator = new QFrame(m_successionRow);
        separator->setFixedHeight(1);
        separator->setStyleSheet("background: transparent; border-top: 1px dashed " + kGreen + ";");
        successionGridLayout->addWidget(separator, r * 2 + 1, 0, 1, columnsPerRow);
    }

    if (match.isMatchFinished())
    {
        m_recordingDot->setStyleSheet("background-color: " + kGray + "; border-radius: 5px;");
        m_recordingStatusLabel->setText("ARRETE");
        m_recordingStatusLabel->setStyleSheet("color: " + kGray + "; font-size: 12px; font-weight: bold;");
    }
    else
    {
        m_recordingDot->setStyleSheet("background-color: " + kRecordingRed + "; border-radius: 5px;");
        m_recordingStatusLabel->setText("ACTIF");
        m_recordingStatusLabel->setStyleSheet("color: " + kRecordingRed + "; font-size: 12px; font-weight: bold;");
    }

    if (match.isMatchFinished() && !m_matchSaved)
    {
        MatchStorage::saveMatch(match);
        m_matchSaved = true;

        if (m_tournamentMatchActive)
        {
            // Differe au tour de boucle d'evenements suivant plutot que
            // d'appeler directement ici : handleTournamentMatchFinished()
            // peut enchainer sur restartMatch(), qui reassigne le Match
            // interne et appelle refreshDisplay() -- rappeler
            // refreshDisplay() alors qu'on est encore DANS refreshDisplay()
            // (via dialog.exec() plus bas dans la pile) laisserait les
            // references locales `frame`/`match` de cet appel-ci pointer
            // vers des donnees perimees pour le reste de la fonction.
            QTimer::singleShot(0, this, &MainWindow::handleTournamentMatchFinished);
        }
    }

    m_moveLogWidget->refresh(frame.getHistory());

    // ---------------------------------------------------
    // Fin de frame : on laisse le score final affiche quelques
    // secondes avant de reellement demarrer la frame suivante,
    // pour que le resultat soit visible et pas ecrase instantanement.
    // ---------------------------------------------------
    if (match.isFrameJustFinished() && !m_nextFrameTimer->isActive())
    {
        m_nextFrameTimer->start();
    }
}

void MainWindow::announceNewEvents(Frame& frame)
{
    const std::vector<LogEntry>& log = frame.getHistory().getLog();

    // Le journal redevient plus court que ce qu'on a deja annonce : une
    // nouvelle frame a demarre (ou un scenario a ete rejoue depuis le
    // debut). On repart de zero plutot que de rester bloque a attendre
    // que le journal depasse a nouveau son ancienne taille.
    if (log.size() < m_lastAnnouncedLogSize)
    {
        m_lastAnnouncedLogSize = 0;
        m_lastAnnouncedPlayer = nullptr;
    }

    // Break en cours par joueur, recalcule depuis le debut du journal a
    // chaque appel (cout negligeable vu la taille d'une frame) : remis a
    // zero a chaque Faute/Fin de tour (comme Player::resetBreak(), voir
    // Frame::switchPlayer()), accumule a chaque bille reussie. On annonce
    // le BREAK (score du tour en cours), pas la valeur de la bille seule,
    // pour coller a ce qu'un vrai arbitre annonce ("Rouge, 12 points" =
    // le break passe a 12, pas "1 point" = la valeur de la rouge).
    std::map<std::string, int> breakByPlayer;
    for (size_t i = 0; i < log.size(); ++i)
    {
        const LogEntry& entry = log[i];
        if (entry.type == LogEntry::Type::Shot)
        {
            breakByPlayer[entry.playerName] += entry.points;
        }
        else
        {
            breakByPlayer[entry.playerName] = 0;
        }

        if (i < m_lastAnnouncedLogSize)
        {
            continue;
        }

        if (entry.type == LogEntry::Type::Shot)
        {
            // Uniquement la valeur du break, jamais le nom de la bille :
            // annoncer la couleur est reserve au cas ou l'arbitre demande
            // au joueur quelle bille il joue et repete sa reponse (voir
            // project_vision_bridge_status, pas encore implemente - la
            // reconnaissance vocale n'existe pas encore).
            m_speech->announce(QString::number(breakByPlayer[entry.playerName]));
        }
        else if (entry.type == LogEntry::Type::Foul)
        {
            m_speech->announce("Faute, " + QString::number(entry.foulPoints));
        }
        // Type::Miss (fin de tour sans bille jouee) : pas d'annonce
        // dediee, le changement de joueur ci-dessous suffit a signaler
        // la fin du tour.
    }
    m_lastAnnouncedLogSize = log.size();

    Player* currentPlayer = &frame.currentPlayer();
    if (m_lastAnnouncedPlayer != nullptr && currentPlayer != m_lastAnnouncedPlayer)
    {
        m_speech->announce("Au tour de " + QString::fromStdString(currentPlayer->getName()));
    }
    m_lastAnnouncedPlayer = currentPlayer;

    // Egalite apres la derniere couleur (voir FramePhase::BlackReplay) :
    // annonce une seule fois par occurrence (peut se reproduire plusieurs
    // fois dans la meme frame si la noire est repotee a nouveau apres un
    // nouvel empochage a egalite).
    if (frame.getPhase() == FramePhase::BlackReplay)
    {
        if (!m_blackReplayAnnounced)
        {
            m_blackReplayAnnounced = true;
            m_speech->announce("Egalite. La noire est respotee.");
        }
    }
    else
    {
        m_blackReplayAnnounced = false;
    }

    // Annonce de fin de frame / fin de match. m_frameEndAnnounced/
    // m_matchEndAnnounced evitent de repeter l'annonce a chaque
    // refreshDisplay() tant que le score final reste affiche (voir
    // m_nextFrameTimer, quelques secondes avant la frame suivante).
    Match& match = m_gameManager.getMatch();
    if (match.isFrameJustFinished())
    {
        if (!m_frameEndAnnounced)
        {
            m_frameEndAnnounced = true;
            m_speech->announce("Frame pour " + QString::fromStdString(frame.getWinnerName()));
        }
    }
    else
    {
        m_frameEndAnnounced = false;
    }

    if (match.isMatchFinished() && !m_matchEndAnnounced)
    {
        m_matchEndAnnounced = true;
        std::string winnerName = (match.getFramesPlayer1() > match.getFramesPlayer2())
            ? match.getPlayer1().getName()
            : match.getPlayer2().getName();
        m_speech->announce(QString::fromStdString(winnerName) + " gagne le match");
    }
}

void MainWindow::snapshotFrameForUndo()
{
    m_undoSnapshot = m_gameManager.getMatch().getCurrentFrame();
    m_hasUndoSnapshot = true;
}

void MainWindow::applyRemotePanelVisibility()
{
    if (!m_remotePanel || !m_remotePanelSimple)
    {
        return;
    }

    // Un telephone connecte prend le relais de la telecommande de bureau
    // (voir MatchWebServer::hasActiveClient()) : elle se masque toute
    // seule, sans toucher a la preference manuelle de l'utilisateur
    // (m_remoteManualVisible), qui reprend effet des que le telephone
    // se deconnecte.
    bool phoneConnected = m_webServer && m_webServer->hasActiveClient();
    bool effectiveVisible = m_remoteManualVisible && !phoneConnected;

    QString version = SettingsDialog::loadRemoteVersion();
    m_remotePanel->setVisible(effectiveVisible && version != "2.0");
    m_remotePanelSimple->setVisible(effectiveVisible && version == "2.0");

    if (m_toggleRemoteButton)
    {
        m_toggleRemoteButton->setText(
            m_remoteManualVisible ? "Masquer telecommande" : "Afficher telecommande"
        );
    }
}

void MainWindow::showRepositioningGuide(bool silentIfUnavailable)
{
    if (m_repositionGuideDialog)
    {
        m_repositionGuideDialog->raise();
        m_repositionGuideDialog->activateWindow();
        return;
    }

    if (!m_visionTimer->isActive())
    {
        if (!silentIfUnavailable)
        {
            showStyledMessage(this, QMessageBox::Information, "Guide de repositionnement",
                "Demarrez d'abord le suivi camera.");
        }
        return;
    }
    if (m_visionBridge->lastBallMapPath().empty())
    {
        if (!silentIfUnavailable)
        {
            showStyledMessage(this, QMessageBox::Information, "Guide de repositionnement",
                "Aucun coup confirme pour l'instant : rien a comparer.");
        }
        return;
    }

    cv::Mat image;
    if (m_multiCameraMode)
    {
        cv::Mat img0, img1, img2;
        if (!m_camera.read(img0) || img0.empty() ||
            !m_camera1.read(img1) || img1.empty() ||
            !m_camera2.read(img2) || img2.empty())
        {
            if (!silentIfUnavailable)
            {
                showStyledMessage(this, QMessageBox::Warning, "Guide de repositionnement", "Impossible de lire l'image camera.");
            }
            return;
        }
        image = m_tableCapture.buildFullTableImage(img0, img1, img2);
    }
    else if (!m_camera.read(image) || image.empty())
    {
        if (!silentIfUnavailable)
        {
            showStyledMessage(this, QMessageBox::Warning, "Guide de repositionnement", "Impossible de lire l'image camera.");
        }
        return;
    }

    if (image.empty())
    {
        if (!silentIfUnavailable)
        {
            showStyledMessage(this, QMessageBox::Warning, "Guide de repositionnement", "Impossible de lire l'image camera.");
        }
        return;
    }

    BallMapRecorder recorder(image.size());
    std::map<std::string, std::vector<cv::Point2f>> target =
        recorder.loadSnapshot(m_visionBridge->lastBallMapPath());
    cv::Mat guide = recorder.renderRepositioningGuide(m_visionBridge->tracker().getAllTracked(), target);

    m_repositionGuideDialog = showRepositioningGuideDialog(this, guide, m_webServer);
}

void MainWindow::closeRepositioningGuide()
{
    if (m_repositionGuideDialog)
    {
        m_repositionGuideDialog->close();
    }
}

void MainWindow::triggerBlancheOffTableFoul()
{
    snapshotFrameForUndo();

    Frame& frame = m_gameManager.getMatch().getCurrentFrame();
    Ball blanche("Blanche", 0);
    Referee foulReferee;

    // Meme ambiguite que Foul/BallOffTable (voir handleBallAction) : si
    // n'importe quelle couleur etait legale, il faut que l'arbitre precise
    // laquelle avant de pouvoir calculer/appliquer la penalite.
    Ball required = frame.getRequiredBall();
    if (required.getName() == "Couleur")
    {
        m_pendingFoulTouchedBall = blanche;
        m_pendingFoulReason = "Blanche sortie de la table";
        m_pendingAction = PendingAction::AnnounceFoulTarget;
        refreshDisplay();
        return;
    }

    int penalty = foulReferee.calculateFoul(required, blanche);
    frame.foul(required, blanche, penalty, "Blanche sortie de la table");
    m_gameManager.afterShot();
    m_pendingAction = PendingAction::None;
    refreshDisplay();
}

void MainWindow::handleBallAction(const QString& ballName, int ballValue)
{
    snapshotFrameForUndo();

    Frame& frame = m_gameManager.getMatch().getCurrentFrame();
    Ball clickedBall(ballName.toStdString(), ballValue);

    // La penalite d'une faute depend du MAXIMUM entre la valeur de la
    // bille demandee et celle de la bille touchee (regle du snooker,
    // min 4, max 7) : jamais seulement la bille touchee, sinon une
    // faute sur la Noire (7) demandee mais Rouge (1) touchee ne coute
    // a tort que 4 points.
    Referee foulReferee;

    switch (m_pendingAction)
    {
    case PendingAction::Foul:
    {
        Ball required = frame.getRequiredBall();
        // "N'importe quelle couleur" etait legale (meme ambiguite que pour
        // le Free Ball, voir Frame::isFreeBallValueAmbiguous()) : impossible
        // de calculer la penalite sans savoir quelle bille etait visee.
        // On memorise la bille touchee et on demande l'annonce au clic
        // suivant (voir PendingAction::AnnounceFoulTarget ci-dessous).
        if (required.getName() == "Couleur")
        {
            m_pendingFoulTouchedBall = clickedBall;
            m_pendingFoulReason = "Mauvaise bille touchee";
            m_pendingAction = PendingAction::AnnounceFoulTarget;
            refreshDisplay();
            return;
        }
        int penalty = foulReferee.calculateFoul(required, clickedBall);
        frame.foul(required, clickedBall, penalty);
        m_gameManager.afterShot();
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    case PendingAction::BallOffTable:
    {
        Ball required = frame.getRequiredBall();
        if (required.getName() == "Couleur")
        {
            m_pendingFoulTouchedBall = clickedBall;
            m_pendingFoulReason = "Bille sortie de la table";
            m_pendingAction = PendingAction::AnnounceFoulTarget;
            refreshDisplay();
            return;
        }
        int penalty = foulReferee.calculateFoul(required, clickedBall);
        frame.foul(required, clickedBall, penalty, "Bille sortie de la table");
        m_gameManager.afterShot();
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    case PendingAction::AnnounceFoulTarget:
    {
        // clickedBall est ici la bille VISEE/annoncee par l'arbitre (pas
        // la bille touchee, deja memorisee dans m_pendingFoulTouchedBall
        // au clic precedent, voir Foul/BallOffTable ci-dessus).
        int penalty = foulReferee.calculateFoul(clickedBall, m_pendingFoulTouchedBall);
        frame.foul(clickedBall, m_pendingFoulTouchedBall, penalty, m_pendingFoulReason);
        m_gameManager.afterShot();
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    case PendingAction::CorrectionBall:
    {
        // Voir le bouton "Correction arbitre" : le mauvais coup vient
        // d'etre annule, clickedBall est la bille REELLEMENT concernee --
        // on la rejoue normalement (playShot() redetermine lui-meme si
        // c'est legal ou fautif a partir de l'etat reel), puis on garde une
        // trace explicite "avant -> apres" dans le journal.
        frame.playShot(clickedBall);
        QString after = describeLogEntry(frame.getHistory().getLog().back());
        frame.logCorrection(m_pendingCorrectionBefore, after.toStdString());
        m_pendingCorrectionBefore.clear();
        m_gameManager.afterShot();
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    case PendingAction::ArmFreeBall:
    {
        frame.setFreeBall(true);
        frame.setFreeBallColor(clickedBall);
        // Si "n'importe quelle couleur" etait legale (Frame::getRequiredBall()
        // ambigu), impossible de deduire automatiquement la valeur a
        // compter : on demande explicitement a l'arbitre de l'annoncer
        // avant de pouvoir jouer le coup (voir isFreeBallValueAmbiguous()).
        // Sinon (rouge ou couleur des couleurs finales) c'est deja deduit.
        m_pendingAction = frame.isFreeBallValueAmbiguous()
            ? PendingAction::ArmFreeBallValue
            : PendingAction::None;
        refreshDisplay();
        return;
    }
    case PendingAction::ArmFreeBallValue:
    {
        frame.setFreeBallValue(clickedBall);
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    case PendingAction::Miss:
    {
        Ball required = frame.getRequiredBall();
        // Meme ambiguite que Foul/BallOffTable ci-dessus (voir leurs
        // commentaires) : reutilise le meme mecanisme d'annonce.
        if (required.getName() == "Couleur")
        {
            m_pendingFoulTouchedBall = clickedBall;
            m_pendingFoulReason = "Absence de veritable tentative (Miss)";
            m_pendingAction = PendingAction::AnnounceFoulTarget;
            refreshDisplay();
            return;
        }
        int penalty = foulReferee.calculateFoul(required, clickedBall);
        frame.foul(required, clickedBall, penalty, "Absence de veritable tentative (Miss)");
        m_gameManager.afterShot();
        // La main passe deja naturellement a l'adversaire (comme une faute
        // normale, equivalent a "il joue la position telle quelle"), mais
        // on ne cloture pas encore l'action en attente : MissChoice
        // demande explicitement a l'adversaire s'il prend la table ainsi
        // ou s'il prefere faire rejouer le fautif (voir m_missChoicePanel).
        m_pendingAction = PendingAction::MissChoice;
        refreshDisplay();
        return;
    }
    case PendingAction::None:
    default:
        break;
    }

    // Aucune action en attente : coup normal.
    if (frame.isFreeBall())
    {
        frame.playFreeBall(clickedBall);
    }
    else
    {
        frame.playShot(clickedBall);
    }
    m_gameManager.afterShot();
    refreshDisplay();
}

void MainWindow::handleRemoteControlAction(const QString& action, const QJsonObject& params)
{
    if (action == "submitNames")
    {
        if (m_matchStarted || m_playerNameDialogOpen)
        {
            // Partie deja en cours (rien a faire), ou dialogue PC deja
            // ouvert et deja abonne au meme signal (voir le connect()
            // local dans promptPlayerNames()) : le laisser gerer ce cas
            // pour ne demarrer le match qu'une seule fois.
            return;
        }
        // Cas du QR code imprime sur la table, scanne sans que personne
        // n'ait touche au PC (voir HomeScreen) : demarre le match
        // directement avec les noms (et la longueur de match) saisis sur
        // le telephone.
        QString p1 = params.value("p1").toString().trimmed();
        QString p2 = params.value("p2").toString().trimmed();
        int frames = params.value("frames").toInt(2);
        beginMatch(p1.isEmpty() ? "Joueur 1" : p1, p2.isEmpty() ? "Joueur 2" : p2, frames);
        return;
    }

    if (!m_matchStarted)
    {
        // Action recue avant tout demarrage de match (ne devrait pas
        // arriver : la page telephone n'affiche ces boutons qu'une fois
        // needsNames redevenu faux, voir MatchWebServer/kPageHtml) --
        // garde defensive plutot qu'un plantage sur getCurrentFrame().
        return;
    }

    Frame& frame = m_gameManager.getMatch().getCurrentFrame();

    if (action == "ball")
    {
        QString ballName = params.value("name").toString();
        handleBallAction(ballName, standardBallValue(ballName));
        return;
    }
    if (action == "armFoul")
    {
        m_pendingAction = PendingAction::Foul;
        refreshDisplay();
        return;
    }
    if (action == "armBallOffTable")
    {
        m_pendingAction = PendingAction::BallOffTable;
        refreshDisplay();
        return;
    }
    if (action == "armFreeBall")
    {
        m_pendingAction = PendingAction::ArmFreeBall;
        refreshDisplay();
        return;
    }
    if (action == "whiteOffTable")
    {
        // Meme fonction que le bouton "Blanche sortie de table" des
        // telecommandes de bureau : action immediate (la bille concernee
        // est deja connue), pas d'attente de clic sur une bille.
        triggerBlancheOffTableFoul();
        return;
    }
    if (action == "restartFrame")
    {
        // Meme fonction que "Recommencer la frame" sur PC, mais la
        // confirmation se fait cote telephone (JS confirm(), voir
        // kPageHtml) plutot qu'une QMessageBox -- aucune boite de
        // dialogue PC n'est possible depuis cette page.
        m_gameManager.getMatch().startNewFrame();
        refreshDisplay();
        return;
    }
    if (action == "concedeFrame")
    {
        // Meme fonction que "Conceder la frame" sur PC ; le choix du
        // joueur qui concede se fait sur la page telephone (2 boutons
        // nommes d'apres l'etat courant, voir kPageHtml) plutot que dans
        // une QMessageBox -- envoye ici comme un simple numero 1 ou 2.
        int playerNumber = params.value("player").toInt();
        if (playerNumber != 1 && playerNumber != 2)
        {
            return;
        }
        Player& conceder = (playerNumber == 1) ? frame.getPlayer1() : frame.getPlayer2();
        if (!frame.concedeFrame(conceder))
        {
            return;
        }
        m_gameManager.afterShot();
        refreshDisplay();
        return;
    }
    if (action == "armCorrection")
    {
        // Meme fonction que "Correction arbitre" sur PC, mais sans
        // l'apercu "avant -> " dans une boite de dialogue (impossible
        // depuis cette page) : annule directement le dernier coup (comme
        // "Retour") si les memes conditions sont reunies, puis arme
        // PendingAction::CorrectionBall -- le bandeau "pending" (voir
        // pendingText plus haut) guide alors vers la bille a cliquer,
        // exactement comme pour "armFoul"/"armBallOffTable".
        if (!m_hasUndoSnapshot)
        {
            return;
        }
        const std::vector<LogEntry>& log = frame.getHistory().getLog();
        if (log.empty()
            || (log.back().type != LogEntry::Type::Shot && log.back().type != LogEntry::Type::Foul))
        {
            return;
        }
        m_pendingCorrectionBefore = describeLogEntry(log.back()).toStdString();
        m_gameManager.getMatch().getCurrentFrame() = m_undoSnapshot;
        m_gameManager.getMatch().undoFrameConclusion();
        m_hasUndoSnapshot = false;
        m_pendingAction = PendingAction::CorrectionBall;
        refreshDisplay();
        return;
    }
    if (action == "armMiss")
    {
        m_pendingAction = PendingAction::Miss;
        refreshDisplay();
        return;
    }
    if (action == "touchingBall")
    {
        // Action immediate (pas d'attente de bille), voir touchingBallButton
        // dans le constructeur : arme juste l'etat pour le prochain coup.
        frame.setTouchingBall(true);
        refreshDisplay();
        return;
    }
    if (action == "missReplay")
    {
        // Action immediate (pas d'attente de bille) : utilisee a la fois
        // par le menu du bouton "Free ball" et par le panneau de choix
        // suivant un Miss (voir PendingAction::MissChoice) -- remise a
        // None systematique, sans effet si deja None (cas Free ball).
        frame.requestReplay();
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        showRepositioningGuide(/*silentIfUnavailable=*/true);
        return;
    }
    if (action == "missChoiceContinue")
    {
        // "Prendre la table" suivant un Miss : la main est deja sur
        // l'adversaire (voir PendingAction::Miss dans handleBallAction()),
        // il ne reste qu'a clore le choix en attente.
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    if (action == "cancel")
    {
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    if (action == "missShot")
    {
        // "Fin de break" : coup rate volontaire, pas une action en
        // attente (voir missShotButton dans le constructeur).
        snapshotFrameForUndo();
        frame.missShot();
        m_gameManager.afterShot();
        refreshDisplay();
        return;
    }
    if (action == "finishFrame")
    {
        if (!frame.forceFinishFrame())
        {
            // Egalite : le bouton de bureau affiche un message
            // explicatif ici, mais depuis le telephone on se contente
            // de ne rien faire (pas de boite de dialogue possible sur
            // cette page).
            return;
        }
        m_gameManager.afterShot();
        refreshDisplay();
        return;
    }
    if (action == "undo")
    {
        // Meme logique que le bouton "Retour" de la telecommande de
        // bureau 2.0 : un seul niveau d'annulation (voir
        // snapshotFrameForUndo()).
        if (!m_hasUndoSnapshot)
        {
            return;
        }
        m_gameManager.getMatch().getCurrentFrame() = m_undoSnapshot;
        m_gameManager.getMatch().undoFrameConclusion();
        m_hasUndoSnapshot = false;
        m_pendingAction = PendingAction::None;
        refreshDisplay();
        return;
    }
    if (action == "goHome")
    {
        // Meme logique que le bouton "Esc" de la telecommande de bureau
        // 2.0 : quitte l'ecran de match et revient a l'accueil.
        m_rootStack->setCurrentIndex(1);
        return;
    }
    if (action == "newMatch")
    {
        // Meme logique que le bouton "Nouveau match" de la telecommande
        // de bureau 1.0 (restartMatch()), mais les noms viennent du
        // formulaire redemande sur la page telephone (voir
        // showNewMatchForm() dans kPageHtml) plutot que d'une boite de
        // dialogue PC. Contrairement a "submitNames", volontairement PAS
        // bloque par m_matchStarted : c'est justement pour redemarrer un
        // match deja en cours.
        QString p1 = params.value("p1").toString().trimmed();
        QString p2 = params.value("p2").toString().trimmed();
        int frames = params.value("frames").toInt(2);
        restartMatch(p1.isEmpty() ? "Joueur 1" : p1, p2.isEmpty() ? "Joueur 2" : p2, frames);
        return;
    }
    if (action == "openRepositionGuide")
    {
        // Meme bouton que sur les telecommandes de bureau (1.0 et 2.0) :
        // declenchement manuel explicite, donc pas silencieux (affiche le
        // message d'erreur habituel sur le PC si la camera n'est pas prete).
        showRepositioningGuide(/*silentIfUnavailable=*/false);
        return;
    }
    // "closeRepositionGuide" n'a rien a faire ici : gere directement par
    // la connexion locale au dialogue dans showRepositioningGuideDialog()
    // (voir MainWindow.cpp, plus haut), qui se deconnecte toute seule
    // quand ce dialogue n'est pas ouvert.
}

// Demarre ou arrete le suivi camera en direct. Au demarrage, ouvre la
// camera 0 (webcam par defaut) ; si aucune camera n'est disponible,
// previent l'utilisateur au lieu de demarrer un timer inutile.
void MainWindow::toggleVisionTracking()
{
    if (m_visionTimer->isActive())
    {
        m_visionTimer->stop();
        m_camera.release();
        m_camera1.release();
        m_camera2.release();
        m_multiCameraMode = false;
        m_visionButton->setText("Demarrer suivi camera");
        m_edgeWarningLabel->setVisible(false);
        return;
    }

    if (!m_camera.open(0))
    {
        showStyledMessage(this, QMessageBox::Warning, "Suivi camera",
            "Aucune camera detectee (index 0). Verifiez qu'une camera est branchee.");
        return;
    }

    // Demande explicitement la plus haute resolution disponible (jusqu'a
    // la 4K) : sans ca, OpenCV/Windows se contente souvent d'une
    // resolution par defaut basse (ex. 640x480) meme sur une camera 4K.
    // Sans effet sur une camera qui ne supporte pas cette resolution
    // (elle retombe silencieusement sur la sienne). VisionGameBridge
    // redimensionne de toute facon une copie de travail pour l'analyse
    // (voir m_analysisWidth), donc l'image d'origine peut rester grande
    // sans ralentir la detection.
    m_camera.set(cv::CAP_PROP_FRAME_WIDTH, 3840);
    m_camera.set(cv::CAP_PROP_FRAME_HEIGHT, 2160);

    // Tente d'ouvrir les 2 autres cameras (installation complete a 3
    // cameras, voir TableCapture). Si l'une des deux manque (ex. poste
    // de developpement avec une seule webcam, en attendant les 3
    // cameras reelles), on repart en mode simple (camera 0 seule) plutot
    // que d'echouer completement : c'est le comportement d'avant.
    bool cam1Opened = m_camera1.open(1);
    bool cam2Opened = cam1Opened && m_camera2.open(2);
    m_multiCameraMode = cam1Opened && cam2Opened;

    if (m_multiCameraMode)
    {
        m_camera1.set(cv::CAP_PROP_FRAME_WIDTH, 3840);
        m_camera1.set(cv::CAP_PROP_FRAME_HEIGHT, 2160);
        m_camera2.set(cv::CAP_PROP_FRAME_WIDTH, 3840);
        m_camera2.set(cv::CAP_PROP_FRAME_HEIGHT, 2160);
    }
    else
    {
        m_camera1.release();
        m_camera2.release();
    }

    m_visionBridge->reset();
    m_visionTimer->start();
    m_visionButton->setText(
        m_multiCameraMode ? "Arreter suivi camera (3 cameras)" : "Arreter suivi camera"
    );
}