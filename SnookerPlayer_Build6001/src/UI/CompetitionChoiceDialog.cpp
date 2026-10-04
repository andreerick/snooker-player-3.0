#include "CompetitionChoiceDialog.h"
#include "UiUtils.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QRect>

#ifndef HOME_SCREEN_IMAGE_PATH
#error "HOME_SCREEN_IMAGE_PATH doit etre defini par CMakeLists.txt (voir target_compile_definitions)"
#endif

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kWhite = "#f5f5f5";
    const QString kGray = "#7a7f87";
    const QString kGold = "#e6bc00";
    const QString kBlue = "#3aa0ff";

    constexpr int kIconWidth = 96;
    constexpr int kIconHeight = 76;

    // Icone du tournoi : le trophee deja dessine dans l'image d'accueil
    // (tuile "Tournoi", voir HomeScreen), decoupe et detouré en rendant
    // transparent son fond quasi noir (sinon un rectangle sombre se verrait
    // sur le fond du bouton).
    QPixmap trophyIcon()
    {
        QPixmap source(HOME_SCREEN_IMAGE_PATH);
        if (source.isNull())
        {
            return QPixmap();
        }
        // Zone du trophee dans l'image native (tuile Tournoi en (896,598)).
        QImage image = source.copy(QRect(896 + 82, 598 + 24, 90, 74))
                           .toImage()
                           .convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y)
        {
            for (int x = 0; x < image.width(); ++x)
            {
                QRgb px = image.pixel(x, y);
                int brightest = qMax(qRed(px), qMax(qGreen(px), qBlue(px)));
                // Tout ce qui est quasi noir (fond du decor, y compris son
                // leger degrade) devient completement transparent.
                int alpha = qBound(0, (brightest - 45) * 5, 255);
                image.setPixel(x, y, qRgba(qRed(px), qGreen(px), qBlue(px), alpha));
            }
        }
        return QPixmap::fromImage(image);
    }

    // Icone du championnat : petit podium (1er au centre, 2e a gauche, 3e a
    // droite), dessine a la main -- aucune image fournie pour celui-ci.
    QPixmap podiumIcon()
    {
        QPixmap pixmap(kIconWidth, kIconHeight);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        struct Step { QRect rect; QColor color; const char* label; };
        const Step steps[] = {
            { QRect(6, 38, 26, 34),  QColor(176, 182, 192), "2" },
            { QRect(35, 18, 26, 54), QColor(230, 188, 0),   "1" },
            { QRect(64, 48, 26, 24), QColor(184, 115, 51),  "3" },
        };

        QFont font;
        font.setBold(true);
        font.setPixelSize(16);
        painter.setFont(font);
        for (const Step& step : steps)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(step.color);
            painter.drawRoundedRect(step.rect, 3, 3);
            painter.setPen(QColor(20, 20, 20));
            painter.drawText(step.rect, Qt::AlignHCenter | Qt::AlignTop, step.label);
        }
        return pixmap;
    }
}

CompetitionChoiceDialog::CompetitionChoiceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Championnat et Tournoi");
    applyDarkTitleBar(this);
    setStyleSheet("background-color: " + kBg + ";");
    resize(560, 320);

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(20);

    QLabel* title = new QLabel("Choisissez une competition", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color: " + kWhite + "; font-size: 15px; font-weight: bold;");
    outer->addWidget(title);

    QHBoxLayout* tilesRow = new QHBoxLayout();
    tilesRow->setSpacing(20);
    outer->addLayout(tilesRow, 1);

    auto makeTile = [&](const QPixmap& icon, const QString& name, const QString& nameColor,
                        const QString& description) -> QPushButton*
    {
        QPushButton* button = new QPushButton(this);
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumSize(220, 200);
        button->setStyleSheet(
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 10px;"
            "}"
            "QPushButton:hover { border-color: " + kGreen + "; }"
        );

        QVBoxLayout* layout = new QVBoxLayout(button);
        layout->setContentsMargins(12, 14, 12, 14);
        layout->setSpacing(8);

        QLabel* iconLabel = new QLabel(button);
        iconLabel->setPixmap(icon);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("background: transparent; border: none;");
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(iconLabel, 1);

        QLabel* nameLabel = new QLabel(name, button);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setStyleSheet(
            "background: transparent; border: none; color: " + nameColor + ";"
            "font-size: 17px; font-weight: bold;");
        nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(nameLabel);

        QLabel* descriptionLabel = new QLabel(description, button);
        descriptionLabel->setAlignment(Qt::AlignCenter);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setStyleSheet(
            "background: transparent; border: none; color: " + kGray + "; font-size: 12px;");
        descriptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(descriptionLabel);

        return button;
    };

    QPushButton* championshipTile = makeTile(podiumIcon(), "CHAMPIONNAT", kBlue,
        "Matchs aller-retour");
    QPushButton* tournamentTile = makeTile(trophyIcon(), "TOURNOI", kGold,
        "Creer ou gerer un tournoi");

    tilesRow->addWidget(championshipTile);
    tilesRow->addWidget(tournamentTile);

    connect(championshipTile, &QPushButton::clicked, this, [this]()
        {
            accept();
            emit championshipRequested();
        });
    connect(tournamentTile, &QPushButton::clicked, this, [this]()
        {
            accept();
            emit tournamentRequested();
        });

    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px; }"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    outer->addWidget(closeButton, 0, Qt::AlignRight);
}
