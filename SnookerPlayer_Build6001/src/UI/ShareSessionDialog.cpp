#include "ShareSessionDialog.h"
#include "MatchWebServer.h"

#include "qrcodegen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QImage>
#include <QPixmap>
#include <QNetworkInterface>
#include <QGuiApplication>
#include <QClipboard>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";

    // Convertit un qrcodegen::QrCode en QPixmap noir/blanc agrandi
    // (chaque module du QR code devient un carre de `scale` pixels), avec
    // une marge blanche autour (quiet zone, necessaire pour que les
    // lecteurs de QR code arrivent a le decoder correctement).
    QPixmap qrCodeToPixmap(const qrcodegen::QrCode& qr, int scale)
    {
        const int margin = 4; // modules de marge, recommandation standard QR
        int size = qr.getSize();
        int imageSize = (size + margin * 2) * scale;

        QImage image(imageSize, imageSize, QImage::Format_RGB32);
        image.fill(Qt::white);

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                if (!qr.getModule(x, y))
                {
                    continue;
                }
                int px = (x + margin) * scale;
                int py = (y + margin) * scale;
                for (int dy = 0; dy < scale; ++dy)
                {
                    for (int dx = 0; dx < scale; ++dx)
                    {
                        image.setPixel(px + dx, py + dy, qRgb(0, 0, 0));
                    }
                }
            }
        }
        return QPixmap::fromImage(image);
    }
}

QString detectLocalLanAddress()
{
    // Cherche une adresse IPv4 privee (192.168.x.x, 10.x.x.x ou
    // 172.16-31.x.x), pas la boucle locale : c'est l'adresse a laquelle
    // un telephone sur le meme Wi-Fi peut joindre ce PC. Simple
    // heuristique (premiere trouvee) : suffisant pour l'usage vise (un
    // seul reseau actif, celui du club/de la maison), pas une detection
    // robuste multi-interfaces (VPN, partage de connexion, etc.).
    const auto addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& addr : addresses)
    {
        if (addr.protocol() != QAbstractSocket::IPv4Protocol)
        {
            continue;
        }
        if (addr.isLoopback())
        {
            continue;
        }
        QString s = addr.toString();
        if (s.startsWith("192.168.") || s.startsWith("10.") ||
            (s.startsWith("172.") && s.section('.', 1, 1).toInt() >= 16 && s.section('.', 1, 1).toInt() <= 31))
        {
            return s;
        }
    }
    return QString();
}

ShareSessionDialog::ShareSessionDialog(MatchWebServer* server, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Partager le suivi du match");
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* title = new QLabel("Scannez pour suivre le match en direct", this);
    title->setAlignment(Qt::AlignHCenter);
    title->setStyleSheet("font-size: 15px; font-weight: bold; color: " + kWhite + ";");
    layout->addWidget(title);

    QString lanAddress = detectLocalLanAddress();
    QLabel* qrLabel = new QLabel(this);
    qrLabel->setAlignment(Qt::AlignHCenter);
    QLineEdit* urlBox = new QLineEdit(this);
    urlBox->setReadOnly(true);
    urlBox->setAlignment(Qt::AlignHCenter);
    urlBox->setStyleSheet(
        "QLineEdit { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px; font-size: 12px; }"
    );

    if (lanAddress.isEmpty())
    {
        QLabel* warn = new QLabel(
            "Impossible de detecter l'adresse Wi-Fi locale. Verifiez que le PC "
            "est bien connecte a un reseau Wi-Fi (pas seulement Ethernet/VPN).",
            this
        );
        warn->setWordWrap(true);
        warn->setStyleSheet("color: #e74c3c; font-size: 12px;");
        layout->addWidget(warn);
    }
    else
    {
        QString url = "http://" + lanAddress + ":" + QString::number(server->port()) + server->pagePath();

        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(url.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        qrLabel->setPixmap(qrCodeToPixmap(qr, 8));

        urlBox->setText(url);

        layout->addWidget(qrLabel);
        layout->addWidget(urlBox);

        QPushButton* copyButton = new QPushButton("Copier le lien", this);
        copyButton->setStyleSheet(
            "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
            "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 6px 12px; }"
            "QPushButton:hover { border-color: " + kGreen + "; }"
        );
        connect(copyButton, &QPushButton::clicked, this, [url]()
            {
                QGuiApplication::clipboard()->setText(url);
            });
        layout->addWidget(copyButton, 0, Qt::AlignHCenter);
    }

    QLabel* hint = new QLabel(
        "Le telephone doit etre connecte au MEME reseau Wi-Fi que ce PC. "
        "Aucune connexion internet n'est necessaire.",
        this
    );
    hint->setWordWrap(true);
    hint->setAlignment(Qt::AlignHCenter);
    hint->setStyleSheet("color: " + kGray + "; font-size: 11px; margin-top: 8px;");
    layout->addWidget(hint);

    // Pas de bouton "Arreter le partage" : le lien (jeton + port
    // persistes, voir MatchWebServer) est cense rester valable en
    // permanence pour cette table (QR code imprime une seule fois), donc
    // rien ici ne doit pouvoir le casser tant que l'appli tourne.
    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px; }"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignHCenter);
}
