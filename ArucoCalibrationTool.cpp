#include "CameraCalibration.h"
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

// =====================================================================
// Outil de calibration AUTOMATIQUE par marqueurs ArUco
// ---------------------------------------------------------------------
// Version a 10 marqueurs, calquee sur manuel_calibration_cameras.odt
// (mesures reelles confirmees a l'installation) :
//   - Camera 1 (index 0, jaune/verte/marron) et Camera 3 (index 2,
//     rouges/rose/noire) sont inclinees ~4 deg vers l'exterieur ->
//     4 marqueurs chacune, homographie complete a 4 points (calibrate()).
//   - Camera 2 (index 1, bleue, centre table) reste verticale ->
//     2 marqueurs, transformation simplifiee a 2 points
//     (calibrateFromTwoPoints()), suffisante vu l'absence de perspective.
//
// Distances reelles mesurees : camera 2 <-> camera 1 = 97cm, camera 2 <->
// camera 3 = 97cm. Zones le long des 357cm de la table : camera 1 =
// 0->130cm, camera 2 = 130->227cm (centree), camera 3 = 227->357cm.
//
// UTILISATION :
//   1. Génère les 10 marqueurs :  ArucoCalibrationTool.exe --generate
//   2. Colle-les sur le bord EXTERIEUR de la table (voir manuel)
//   3. Teste le positionnement (avant de calibrer) :
//        ArucoCalibrationTool.exe --check <index_camera 0/1/2> <image_ou_flux>
//   4. Calibre une camera (definitif) :
//        ArucoCalibrationTool.exe <index_camera 0/1/2> <image_ou_flux>
// =====================================================================

struct MarkerInfo { int id; std::string description; };

const std::vector<MarkerInfo> ALL_MARKERS = {
    { 1,  "Camera 1 (jaune/verte/marron) - rebord HAUT, cote baulk" },
    { 2,  "Camera 1 (jaune/verte/marron) - rebord HAUT, cote camera 2" },
    { 3,  "Camera 1 (jaune/verte/marron) - rebord BAS, cote baulk" },
    { 4,  "Camera 1 (jaune/verte/marron) - rebord BAS, cote camera 2" },
    { 5,  "Camera 2 (bleue, centre) - rebord HAUT" },
    { 6,  "Camera 2 (bleue, centre) - rebord BAS" },
    { 7,  "Camera 3 (rouges/rose/noire) - rebord HAUT, cote camera 2" },
    { 8,  "Camera 3 (rouges/rose/noire) - rebord HAUT, cote noire" },
    { 9,  "Camera 3 (rouges/rose/noire) - rebord BAS, cote camera 2" },
    { 10, "Camera 3 (rouges/rose/noire) - rebord BAS, cote noire" },
};

// Specification par camera : quels marqueurs elle utilise (dans l'ordre
// haut-gauche/haut-droit/bas-droit/bas-gauche pour une camera inclinee,
// ou haut/bas pour la camera verticale) et leurs points reels (cm)
// correspondants sur la table.
struct CameraMarkerSpec
{
    bool tilted = false; // true -> 4 marqueurs / homographie, false -> 2 marqueurs / 2 points
    std::vector<int> ids;
    std::vector<cv::Point2f> tableCm;
};

CameraMarkerSpec getCameraMarkerSpec(int cameraIndex)
{
    const float W = 178.0f; // largeur reelle de la table (cm)
    CameraMarkerSpec spec;
    switch (cameraIndex)
    {
    case 0: // Camera 1, jaune/verte/marron, inclinee ~4 deg, zone 0->130cm
        spec.tilted = true;
        spec.ids = { 1, 2, 4, 3 };
        spec.tableCm = { {0.f, 0.f}, {130.f, 0.f}, {130.f, W}, {0.f, W} };
        break;
    case 1: // Camera 2, bleue/centre, verticale, zone 130->227cm (centree sur 178.5)
        spec.tilted = false;
        spec.ids = { 5, 6 };
        spec.tableCm = { {178.5f, 0.f}, {178.5f, W} };
        break;
    case 2: // Camera 3, rouges/rose/noire, inclinee ~4 deg, zone 227->357cm
        spec.tilted = true;
        spec.ids = { 7, 8, 10, 9 };
        spec.tableCm = { {227.f, 0.f}, {357.f, 0.f}, {357.f, W}, {227.f, W} };
        break;
    }
    return spec;
}

// Position reelle de la bille bleue (centre exact de la table, section 2 du manuel).
const cv::Point2f BLUE_BALL_CM(178.5f, 89.0f);

// Centre reel (cm) de la zone couverte par une camera = moyenne de ses points de reference.
// Pour une camera bien inclinee/positionnee, c'est le point que son champ de vision doit
// cadrer au milieu de l'image (voir "mire" dans runPositionCheck()).
cv::Point2f zoneCenterCm(const CameraMarkerSpec& spec)
{
    cv::Point2f c(0.f, 0.f);
    for (const auto& p : spec.tableCm) c += p;
    c *= (1.0f / static_cast<float>(spec.tableCm.size()));
    return c;
}

void generateMarkers()
{
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

    for (const auto& m : ALL_MARKERS)
    {
        cv::Mat markerImage;
        cv::aruco::generateImageMarker(dictionary, m.id, 400, markerImage);
        std::string filename = "marker_" + std::to_string(m.id) + ".png";
        cv::imwrite(filename, markerImage);
        std::cout << "Marqueur genere : " << filename << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Colle ces 10 marqueurs sur le bord EXTERIEUR de la table (chassis), voir le manuel :" << std::endl;
    for (const auto& m : ALL_MARKERS)
    {
        std::cout << "  marker_" << m.id << ".png -> " << m.description << std::endl;
    }
}

// =====================================================================
// Test de positionnement (AVANT calibration) : verifie en direct que la
// camera est bien a plat au-dessus de sa zone, sans encore calculer ni
// sauvegarder d'homographie. Deux indices, tires de la geometrie des
// marqueurs ArUco deja utilises pour la calibration :
//   - "carre" : un marqueur physique est carre ; vu depuis une camera
//     bien perpendiculaire au tapis, il reste (a peu pres) carre a
//     l'image. Une camera inclinee le deforme en trapeze.
//   - "tailles egales" : les 2 marqueurs (haut/bas) sont sur le meme
//     plan (le tapis) a la meme distance de la camera si elle est bien
//     centree et a plat ; des tailles tres differentes trahissent un
//     angle ou un decentrage.
// =====================================================================

struct MarkerGeometry
{
    cv::Point2f center{ 0.f, 0.f };
    float avgSideLen = 0.f;
    float squareness = 0.f;      // 1.0 = carre parfait, plus petit = trapeze
    float maxAngleDeviation = 0.f; // ecart max a 90 degres, sur les 4 coins
};

MarkerGeometry analyzeMarkerGeometry(const std::vector<cv::Point2f>& c)
{
    // c[0]=haut-gauche, c[1]=haut-droit, c[2]=bas-droit, c[3]=bas-gauche (convention ArUco)
    float top = static_cast<float>(cv::norm(c[1] - c[0]));
    float right = static_cast<float>(cv::norm(c[2] - c[1]));
    float bottom = static_cast<float>(cv::norm(c[2] - c[3]));
    float left = static_cast<float>(cv::norm(c[3] - c[0]));

    float ratioH = std::min(top, bottom) / std::max(top, bottom);
    float ratioV = std::min(left, right) / std::max(left, right);

    auto angleAtCorner = [](cv::Point2f prev, cv::Point2f curr, cv::Point2f next) {
        cv::Point2f v1 = prev - curr, v2 = next - curr;
        float dot = v1.x * v2.x + v1.y * v2.y;
        float n1 = static_cast<float>(cv::norm(v1)), n2 = static_cast<float>(cv::norm(v2));
        float cosA = dot / (n1 * n2);
        cosA = std::max(-1.0f, std::min(1.0f, cosA));
        return std::acos(cosA) * 180.0f / static_cast<float>(CV_PI);
    };
    float aTL = angleAtCorner(c[3], c[0], c[1]);
    float aTR = angleAtCorner(c[0], c[1], c[2]);
    float aBR = angleAtCorner(c[1], c[2], c[3]);
    float aBL = angleAtCorner(c[2], c[3], c[0]);

    MarkerGeometry g;
    for (const auto& p : c) g.center += p;
    g.center *= 0.25f;
    g.avgSideLen = (top + right + bottom + left) / 4.0f;
    g.squareness = std::min(ratioH, ratioV);
    g.maxAngleDeviation = std::max({ std::abs(aTL - 90.f), std::abs(aTR - 90.f), std::abs(aBR - 90.f), std::abs(aBL - 90.f) });
    return g;
}

void runPositionCheck(int cameraIndex, const std::string& source)
{
    const float SQUARENESS_MIN = 0.85f;   // en dessous : trapeze trop marque -> camera inclinee
    const float ANGLE_DEV_MAX = 15.0f;    // degres d'ecart a 90 tolere sur un coin
    const float SIZE_RATIO_MIN = 0.85f;   // rapport min entre les 2 marqueurs haut/bas (camera verticale uniquement)
    const float TABLE_WIDTH_CM = 178.0f;  // distance reelle entre les 2 marqueurs haut/bas de la camera verticale
    const float ZONE_LENGTH_CM = 130.0f;  // distance reelle entre les 2 marqueurs du rebord HAUT (cameras inclinees)
    const float OFFSET_MAX_CM = 5.0f;     // decalage tolere entre le centre image et la cible (mire)

    auto drawCrosshair = [](cv::Mat& img, cv::Point2f c, int size, const cv::Scalar& color) {
        cv::Point pc(static_cast<int>(c.x), static_cast<int>(c.y));
        cv::line(img, cv::Point(pc.x - size, pc.y), cv::Point(pc.x + size, pc.y), color, 1);
        cv::line(img, cv::Point(pc.x, pc.y - size), cv::Point(pc.x, pc.y + size), color, 1);
        cv::circle(img, pc, size / 3, color, 1);
    };

    CameraMarkerSpec spec = getCameraMarkerSpec(cameraIndex);

    bool isNumber = !source.empty() && std::all_of(source.begin(), source.end(), ::isdigit);
    cv::VideoCapture cap;
    cv::Mat staticFrame;
    if (isNumber)
    {
        cap.open(std::stoi(source));
        if (!cap.isOpened())
        {
            std::cout << "Impossible d'ouvrir la camera " << source << std::endl;
            return;
        }
    }
    else
    {
        staticFrame = cv::imread(source);
        if (staticFrame.empty())
        {
            std::cout << "Impossible de charger l'image : " << source << std::endl;
            return;
        }
    }

    cv::Point2f target = zoneCenterCm(spec);
    float distToBlueCm = static_cast<float>(cv::norm(target - BLUE_BALL_CM));

    std::cout << "=====================================================" << std::endl;
    std::cout << "Test de positionnement camera " << cameraIndex << " (avant calibration)" << std::endl;
    std::cout << "Marqueurs attendus : ";
    for (size_t i = 0; i < spec.ids.size(); i++) std::cout << "ID " << spec.ids[i] << (i + 1 < spec.ids.size() ? ", " : "");
    std::cout << std::endl;
    if (spec.tilted)
    {
        std::cout << "Camera inclinee par design (~4 deg) : la mire doit cadrer le centre de SA zone," << std::endl;
        std::cout << "situe a " << distToBlueCm << " cm de la bille bleue (pas la bille bleue elle-meme)." << std::endl;
        std::cout << "La forme (carre) de chaque marqueur individuel reste indicative, pas bloquante." << std::endl;
    }
    std::cout << "Appuie sur 'q' ou Echap pour quitter." << std::endl;
    std::cout << "=====================================================" << std::endl;

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);

    while (true)
    {
        cv::Mat frame;
        if (isNumber) { cap >> frame; if (frame.empty()) break; }
        else { frame = staticFrame.clone(); }

        std::vector<std::vector<cv::Point2f>> corners;
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> rejected;
        detector.detectMarkers(frame, corners, ids, rejected);

        std::map<int, std::vector<cv::Point2f>> byId;
        for (size_t i = 0; i < ids.size(); i++) byId[ids[i]] = corners[i];

        bool allPresent = std::all_of(spec.ids.begin(), spec.ids.end(), [&](int id) { return byId.count(id) > 0; });

        std::string statusText;
        cv::Scalar statusColor(0, 0, 255); // rouge par defaut

        cv::Point2f imgCenter(frame.cols / 2.0f, frame.rows / 2.0f);
        drawCrosshair(frame, imgCenter, 20, cv::Scalar(255, 255, 255));

        if (!allPresent)
        {
            statusText = "Marqueur(s) manquant(s) - attendus : ";
            for (size_t i = 0; i < spec.ids.size(); i++)
            {
                statusText += "ID " + std::to_string(spec.ids[i]) + (byId.count(spec.ids[i]) ? " OK" : " ABSENT");
                if (i + 1 < spec.ids.size()) statusText += ", ";
            }
        }
        else if (!spec.tilted)
        {
            // Camera verticale (bille bleue) : verification complete, y compris le centrage.
            MarkerGeometry gHaut = analyzeMarkerGeometry(byId[spec.ids[0]]);
            MarkerGeometry gBas = analyzeMarkerGeometry(byId[spec.ids[1]]);
            cv::line(frame, gHaut.center, gBas.center, cv::Scalar(255, 200, 0), 1);

            float sizeRatio = std::min(gHaut.avgSideLen, gBas.avgSideLen) / std::max(gHaut.avgSideLen, gBas.avgSideLen);
            bool squareOk = gHaut.squareness >= SQUARENESS_MIN && gBas.squareness >= SQUARENESS_MIN;
            bool angleOk = gHaut.maxAngleDeviation <= ANGLE_DEV_MAX && gBas.maxAngleDeviation <= ANGLE_DEV_MAX;
            bool sizeOk = sizeRatio >= SIZE_RATIO_MIN;

            // Mire au centre exact de l'image : le milieu haut/bas (donc le point vise,
            // la bille bleue) doit tomber dessus si la camera est bien A-PLOMB au-dessus
            // de sa zone (pas juste a plat, mais aussi bien centree).
            cv::Point2f midpoint = (gHaut.center + gBas.center) * 0.5f;
            float markerDistPx = static_cast<float>(cv::norm(gHaut.center - gBas.center));
            float pxPerCm = markerDistPx / TABLE_WIDTH_CM;
            float offsetPx = static_cast<float>(cv::norm(midpoint - imgCenter));
            float offsetCm = pxPerCm > 1e-3f ? offsetPx / pxPerCm : std::numeric_limits<float>::infinity();
            bool centeredOk = std::isfinite(offsetCm) && offsetCm <= OFFSET_MAX_CM;

            cv::Scalar centerColor = centeredOk ? cv::Scalar(0, 200, 0) : cv::Scalar(0, 0, 255);
            cv::line(frame, imgCenter, midpoint, centerColor, 1);
            cv::circle(frame, cv::Point(static_cast<int>(midpoint.x), static_cast<int>(midpoint.y)), 5, centerColor, -1);

            char buf[256];
            std::snprintf(buf, sizeof(buf), "carre:%.2f/%.2f  angle:%.0f/%.0f  taille:%.2f  decalage:%.1fcm",
                          gHaut.squareness, gBas.squareness, gHaut.maxAngleDeviation, gBas.maxAngleDeviation, sizeRatio, offsetCm);
            cv::putText(frame, buf, cv::Point(10, frame.rows - 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

            if (squareOk && angleOk && sizeOk && centeredOk)
            {
                statusText = "Positionnement OK";
                statusColor = cv::Scalar(0, 200, 0); // vert
            }
            else if (!squareOk || !angleOk)
            {
                statusText = "Inclinaison detectee (marqueur deforme en trapeze) - mets la camera a plat";
            }
            else if (!sizeOk)
            {
                statusText = "Camera decentree ou inclinee (tailles des 2 marqueurs trop differentes)";
            }
            else
            {
                char cbuf[160];
                std::snprintf(cbuf, sizeof(cbuf), "Camera pas centree sur la zone (decalage %.1f cm) - recentre-la sur la mire", offsetCm);
                statusText = cbuf;
            }
        }
        else
        {
            // Camera inclinee par design (~4 deg, Camera 1 ou Camera 3) : la mire vise le
            // CENTRE DE SA PROPRE ZONE (pas la bille bleue) - c'est justement le but de
            // l'inclinaison : recentrer le champ de vision sur la zone plutot que sur
            // l'aplomb de la camera. La forme (carre) de chaque marqueur reste indicative
            // seulement, un vrai trapeze etant attendu du fait de l'inclinaison voulue.
            std::vector<MarkerGeometry> geoms;
            for (int id : spec.ids) geoms.push_back(analyzeMarkerGeometry(byId[id]));

            bool allShapeOk = std::all_of(geoms.begin(), geoms.end(), [&](const MarkerGeometry& g) {
                return g.squareness >= SQUARENESS_MIN && g.maxAngleDeviation <= ANGLE_DEV_MAX;
            });

            cv::Point2f centroid(0.f, 0.f);
            for (const auto& g : geoms) centroid += g.center;
            centroid *= 0.25f;

            // Echelle px->cm approximative via le cote HAUT (entre les 2 premiers marqueurs,
            // distance reelle connue = largeur de la zone, 130cm).
            float topDistPx = static_cast<float>(cv::norm(geoms[0].center - geoms[1].center));
            float pxPerCm = topDistPx / ZONE_LENGTH_CM;
            float offsetPx = static_cast<float>(cv::norm(centroid - imgCenter));
            float offsetCm = pxPerCm > 1e-3f ? offsetPx / pxPerCm : std::numeric_limits<float>::infinity();
            bool centeredOk = std::isfinite(offsetCm) && offsetCm <= OFFSET_MAX_CM;

            cv::Scalar centerColor = centeredOk ? cv::Scalar(0, 200, 0) : cv::Scalar(0, 0, 255);
            cv::line(frame, imgCenter, centroid, centerColor, 1);
            cv::circle(frame, cv::Point(static_cast<int>(centroid.x), static_cast<int>(centroid.y)), 5, centerColor, -1);

            char buf[256];
            std::snprintf(buf, sizeof(buf), "carre: %.2f / %.2f / %.2f / %.2f  decalage:%.1fcm  (cible a %.1fcm de la bille bleue)",
                          geoms[0].squareness, geoms[1].squareness, geoms[2].squareness, geoms[3].squareness, offsetCm, distToBlueCm);
            cv::putText(frame, buf, cv::Point(10, frame.rows - 15), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

            if (centeredOk)
            {
                statusText = "Positionnement OK (zone bien cadree)";
                statusColor = cv::Scalar(0, 200, 0); // vert
            }
            else
            {
                char cbuf[192];
                std::snprintf(cbuf, sizeof(cbuf), "Zone pas centree dans l'image (decalage %.1f cm) - ajuste l'inclinaison vers la mire", offsetCm);
                statusText = cbuf;
            }
            if (!allShapeOk)
            {
                statusText += " [forme d'un marqueur suspecte, verifie qu'il est bien colle a plat]";
            }
        }

        for (const auto& kv : byId)
        {
            std::vector<cv::Point> pts;
            for (const auto& p : kv.second) pts.push_back(cv::Point(static_cast<int>(p.x), static_cast<int>(p.y)));
            cv::polylines(frame, pts, true, cv::Scalar(0, 255, 255), 2);
        }

        cv::putText(frame, statusText, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, statusColor, 2);
        cv::imshow("Test de positionnement - avant calibration", frame);

        int key = cv::waitKey(isNumber ? 30 : 0);
        if (key == 'q' || key == 'Q' || key == 27) break;
        if (!isNumber) break; // image fixe : une seule passe, une touche quelconque ferme
    }
}

int main(int argc, char** argv)
{
    if (argc >= 2 && std::string(argv[1]) == "--generate")
    {
        generateMarkers();
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--check")
    {
        if (argc < 4)
        {
            std::cout << "Utilisation : ArucoCalibrationTool.exe --check <index_camera 0/1/2> <image_ou_index_flux>" << std::endl;
            return 1;
        }
        int cameraIndex = std::stoi(argv[2]);
        if (cameraIndex < 0 || cameraIndex >= CameraSystem::NB_CAMERAS)
        {
            std::cout << "Erreur : l'index camera doit etre 0, 1 ou 2." << std::endl;
            return 1;
        }
        runPositionCheck(cameraIndex, argv[3]);
        return 0;
    }

    if (argc < 3)
    {
        std::cout << "Utilisation :" << std::endl;
        std::cout << "  Generer les marqueurs        : ArucoCalibrationTool.exe --generate" << std::endl;
        std::cout << "  Tester le positionnement      : ArucoCalibrationTool.exe --check <index_camera 0/1/2> <image_ou_index_flux>" << std::endl;
        std::cout << "  Calibrer une camera (definitif): ArucoCalibrationTool.exe <index_camera 0/1/2> <image_ou_index_flux>" << std::endl;
        return 1;
    }

    int cameraIndex = std::stoi(argv[1]);
    std::string source = argv[2];

    if (cameraIndex < 0 || cameraIndex >= CameraSystem::NB_CAMERAS)
    {
        std::cout << "Erreur : l'index camera doit etre 0, 1 ou 2." << std::endl;
        return 1;
    }

    cv::Mat frame;
    bool isNumber = !source.empty() && std::all_of(source.begin(), source.end(), ::isdigit);

    if (isNumber)
    {
        cv::VideoCapture cap(std::stoi(source));
        if (!cap.isOpened())
        {
            std::cout << "Impossible d'ouvrir la camera " << source << std::endl;
            return 1;
        }
        cap >> frame;
    }
    else
    {
        frame = cv::imread(source);
    }

    if (frame.empty())
    {
        std::cout << "Image vide, impossible de continuer." << std::endl;
        return 1;
    }

    // -----------------------------------------------------------------
    // Detection automatique des marqueurs de CETTE camera (voir
    // getCameraMarkerSpec() : 2 marqueurs/2 points pour la camera
    // verticale (bleue), 4 marqueurs/homographie pour les 2 cameras
    // inclinees, conformement a manuel_calibration_cameras.odt).
    // -----------------------------------------------------------------
    CameraMarkerSpec spec = getCameraMarkerSpec(cameraIndex);

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);

    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> rejected;
    detector.detectMarkers(frame, corners, ids, rejected);

    std::cout << "Marqueurs detectes : " << ids.size() << " (attendus : ";
    for (size_t i = 0; i < spec.ids.size(); i++) std::cout << "ID " << spec.ids[i] << (i + 1 < spec.ids.size() ? ", " : "");
    std::cout << ")" << std::endl;

    std::map<int, cv::Point2f> markerCenters;
    for (size_t i = 0; i < ids.size(); i++)
    {
        cv::Point2f center(0.f, 0.f);
        for (const auto& pt : corners[i])
        {
            center += pt;
        }
        center *= (1.0f / corners[i].size());
        markerCenters[ids[i]] = center;
        std::cout << "  ID " << ids[i] << " detecte au centre (" << center.x << ", " << center.y << ")" << std::endl;
    }

    bool allPresent = std::all_of(spec.ids.begin(), spec.ids.end(), [&](int id) { return markerCenters.count(id) > 0; });
    if (!allPresent)
    {
        std::cout << "Erreur : il manque au moins un marqueur parmi : ";
        for (size_t i = 0; i < spec.ids.size(); i++) std::cout << "ID " << spec.ids[i] << (i + 1 < spec.ids.size() ? ", " : "");
        std::cout << std::endl;
        return 1;
    }

    CameraCalibration calib;
    bool ok = false;
    if (spec.tilted)
    {
        // 4 marqueurs -> homographie complete (haut-gauche, haut-droit, bas-droit, bas-gauche).
        std::vector<cv::Point2f> imagePoints;
        for (int id : spec.ids) imagePoints.push_back(markerCenters[id]);
        ok = calib.calibrate(imagePoints, spec.tableCm);
    }
    else
    {
        // 2 marqueurs -> transformation simplifiee (rotation + echelle + translation).
        ok = calib.calibrateFromTwoPoints(
            markerCenters[spec.ids[0]], markerCenters[spec.ids[1]],
            spec.tableCm[0], spec.tableCm[1]
        );
    }

    if (!ok)
    {
        std::cout << "Echec de la calibration." << std::endl;
        return 1;
    }

    std::string outputPath = "camera_" + std::to_string(cameraIndex) + ".yml";
    calib.saveToFile(outputPath);
    std::cout << "Calibration automatique reussie et sauvegardee : " << outputPath << std::endl;

    return 0;
}