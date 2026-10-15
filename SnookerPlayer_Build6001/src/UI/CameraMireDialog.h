#pragma once

#include <QDialog>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>

class QLabel;
class QTimer;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;

// =====================================================================
// CameraMireDialog
// ---------------------------------------------------------------------
// Apercu en direct d'une camera avec une MIRE de cadrage, pour verifier
// son positionnement au-dessus de la table AVANT de coller les marqueurs
// ou de calibrer :
//   - croix + cercles au centre de l'image ;
//   - grille des tiers ;
//   - cadre vert = ou doivent tomber les bords de la table (178 cm de
//     large) et la longueur de la zone de cette camera, calcule d'apres
//     la hauteur de la camera et son champ de vision (reglables) ;
//   - INCLINAISON : le cadre est dessine comme la camera le verrait si elle
//     etait inclinee de N degres (cameras 1 et 3 : environ 4 deg vers le
//     bout de la table ; camera 2 : verticale, 0 deg). On regle N jusqu'a
//     ce que le cadre colle aux bords reels de la table ;
//   - ORIENTATION : l'image peut etre tournee de 0/90/180/270 degres
//     (camera montee de travers ou a l'envers) et le cadre peut etre
//     tourne d'un angle libre pour mesurer de combien la camera est
//     tournee par rapport aux rails de la table ;
//   - marqueurs ArUco reconnus, avec leur numero (ceux attendus pour
//     cette camera en vert).
// Utilisable sans marqueurs ni calibration. Ouvre la camera en direct :
// si un suivi camera est deja en cours, la camera peut etre occupee.
// =====================================================================
class CameraMireDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CameraMireDialog(int cameraIndex, QWidget* parent = nullptr);
    ~CameraMireDialog() override;

private:
    void tick();
    void drawMire(cv::Mat& frame) const;

    int m_cameraIndex;
    cv::VideoCapture m_capture;
    QTimer* m_timer = nullptr;
    QLabel* m_video = nullptr;
    QLabel* m_status = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QSpinBox* m_fovSpin = nullptr;
    QComboBox* m_orientationBox = nullptr;
    QDoubleSpinBox* m_angleSpin = nullptr;
    QDoubleSpinBox* m_tiltSpin = nullptr;
    QCheckBox* m_markersBox = nullptr;
    QCheckBox* m_frameBox = nullptr;
    QString m_markerText;
};
