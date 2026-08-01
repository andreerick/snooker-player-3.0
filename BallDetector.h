
Balldetector · CPP
#include "BallDetector.h"
#include <iostream>
 
// =====================================================================
// Plages HSV de depart pour chaque bille.
// OpenCV utilise H:0-179, S:0-255, V:0-255 (attention, H va jusqu'a
// 179 et pas 360 comme dans d'autres logiciels).
//
// !!! CES VALEURS SONT DES POINTS DE DEPART, PAS DES VALEURS FINALES !!!
// Elles devront etre affinees avec de vraies photos de la table une
// fois les cameras branchees, car l'eclairage change tout.
// =====================================================================
BallDetector::BallDetector()
{
    m_colorRanges.push_back({ "Rouge",  cv::Scalar(0,   150, 100), cv::Scalar(8,   255, 255) });
    m_colorRanges.push_back({ "Jaune",  cv::Scalar(20,  100, 100), cv::Scalar(35,  255, 255) });
    m_colorRanges.push_back({ "Verte",  cv::Scalar(45,  60,  60),  cv::Scalar(75,  255, 255) });
    m_colorRanges.push_back({ "Marron", cv::Scalar(9,   100, 80),  cv::Scalar(18,  220, 160) });
    m_colorRanges.push_back({ "Bleue",  cv::Scalar(100, 100, 60),  cv::Scalar(125, 255, 255) });
    m_colorRanges.push_back({ "Rose",   cv::Scalar(140, 50,  150), cv::Scalar(170, 180, 255) });
    m_colorRanges.push_back({ "Noire",  cv::Scalar(0,   0,   0),   cv::Scalar(179, 255, 40)  });
    m_colorRanges.push_back({ "Blanche",cv::Scalar(0,   0,   180), cv::Scalar(179, 60,  255) });
}
 
void BallDetector::setColorRange(
    const std::string& colorName,
    cv::Scalar hsvLow,
    cv::Scalar hsvHigh
)
{
    for (auto& range : m_colorRanges)
    {
        if (range.name == colorName)
        {
            range.low = hsvLow;
            range.high = hsvHigh;
            return;
        }
    }
    std::cout << "Couleur inconnue : " << colorName << std::endl;
}
 
cv::Mat BallDetector::getColorMask(const cv::Mat& frame, const std::string& colorName) const
{
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
 
    for (const auto& range : m_colorRanges)
    {
        if (range.name == colorName)
        {
            cv::Mat mask;
            cv::inRange(hsv, range.low, range.high, mask);
            return mask;
        }
    }
 
    std::cout << "Couleur inconnue : " << colorName << std::endl;
    return cv::Mat();
}
 
std::vector<DetectedBall> BallDetector::detectForColor(
    const cv::Mat& hsvFrame,
    const ColorRange& range,
    int minRadius,
    int maxRadius
) const
{
    std::vector<DetectedBall> results;
 
    cv::Mat mask;
    cv::inRange(hsvFrame, range.low, range.high, mask);
 
    // Nettoyage du masque : enleve le bruit (petits points isoles)
    // et comble les petits trous dans les zones detectees.
    cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
    cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
 
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
 
    for (const auto& contour : contours)
    {
        cv::Point2f center;
        float radius;
        cv::minEnclosingCircle(contour, center, radius);
 
        if (radius < minRadius || radius > maxRadius)
        {
            continue; // trop petit (bruit) ou trop gros (pas une bille)
        }
 
        // Verifie que le contour est bien "rond" (une bille est un cercle,
        // pas une forme allongee ou irreguliere comme un reflet).
        double area = cv::contourArea(contour);
        double circleArea = CV_PI * radius * radius;
        double circularity = area / circleArea;
 
        if (circularity < 0.7)
        {
            continue; // forme trop irreguliere pour etre une bille
        }
 
        DetectedBall ball;
        ball.colorName = range.name;
        ball.pixelPosition = center;
        ball.radius = radius;
        results.push_back(ball);
    }
 
    return results;
}
 
std::string BallDetector::classifyColorAtPoint(const cv::Vec3b& hsvPixel) const
{
    int h = hsvPixel[0];
    int s = hsvPixel[1];
    int v = hsvPixel[2];
 
    for (const auto& range : m_colorRanges)
    {
        bool hOk = (h >= range.low[0] && h <= range.high[0]);
        bool sOk = (s >= range.low[1] && s <= range.high[1]);
        bool vOk = (v >= range.low[2] && v <= range.high[2]);
        if (hOk && sOk && vOk)
        {
            return range.name;
        }
    }
    return "Inconnue";
}
 
std::vector<DetectedBall> BallDetector::detectBallsByShape(
    const cv::Mat& frame,
    int minRadius,
    int maxRadius
) const
{
    std::vector<DetectedBall> results;
 
    if (frame.empty())
    {
        std::cout << "Image vide, impossible de detecter les billes." << std::endl;
        return results;
    }
 
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, gray, 5);
 
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(
        gray, circles, cv::HOUGH_GRADIENT,
        1.2,            // dp
        15,             // distance minimale entre deux centres
        40,             // param1 : seuil haut de Canny (interne a Hough)
        20,             // param2 : seuil d'accumulateur (plus bas = plus permissif)
        minRadius,
        maxRadius
    );
 
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
 
    for (const auto& c : circles)
    {
        int x = static_cast<int>(std::round(c[0]));
        int y = static_cast<int>(std::round(c[1]));
        float radius = c[2];
 
        if (x < 0 || y < 0 || x >= hsv.cols || y >= hsv.rows)
        {
            continue;
        }
 
        cv::Vec3b pixel = hsv.at<cv::Vec3b>(y, x);
        std::string colorName = classifyColorAtPoint(pixel);
 
        DetectedBall ball;
        ball.colorName = colorName;
        ball.pixelPosition = cv::Point2f(static_cast<float>(x), static_cast<float>(y));
        ball.radius = radius;
        results.push_back(ball);
    }
 
    return results;
}
 
 
std::vector<DetectedBall> BallDetector::detectBalls(
    const cv::Mat& frame,
    int minRadius,
    int maxRadius
) const
{
    std::vector<DetectedBall> allBalls;
 
    if (frame.empty())
    {
        std::cout << "Image vide, impossible de detecter les billes." << std::endl;
        return allBalls;
    }
 
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
 
    // Un peu de flou pour reduire le bruit avant la detection de couleur.
    cv::GaussianBlur(hsv, hsv, cv::Size(5, 5), 0);
 
    for (const auto& range : m_colorRanges)
    {
        std::vector<DetectedBall> found = detectForColor(hsv, range, minRadius, maxRadius);
        allBalls.insert(allBalls.end(), found.begin(), found.end());
    }
 
    return allBalls;
}
 







