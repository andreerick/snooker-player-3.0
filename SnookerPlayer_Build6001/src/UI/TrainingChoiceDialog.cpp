#include "TrainingChoiceDialog.h"
#include "UiUtils.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileInfo>
#include <QResizeEvent>

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

    // Un simple QLabel avec un pixmap fixe ne remplit jamais tout l'espace
    // que le bouton/la mise en page lui accorde (son sizeHint reste la
    // taille du pixmap passe a la construction) : c'est ce qui laissait
    // les logos petits et decentres malgre une tuile plus grande. Ce label
    // garde l'image source en pleine resolution et la redimensionne (en
    // conservant les proportions) a CHAQUE redimensionnement reel du
    // label, pour toujours occuper le maximum d'espace disponible.
    class AspectFitLabel : public QLabel
    {
    public:
        explicit AspectFitLabel(QWidget* parent = nullptr) : QLabel(parent) {}

        void setSourcePixmap(const QPixmap& pixmap)
        {
            m_source = pixmap;
            updateScaledPixmap();
        }

    protected:
        void resizeEvent(QResizeEvent* event) override
        {
            QLabel::resizeEvent(event);
            updateScaledPixmap();
        }

    private:
        void updateScaledPixmap()
        {
            if (m_source.isNull() || size().isEmpty())
            {
                return;
            }
            QLabel::setPixmap(m_source.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        QPixmap m_source;
    };
}

TrainingChoiceDialog::TrainingChoiceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Entrainement");
    applyDarkTitleBar(this);
    setStyleSheet("background-color: " + kBg + ";");
    resize(520, 300);

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(20);

    QLabel* title = new QLabel("Choisissez un mode", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color: " + kWhite + "; font-size: 15px; font-weight: bold;");
    outer->addWidget(title);

    QHBoxLayout* tilesRow = new QHBoxLayout();
    tilesRow->setSpacing(20);
    outer->addLayout(tilesRow, 1);

    auto makeTile = [&](const QPixmap& icon, const QString& text) -> QPushButton*
    {
        QPushButton* button = new QPushButton(this);
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumSize(200, 180);
        button->setStyleSheet(
            "QPushButton {"
            "  background-color: " + kPanel + ";"
            "  border: 1px solid " + kBorder + ";"
            "  border-radius: 10px;"
            "}"
            "QPushButton:hover { border-color: " + kGreen + "; }"
        );

        QVBoxLayout* layout = new QVBoxLayout(button);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(10);

        AspectFitLabel* iconLabel = new AspectFitLabel(button);
        iconLabel->setSourcePixmap(icon);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("background: transparent; border: none;");
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(iconLabel, 1);

        if (!text.isEmpty())
        {
            QLabel* textLabel = new QLabel(text, button);
            textLabel->setAlignment(Qt::AlignCenter);
            textLabel->setStyleSheet(
                "background: transparent; border: none; color: " + kWhite + ";"
                "font-size: 14px; font-weight: bold;"
            );
            textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            layout->addWidget(textLabel);
        }

        return button;
    };

    // Les deux logos n'ont aucun texte de bouton a cote (voir QString()
    // plus bas, chacun integre deja le sien) : voir AspectFitLabel plus
    // haut, qui les redimensionne pour remplir tout l'espace reellement
    // accorde par la mise en page (pas une taille fixe devinee a la main).
    QPixmap cueSenseLogo(QFileInfo(HOME_SCREEN_IMAGE_PATH).absolutePath() + "/cuesense_logo_transparent.png");
    QPushButton* cueSenseTile = makeTile(cueSenseLogo, QString());

    // Icone "Exercice guides" fournie par l'utilisateur (deja son propre
    // texte "Exercices guides" integre a l'image, comme le logo CueSense
    // ci-dessus) -- remplace l'icone vectorielle provisoire (queues
    // croisees, "on verra apres" a l'epoque).
    QPixmap exerciseIcon(QFileInfo(HOME_SCREEN_IMAGE_PATH).absolutePath() + "/exercice_guides_icon.png");
    QPushButton* exerciseTile = makeTile(exerciseIcon, QString());

    tilesRow->addWidget(cueSenseTile);
    tilesRow->addWidget(exerciseTile);

    connect(cueSenseTile, &QPushButton::clicked, this, [this]()
        {
            emit cueSenseRequested();
            accept();
        });
    connect(exerciseTile, &QPushButton::clicked, this, [this]()
        {
            emit exerciseRequested();
            accept();
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
