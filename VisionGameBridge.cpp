#include "VisionGameBridge.h"
#include <algorithm>
#include <map>

namespace
{
    struct BallInfo
    {
        std::string name;
        int value;
    };

    // Valeurs standard des billes de couleur (regles du snooker). La
    // Rouge est traitee comme les autres ici : contrairement au reste
    // de MesureVision (ou "Rouge" est exclue du filet de securite par
    // forme a cause des 15 exemplaires), le suivi par BallTracker gere
    // deja plusieurs rouges independamment, donc aucune raison de
    // l'exclure de la traduction vers le moteur de jeu.
    const std::vector<BallInfo> kColoredBalls = {
        { "Rouge",  1 },
        { "Jaune",  2 },
        { "Verte",  3 },
        { "Marron", 4 },
        { "Bleue",  5 },
        { "Rose",   6 },
        { "Noire",  7 }
    };
}

VisionGameBridge::VisionGameBridge(int maxFramesLost, float maxMatchDistancePx, int minRadius, int maxRadius, int analysisWidth)
    : m_tracker(maxFramesLost, maxMatchDistancePx)
    , m_minRadius(minRadius)
    , m_maxRadius(maxRadius)
    , m_analysisWidth(analysisWidth)
{
}

void VisionGameBridge::reset()
{
    m_tracker.reset();
    m_shotCounter = 0;
}

void VisionGameBridge::setSnapshotFolder(const std::string& folder)
{
    m_snapshotFolder = folder;
}

void VisionGameBridge::setPocketZones(const std::vector<PocketZone>& zones)
{
    m_shotAnalyzer.setPocketZones(zones);
    m_pocketZonesManuallySet = true;
}

void VisionGameBridge::ensurePocketZonesConfigured(const cv::Size& imageSize)
{
    if (m_pocketZonesManuallySet)
    {
        return;
    }
    if (imageSize == m_shotAnalyzerConfiguredSize)
    {
        return;
    }
    m_shotAnalyzerConfiguredSize = imageSize;

    // Approximation raisonnable en attendant une vraie calibration (voir
    // setPocketZones()) : 4 poches de coin + 2 poches de milieu (haut et
    // bas), en retrait du bord proportionnellement a la taille de
    // l'image plutot qu'un nombre de pixels fixe (qui n'aurait aucun
    // sens a la fois sur une petite image de test et sur une image 4K
    // ou une image assemblee 3 cameras).
    float w = static_cast<float>(imageSize.width);
    float h = static_cast<float>(imageSize.height);
    float radius = std::min(w, h) * 0.06f;

    std::vector<PocketZone> zones = {
        { "haut-gauche", cv::Point2f(radius, radius), radius },
        { "haut-milieu", cv::Point2f(w / 2.f, radius), radius },
        { "haut-droite", cv::Point2f(w - radius, radius), radius },
        { "bas-gauche",  cv::Point2f(radius, h - radius), radius },
        { "bas-milieu",  cv::Point2f(w / 2.f, h - radius), radius },
        { "bas-droite",  cv::Point2f(w - radius, h - radius), radius },
    };
    m_shotAnalyzer.setPocketZones(zones);
    m_shotAnalyzer.setEdgeMargin(std::min(w, h) * 0.03f);
}

void VisionGameBridge::processImage(const cv::Mat& image, Frame& frame, std::vector<std::string>* log)
{
    // Redimensionne une COPIE de travail pour la detection (rayons/distances
    // calibres pour ~m_analysisWidth px), quelle que soit la resolution
    // reelle de la camera (jusqu'a la 4K). Les positions/rayons detectes
    // sont remis a l'echelle de l'image D'ORIGINE juste apres, pour que
    // tout le reste (suivi, cartographie, guide de repositionnement,
    // photo de preuve ci-dessous) continue de raisonner en coordonnees
    // reelles sans rien changer d'autre dans le pipeline.
    cv::Mat analysisImage = image;
    double scaleBackUp = 1.0;
    if (image.cols > m_analysisWidth)
    {
        double scaleDown = static_cast<double>(m_analysisWidth) / image.cols;
        cv::resize(image, analysisImage, cv::Size(), scaleDown, scaleDown, cv::INTER_AREA);
        scaleBackUp = 1.0 / scaleDown;
    }

    std::vector<DetectedBall> detections = m_detector.detectBallsCombined(analysisImage, m_minRadius, m_maxRadius);
    if (scaleBackUp != 1.0)
    {
        for (auto& ball : detections)
        {
            ball.pixelPosition *= static_cast<float>(scaleBackUp);
            ball.radius *= static_cast<float>(scaleBackUp);
        }
    }

    std::vector<RemovedBall> removedBalls = m_tracker.update(detections);

    // Zones de poches (par defaut, proportionnelles a la taille de
    // l'image, sauf calibration reelle fournie via setPocketZones()) :
    // necessaires pour que analyzeDisappearance() ci-dessous puisse
    // distinguer un empochage plausible d'une disparition suspecte.
    ensurePocketZonesConfigured(image.size());

    bool actionTriggered = false;

    // Chaque bille qui vient de disparaitre du suivi (voir BallTracker::
    // update()) est analysee via ShotAnalyzer::analyzeDisappearance()
    // AVANT d'etre traitee comme un coup reel : une main qui passe
    // devant une bille, un reflet, ou une bille repoussee hors de la
    // table produirait sinon un faux coup/une fausse faute sur le
    // moteur de jeu. Seules les disparitions pres d'une poche connue
    // (Pocketed) ou pres du bord (OffTable) sont traitees ; le reste
    // (Unknown = occlusion probable, pas assez fiable) est ignore.
    for (const auto& removed : removedBalls)
    {
        if (removed.colorName == "Blanche")
        {
            // La bille blanche ne doit jamais disparaitre de la table :
            // si sa disparition est fiable (pas juste une occlusion),
            // c'est toujours une faute, empochee ou sortie de la table.
            DisappearanceResult verdict = m_shotAnalyzer.analyzeDisappearance(removed.lastPosition, image.size());
            if (verdict.type == DisappearanceType::Unknown)
            {
                if (log)
                {
                    log->push_back("Blanche disparue de facon incertaine (occlusion probable) -> ignoree");
                }
                continue;
            }

            Ball required = frame.getRequiredBall();
            int penalty = std::max(4, required.getValue());
            if (log)
            {
                log->push_back("Blanche confirmee empochee -> faute (" + std::to_string(penalty) + " points)");
            }
            frame.foul(required, Ball("Blanche", 0), penalty);
            actionTriggered = true;
            continue;
        }

        auto colorIt = std::find_if(kColoredBalls.begin(), kColoredBalls.end(),
            [&](const BallInfo& info) { return info.name == removed.colorName; });
        if (colorIt == kColoredBalls.end())
        {
            continue; // couleur inconnue : ignoree par securite
        }

        DisappearanceResult verdict = m_shotAnalyzer.analyzeDisappearance(removed.lastPosition, image.size());
        Ball ball(colorIt->name, colorIt->value);

        if (verdict.type == DisappearanceType::Pocketed)
        {
            // Coup reel sur le moteur de jeu, qui applique lui-meme les
            // regles du snooker (legal ou faute). Si un Free Ball est
            // arme (bouton dedie dans MainWindow, ex. faute precedente),
            // c'est playFreeBall() qu'il faut appeler et non playShot() :
            // lui seul verifie que la bille empochee correspond a la
            // couleur choisie pour le Free Ball et gere les transitions
            // de phase specifiques (voir Frame::playFreeBall()).
            if (frame.isFreeBall())
            {
                if (log)
                {
                    log->push_back(colorIt->name + " confirmee empochee (poche " + verdict.pocketName + ") -> Frame::playFreeBall()");
                }
                frame.playFreeBall(ball);
            }
            else
            {
                if (log)
                {
                    log->push_back(colorIt->name + " confirmee empochee (poche " + verdict.pocketName + ") -> Frame::playShot()");
                }
                frame.playShot(ball);
            }
            actionTriggered = true;
        }
        else if (verdict.type == DisappearanceType::OffTable)
        {
            Ball required = frame.getRequiredBall();
            Referee referee;
            int penalty = referee.calculateFoul(required, ball);
            if (log)
            {
                log->push_back(colorIt->name + " sortie de la table -> faute (" + std::to_string(penalty) + " points)");
            }
            frame.foul(required, ball, penalty, "Bille sortie de la table");
            actionTriggered = true;
        }
        else if (log)
        {
            log->push_back(colorIt->name + " disparue de facon incertaine (occlusion probable) -> ignoree");
        }
    }

    // Alerte precoce (voir ballsNearEdge()) : parmi les billes ENCORE
    // suivies (donc pas empochees/disparues cette image), lesquelles ont
    // une derniere position connue dangereusement proche du bord de la
    // table. Purement informatif : ne declenche jamais rien sur le
    // moteur de jeu, contrairement a la classification des disparitions
    // ci-dessus.
    m_ballsNearEdge.clear();
    for (const auto& colorEntry : m_tracker.getAllTracked())
    {
        for (const auto& ball : colorEntry.second)
        {
            if (m_shotAnalyzer.isNearTableEdge(ball.lastPosition, image.size()))
            {
                m_ballsNearEdge.push_back(colorEntry.first + " proche du bord de la table (risque de sortie)");
            }
        }
    }

    // Photo de preuve + cartographie des billes : prises juste apres un
    // coup confirme (pas a chaque image), pour garder une trace visuelle
    // ET les positions exactes de l'etat de la table a cet instant precis,
    // exploitables en cas de contestation, de miss a verifier, ou pour
    // generer plus tard un guide de repositionnement (voir
    // BallMapRecorder::renderRepositioningGuide). Rien ne se passe si
    // aucun dossier n'a ete configure.
    if (actionTriggered && !m_snapshotFolder.empty())
    {
        ++m_shotCounter;

        std::string photoPath = m_tableCapture.saveSnapshot(image, m_snapshotFolder, m_shotCounter);
        if (log && !photoPath.empty())
        {
            log->push_back("Photo de preuve sauvegardee : " + photoPath);
        }

        BallMapRecorder recorder(image.size());
        std::string mapPath = recorder.saveSnapshot(m_tracker.getAllTracked(), m_snapshotFolder, m_shotCounter);
        if (!mapPath.empty())
        {
            m_lastBallMapPath = mapPath;
            if (log)
            {
                log->push_back("Cartographie des billes sauvegardee : " + mapPath);
            }
        }
    }
}