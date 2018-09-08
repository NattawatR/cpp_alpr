
//opencv
#include "opencv2/imgcodecs.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include "opencv2/objdetect/objdetect.hpp"

//C++
#include <iostream>
#include <sstream>

#include "package_bgs/PBAS/PixelBasedAdaptiveSegmenter.h"
#include "package_tracking/BlobTracking.h"
#include "package_analysis/VehicleCouting.h"

using namespace cv;
using namespace std;

// Global variables
Mat frame, grayFrame;            //current frame
Mat fgMaskMOG2, k;               //fg mask fg mask generated by MOG2 method
Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
int keyboard;                    //input from keyboard

int morph_elem = 1;
int morph_size = 3;
int morph_operator = 1;
int const max_operator = 4;
int const max_elem = 2;
int const max_kernel_size = 21;
string window_name = "FG Mask MOG 2";
//Rect region_of_interest = Rect(20, 300, 680, 200);
Rect region_of_interest = Rect(250, 450, 1000, 250);

Mat flow, cflow;
UMat gray, prevgray, uflow;

//string car_cascade_name = "cars.xml";
//CascadeClassifier car_cascade;

//Hsv project
Mat hsvImage;

//Edge
int scale = 1;
int delta = 0;
int ddepth = CV_16S;

Mat grad_x, grad_y;
Mat abs_grad_x, abs_grad_y, grad;

//Init interface
void processVideo(string videoFilename);
void processImages(string imageFilename);
void Morphology_Operations();
void Morphology_Operations_2(int, void *);
static void drawOptFlowMap(const Mat &flow, Mat &cflowmap, int step, double, const Scalar &color);

string cars_cascade_name = "/home/kridsada/Documents/LicenPLateProject/cpp_alpr/cars.xml";
CascadeClassifier cars_cascade;
vector<Rect> cars;

int main(int argc, char *argv[])
{
    Mat image;
    image = imread("/home/kridsada/Documents/LicenPLateProject/cpp_alpr/test_images/imageTest.jpg", CV_LOAD_IMAGE_COLOR); // Read the file

    if (!image.data) // Check for invalid input
    {
        cout << "Could not open or find the image" << std::endl;
        return -1;
    }

    
    

    cout << "Process image for imageTest" << endl;
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(11, 11));
    // Mat morpElement = getStructuringElement(1, Size(1, 1), Point(1, 1));

    Mat grayImage;
    // To gray
    cvtColor(image, grayImage, CV_BGR2GRAY);

    // Top hat
    morphologyEx(grayImage, grayImage, MORPH_TOPHAT, element, Point(-1, -1));

    // Mat grayImageheight = grayImage > 40;
    // Mat adaptive;
    // adaptiveThreshold(grayImage, adaptive, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 105, 1); 
    // grayImage = grayImage > 10;

    // Mat andImage;
    // bitwise_and(grayImage, adaptive, andImage);

    // Mat dilatImage;
    // erode(adaptive, dilatImage, Mat(), Point(-1, -1), 1, 1, 1);



    namedWindow("Display window", WINDOW_AUTOSIZE); // Create a window for display.
    imshow("Normal image", grayImage);
    // imshow("Display window", dilatImage);
    // imshow("Gray image height", grayImageheight);
    // imshow("Adaptive", adaptive);
    // imshow("And", andImage);

    waitKey(0); // Wait for a keystroke in the window
    return 0;
}

void processVideo(string videoFilename)
{
    //create the capture object
    VideoCapture capture(videoFilename);
    if (!capture.isOpened())
    {
        //error in opening the video input
        cerr << "Unable to open video file: " << videoFilename << endl;
        exit(EXIT_FAILURE);
    }
    //read input data. ESC or 'q' for quitting
    while ((char)keyboard != 'q' && (char)keyboard != 27)
    {
        //read the current frame
        if (!capture.read(frame))
        {
            cerr << "Unable to read next frame." << endl;
            cerr << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }

        //Crop image
        frame = frame(region_of_interest);

        //Hsv test
        cvtColor(frame, hsvImage, CV_BGR2GRAY);

        cars_cascade.detectMultiScale(hsvImage, cars, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));

        for (size_t j = 0; j < cars.size(); j++)
        {
            rectangle(hsvImage, Point(cars[j].x, cars[j].y), Point(cars[j].x + cars[j].width, cars[j].y + cars[j].height), Scalar(0, 55, 255), +1, 4);
            //Point center(eyes[j].x + eyes[j].width * 0.5,eyes[j].y + eyes[j].height * 0.5);
            //int radius = cvRound((eyes[j].width + eyes[j].height) * 0.25);
            //circle(fgMaskMOG2, center, radius, Scalar(255, 0, 0), 4, 8, 0);
        }

        imshow("frame", frame);
        imshow("hsv", hsvImage);
        keyboard = waitKey(30);
    }
    //delete capture object
    capture.release();
}

void processImages(string imageFilename)
{
    //read the first file of the sequence
    frame = imread(imageFilename);
    imshow("test", frame);
}

/**
  * @function Morphology_Operations
  */
void Morphology_Operations()
{
    // Since MORPH_X : 2,3,4,5 and 6
    int operation = 1 + 2;

    Mat element = getStructuringElement(0, Size(2 * 3 + 1, 2 * 3 + 1), Point(5, 5));

    /// Apply the specified morphology operation
    morphologyEx(fgMaskMOG2, fgMaskMOG2, operation, element);
}

void Morphology_Operations_2(int, void *)
{
    // Since MORPH_X : 2,3,4,5 and 6
    int operation = 0 + 2;

    Mat element = getStructuringElement(1, Size(2 * 1 + 1, 2 * 1 + 1), Point(1, 1));

    /// Apply the specified morphology operation
    morphologyEx(fgMaskMOG2, fgMaskMOG2, operation, element);
}

static void drawOptFlowMap(const Mat &flow, Mat &cflowmap, int step,
                           double, const Scalar &color)
{
    for (int y = 0; y < cflowmap.rows; y += step)
        for (int x = 0; x < cflowmap.cols; x += step)
        {
            const Point2f &fxy = flow.at<Point2f>(y, x);
            line(cflowmap, Point(x, y), Point(cvRound(x + fxy.x), cvRound(y + fxy.y)),
                 color);
            circle(cflowmap, Point(x, y), 2, color, -1);
        }
}