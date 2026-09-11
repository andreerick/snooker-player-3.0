#include <opencv2/opencv.hpp>
#include <iostream>

void TestOpenCV()
{
    cv::VideoCapture camera(0);

    if (!camera.isOpened())
    {
        std::cout << "ERREUR : camera introuvable" << std::endl;
        return;
    }

    cv::Mat image;

    while (true)
    {
        camera >> image;

        if (image.empty())
            break;

        cv::imshow("Camera Snooker Player", image);

        int touche = cv::waitKey(30);

        // ESPACE = prendre une photo
        if (touche == 32)
        {
            cv::imwrite("C:\\Users\\Public\\capture_snooker.jpg", image);

            std::cout
                << "Photo enregistree : capture_snooker.jpg"
                << std::endl;
        }

        // ECHAP = quitter
        if (touche == 27)
            break;
    }

    camera.release();
    cv::destroyAllWindows();
}