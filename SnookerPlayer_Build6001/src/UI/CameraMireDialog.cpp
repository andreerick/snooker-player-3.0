#include "CameraMireDialog.h"
#include "UiUtils.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <opencv2/imgproc.hpp>

// La detection des marqueurs ArUco (module objdetect) n'est pas compilee sous
// Windows : en Debug, objdetect charge la bibliotheque protobuf de debug, qui
// declenche une assertion au demarrage et empeche SnookerPlayer.exe de
// se lancer. Elle est disponible sur Mac (la ou se font les vrais tests).
#ifndef Q_OS_WIN
#include <opencv2/objdetect.hpp>
#define MIRE_HAS_ARUCO 1
#endif

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace
{
    const QString kBg = "#000000";
    const QString kPanel = "#111316";
    const QString kBorder = "#2a2d31";
    const QString kGreen = "#1a9000";
    const QString kGray = "#7a7f87";
    const QString kWhite = "#f5f5f5";

    const double kTableWidthCm = 178.0;

    QString settingsFilePath()
    {
        return QCoreApplication::applicationDirPath() + "/settings.ini";
    }

    // Marqueurs attendus par camera (voir ArucoCalibrationTool.cpp) et
    // longueur de la zone couverte le long de la table (cm).
    std::vector<int> expectedMarkerIds(int cameraIndex)
    {
        switch (cameraIndex)
        {
        case 0: return { 1, 2, 3, 4 };
        case 1: return { 5, 6 };
        default: return { 7, 8, 9, 10 };
        }
    }

    double zoneLengthCm(int cameraIndex)
    {
        return cameraIndex == 1 ? 97.0 : 130.0;
    }

    QString checkStyle()
    {
        return "QCheckBox { color: " + kWhite + "; background: transparent; }";
    }
}

CameraMireDialog::CameraMireDialog(int cameraIndex, QWidget* parent)
    : QDialog(parent)
    , m_cameraIndex(cameraIndex)
{
    setWindowTitle("Mire de cadrage - camera " + QString::number(cameraIndex));
    applyDarkTitleBar(this);
    resize(1000, 700);
    setStyleSheet("background-color: " + kBg + "; color: " + kWhite + ";");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    m_video = new QLabel(this);
    m_video->setAlignment(Qt::AlignCenter);
    m_video->setMinimumSize(800, 450);
    m_video->setStyleSheet("background-color: " + kPanel + "; border: 1px solid " + kBorder + ";");
    root->addWidget(m_video, 1);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setStyleSheet("color: " + kWhite + "; font-size: 12px; background: transparent;");
    root->addWidget(m_status);

    QSettings settings(settingsFilePath(), QSettings::IniFormat);

    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(14);

    row->addWidget(new QLabel("Hauteur de la camera (cm) :", this));
    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(80, 250);
    m_heightSpin->setValue(settings.value("mire/hauteur", 120).toInt());
    row->addWidget(m_heightSpin);

    row->addWidget(new QLabel("Champ de vision (degres, diagonale) :", this));
    m_fovSpin = new QSpinBox(this);
    m_fovSpin->setRange(60, 140);
    m_fovSpin->setValue(settings.value("mire/fov", 100).toInt());
    row->addWidget(m_fovSpin);
    row->addStretch(1);
    root->addLayout(row);

    QHBoxLayout* options = new QHBoxLayout();
    m_frameBox = new QCheckBox("Cadre de la table", this);
    m_frameBox->setChecked(true);
    m_markersBox = new QCheckBox("Marqueurs", this);
    m_markersBox->setChecked(true);
    for (QCheckBox* box : { m_frameBox, m_markersBox })
    {
        box->setStyleSheet(checkStyle());
        options->addWidget(box);
    }
    options->addStretch(1);

    QHBoxLayout* orientation = new QHBoxLayout();
    orientation->setSpacing(14);
    orientation->addWidget(new QLabel("Orientation de l'image :", this));
    m_orientationBox = new QComboBox(this);
    m_orientationBox->addItems({ "Normale (0 deg)", "Tournee de 90 deg (sens horaire)",
                                 "A l'envers (180 deg)", "Tournee de 90 deg (sens anti-horaire)" });
    int savedOrientation = settings.value("mire/orientation", -1).toInt();
    if (savedOrientation < 0)
    {
        savedOrientation = settings.value("mire/flip", false).toBool() ? 2 : 0; // ancien reglage
    }
    m_orientationBox->setCurrentIndex(std::min(std::max(savedOrientation, 0), 3));
    m_orientationBox->setStyleSheet(
        "QComboBox { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 6px; padding: 4px 8px; }"
        "QComboBox QAbstractItemView { background-color: " + kPanel + "; color: " + kWhite + "; }"
    );
    orientation->addWidget(m_orientationBox);

    orientation->addWidget(new QLabel("Rotation du cadre (deg) :", this));
    m_angleSpin = new QDoubleSpinBox(this);
    m_angleSpin->setRange(-45.0, 45.0);
    m_angleSpin->setSingleStep(0.5);
    m_angleSpin->setDecimals(1);
    m_angleSpin->setValue(0.0);
    orientation->addWidget(m_angleSpin);
    orientation->addStretch(1);
    root->addLayout(orientation);

    // Inclinaison de la camera au-dessus de la table : verticale (0 deg)
    // pour la camera 2, environ 4 deg vers le bout de la table pour 1 et 3.
    QHBoxLayout* tilt = new QHBoxLayout();
    tilt->setSpacing(14);
    tilt->addWidget(new QLabel("Inclinaison de la camera (deg, + = vers le haut de l'image) :", this));
    m_tiltSpin = new QDoubleSpinBox(this);
    m_tiltSpin->setRange(-20.0, 20.0);
    m_tiltSpin->setSingleStep(0.5);
    m_tiltSpin->setDecimals(1);
    const double defaultTilt = (m_cameraIndex == 1) ? 0.0 : 4.0;
    m_tiltSpin->setValue(settings.value(QString("mire/inclinaison_%1").arg(m_cameraIndex), defaultTilt).toDouble());
    tilt->addWidget(m_tiltSpin);
    tilt->addWidget(new QLabel(m_cameraIndex == 1 ? "(camera 2 : verticale, 0 deg)"
                                                  : "(cameras 1 et 3 : environ 4 deg vers le bout de la table)", this));
    tilt->addStretch(1);
    root->addLayout(tilt);

    QPushButton* closeButton = new QPushButton("Fermer", this);
    closeButton->setStyleSheet(
        "QPushButton { background-color: " + kPanel + "; color: " + kWhite + ";"
        "border: 1px solid " + kBorder + "; border-radius: 5px; padding: 8px 16px; }"
        "QPushButton:hover { border-color: " + kGreen + "; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    options->addWidget(closeButton);
    root->addLayout(options);

    // Ouverture en 1280x720 (MJPG) : assez de details pour les marqueurs,
    // et fluide meme sur une liaison USB 2.0.
    if (!m_capture.open(m_cameraIndex))
    {
        m_status->setText("Impossible d'ouvrir la camera " + QString::number(m_cameraIndex) +
                          ". Si un suivi camera est en cours, ou si un autre programme l'utilise, "
                          "fermez-le puis rouvrez la mire.");
        return;
    }
    m_capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    m_timer = new QTimer(this);
    m_timer->setInterval(80);
    connect(m_timer, &QTimer::timeout, this, &CameraMireDialog::tick);
    m_timer->start();
}

CameraMireDialog::~CameraMireDialog()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("mire/hauteur", m_heightSpin->value());
    settings.setValue("mire/fov", m_fovSpin->value());
    settings.setValue("mire/orientation", m_orientationBox->currentIndex());
    settings.setValue(QString("mire/inclinaison_%1").arg(m_cameraIndex), m_tiltSpin->value());
    settings.sync();
    if (m_capture.isOpened())
    {
        m_capture.release();
    }
}

void CameraMireDialog::tick()
{
    cv::Mat frame;
    if (!m_capture.read(frame) || frame.empty())
    {
        return;
    }
    switch (m_orientationBox->currentIndex())
    {
    case 1: cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE); break;
    case 2: cv::rotate(frame, frame, cv::ROTATE_180); break;
    case 3: cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); break;
    default: break;
    }

    drawMire(frame);

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    QImage image(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
    m_video->setPixmap(QPixmap::fromImage(image.copy()).scaled(
        m_video->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CameraMireDialog::drawMire(cv::Mat& frame) const
{
    const int w = frame.cols;
    const int h = frame.rows;
    const cv::Point center(w / 2, h / 2);
    const cv::Scalar white(255, 255, 255);
    const cv::Scalar grey(190, 190, 190);
    const cv::Scalar green(0, 220, 0);
    const cv::Scalar yellow(0, 220, 255);
    const cv::Scalar red(0, 0, 255);

    // Grille des tiers
    for (int i = 1; i <= 2; ++i)
    {
        cv::line(frame, cv::Point(w * i / 3, 0), cv::Point(w * i / 3, h), grey, 1);
        cv::line(frame, cv::Point(0, h * i / 3), cv::Point(w, h * i / 3), grey, 1);
    }

    // Croix + cercles au centre
    cv::line(frame, cv::Point(center.x, 0), cv::Point(center.x, h), white, 1);
    cv::line(frame, cv::Point(0, center.y), cv::Point(w, center.y), white, 1);
    for (int i = 1; i <= 3; ++i)
    {
        cv::circle(frame, center, h * i / 12, white, 1);
    }

    // Echelle : champ vu a la hauteur de la camera (modele simple, sans
    // distorsion de l'objectif : indicatif, a ajuster avec le champ de vision).
    const double diagRad = m_fovSpin->value() * CV_PI / 180.0;
    const double diagPx = std::sqrt(static_cast<double>(w) * w + static_cast<double>(h) * h);
    const double tanHalfH = std::tan(diagRad / 2.0) * w / diagPx;
    const double fieldWidthCm = 2.0 * m_heightSpin->value() * tanHalfH;
    const double pxPerCm = w / fieldWidthCm;

    QString frameText;
    if (m_frameBox->isChecked())
    {
        // Cadre vu par une camera a la hauteur H, inclinee de theta autour de
        // l'axe horizontal de l'image (modele sans distorsion), et dont l'axe
        // optique vise le centre de la zone. Theta = 0 : simple rectangle.
        const double zoneL = zoneLengthCm(m_cameraIndex);
        const double camH = m_heightSpin->value();
        const double theta = m_tiltSpin->value() * CV_PI / 180.0;
        const double focal = pxPerCm * camH;
        const double y0 = camH * std::tan(theta);
        // Rotation libre du cadre autour du centre de l'image (orientation).
        const double angle = m_angleSpin->value() * CV_PI / 180.0;
        const double ca = std::cos(angle);
        const double sa = std::sin(angle);
        const double cornersX[4] = { -kTableWidthCm / 2.0, kTableWidthCm / 2.0, kTableWidthCm / 2.0, -kTableWidthCm / 2.0 };
        const double cornersY[4] = { zoneL / 2.0, zoneL / 2.0, -zoneL / 2.0, -zoneL / 2.0 }; // haut puis bas de l'image
        std::vector<cv::Point> poly;
        bool fits = true;
        double topWidthPx = 0.0;
        double bottomWidthPx = 0.0;
        std::vector<cv::Point2d> pts;
        for (int i = 0; i < 4; ++i)
        {
            const double gy = y0 + cornersY[i];
            const double depth = gy * std::sin(theta) + camH * std::cos(theta);
            const double xc = cornersX[i];
            const double yc = gy * std::cos(theta) - camH * std::sin(theta);
            const double u = depth > 1.0 ? focal * xc / depth : 0.0;
            const double v = depth > 1.0 ? focal * yc / depth : 0.0;
            const double px = u * ca - (-v) * sa; // v vers le haut -> y image = -v
            const double py = u * sa + (-v) * ca;
            pts.emplace_back(center.x + px, center.y + py);
            poly.emplace_back(static_cast<int>(center.x + px), static_cast<int>(center.y + py));
            if (center.x + px < 0 || center.x + px > w || center.y + py < 0 || center.y + py > h)
            {
                fits = false;
            }
        }
        topWidthPx = std::hypot(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
        bottomWidthPx = std::hypot(pts[2].x - pts[3].x, pts[2].y - pts[3].y);
        const cv::Scalar color = fits ? green : red;
        cv::polylines(frame, poly, true, color, 2);
        cv::putText(frame, "largeur de la table (178 cm)", cv::Point(std::max(poly[0].x, 10), std::max(poly[0].y - 8, 20)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        frameText = QString("Cadre : %1 x %2 cm (1 cm = %3 px)%4")
                        .arg(kTableWidthCm, 0, 'f', 0)
                        .arg(zoneLengthCm(m_cameraIndex), 0, 'f', 0)
                        .arg(pxPerCm, 0, 'f', 1)
                        .arg(fits ? "" : " - trop grand pour l'image : monter la camera");
        if (std::abs(m_tiltSpin->value()) > 0.01 && bottomWidthPx > 1.0)
        {
            frameText += QString(" | inclinaison : %1 deg (bord haut = %2 % du bord bas)")
                             .arg(m_tiltSpin->value(), 0, 'f', 1)
                             .arg(100.0 * topWidthPx / bottomWidthPx, 0, 'f', 0);
        }
        if (std::abs(m_angleSpin->value()) > 0.01)
        {
            frameText += QString(" | rotation du cadre : %1 deg (tourner la camera de cet angle)")
                             .arg(m_angleSpin->value(), 0, 'f', 1);
        }
    }

    // Marqueurs ArUco
    QString markerText;
#ifndef MIRE_HAS_ARUCO
    markerText = "Detection des marqueurs : disponible sur Mac";
#else
    if (m_markersBox->isChecked())
    {
        cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
        cv::aruco::DetectorParameters params;
        cv::aruco::ArucoDetector detector(dictionary, params);

        std::vector<std::vector<cv::Point2f>> corners;
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> rejected;
        detector.detectMarkers(frame, corners, ids, rejected);

        const std::vector<int> expected = expectedMarkerIds(m_cameraIndex);
        std::map<int, bool> seen;
        for (size_t i = 0; i < ids.size(); ++i)
        {
            const bool isExpected = std::find(expected.begin(), expected.end(), ids[i]) != expected.end();
            seen[ids[i]] = true;
            std::vector<cv::Point> poly;
            for (const auto& p : corners[i])
            {
                poly.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));
            }
            cv::polylines(frame, poly, true, isExpected ? green : yellow, 2);
            cv::Point2f c(0.f, 0.f);
            for (const auto& p : corners[i]) c += p;
            c *= 0.25f;
            cv::putText(frame, std::to_string(ids[i]), cv::Point(static_cast<int>(c.x) + 8, static_cast<int>(c.y) - 8),
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, isExpected ? green : yellow, 2);
        }

        QStringList parts;
        for (int id : expected)
        {
            parts << QString("%1 %2").arg(id).arg(seen.count(id) ? "OK" : "absent");
        }
        markerText = "Marqueurs attendus : " + parts.join(", ");
    }
#endif

    // Texte d'etat (mis a jour sans toucher au const : via const_cast sur le widget)
    const_cast<CameraMireDialog*>(this)->m_status->setText(
        "Camera " + QString::number(m_cameraIndex) + " - hauteur " + QString::number(m_heightSpin->value()) +
        " cm.  " + frameText + (markerText.isEmpty() ? "" : "   |   " + markerText) +
        "\nLes deux grands rails de la table doivent etre paralleles aux bords gauche et droit du cadre vert, "
        "et la table centree sur la croix.");
}
