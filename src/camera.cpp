#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"
#include "matching2D.hpp"

#include <boost/circular_buffer.hpp>

#include "displacement_calculator.hpp"

using namespace std;

/* MAIN PROGRAM */
int main(int argc, const char *argv[])
{

	int num_args = argc;
	if(num_args!=5)
	{
		cout<<"Incorrect argument number"<<endl<<endl;
		cout<<"Usage: "<<endl;
		cout<<"      2D_feature_tracking <Detector type> <Descriptor type> <Match selector type>"<<endl;
		cout<<"Detector types: SHITOMASI , HARRIS , FAST , BRISK , ORB, AKAZE , SIFT"<<endl;
		cout<<"Descriptor types: BRISK , BRIEF , ORB , FREAK , AKAZE , SIFT"<<endl;
		cout<<"Match types: SEL_NN , SEL_KNN"<<endl;
		cout<<"Note: you can only use AKAZE detector with AKAZE descriptor, so don't mix AKAZE det/desc with oher options"<<endl;
		return -1;
	}

	string detectorType = argv[1];
	string descriptorType = argv[2];
	string selectorType = argv[3];
	string img_folder = argv[4];

    /* INIT VARIABLES AND DATA STRUCTURES */

    // data location
    string dataPath = "../";

    // camera
    string imgBasePath = dataPath + "images/";
    string imgPrefix = img_folder + "/"; // left camera, color
    string imgFileType = ".png";
    int imgStartIndex = 0; // first file index to load (assumes Lidar and camera names have identical naming convention)
    //int imgEndIndex = 9;   // last file index to load
    int imgEndIndex = 24;   // last file index to load
    int imgFillWidth = 4;  // no. of digits which make up the file index (e.g. img-0001.png)

    // misc
    int dataBufferSize = 2;       // no. of images which are held in memory (ring buffer) at the same time
    boost::circular_buffer<DataFrame> dataBuffer(dataBufferSize);
    bool bVis = false;            // visualize results

    /* MAIN LOOP OVER ALL IMAGES */

    for (size_t imgIndex = 0; imgIndex <= imgEndIndex - imgStartIndex; imgIndex++)
    {
        /* LOAD IMAGE INTO BUFFER */

        // assemble filenames for current index
        ostringstream imgNumber;
        //imgNumber << setfill('0') << setw(imgFillWidth) << imgStartIndex + imgIndex;
        imgNumber << imgStartIndex + imgIndex;
        string imgFullFilename = imgBasePath + imgPrefix + imgNumber.str() + imgFileType;

        cout <<"image full name: "<<imgFullFilename<< endl;


        // load image from file and convert to grayscale
        cv::Mat img, imgGray;
        img = cv::imread(imgFullFilename);
        cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

        DataFrame frame;
        frame.cameraImg = imgGray;
        dataBuffer.push_back(frame);

        cout << "#1 : LOAD IMAGE INTO BUFFER done" << endl;

        /* DETECT IMAGE KEYPOINTS */

        // extract 2D keypoints from current image
        vector<cv::KeyPoint> keypoints; // create empty feature list for current image

        if (detectorType.compare("SHITOMASI") == 0)
        {
            detKeypointsShiTomasi(keypoints, imgGray, false);
        }
        else
        {
        	if (detectorType.compare("HARRIS") == 0)
        	{
        		detKeypointsHarris(keypoints, imgGray, false);
        	}
        	else
        	{
        		detKeypointsModern(keypoints, imgGray, detectorType, false);
        	}
        }

        // only keep keypoints on the preceding vehicle
        bool bFocusOnVehicle = false;
        cv::Rect vehicleRect(535, 180, 180, 150);
        if (bFocusOnVehicle)
        {
        	vector<cv::KeyPoint> inVehicleKeypoints;
            for(auto it = keypoints.begin(); it != keypoints.end(); it++)
            {
            	if( vehicleRect.contains(it->pt))
            	{
            		inVehicleKeypoints.push_back(*it);
            	}
            }

            keypoints = inVehicleKeypoints;
            cout << " -----> Number of Keypoints on the preceding vehic: "<< keypoints.size() << endl;
        }

        // optional : limit number of keypoints (helpful for debugging and learning)
        bool bLimitKpts = true;
        if (bLimitKpts)
        {
            int maxKeypoints = 250;
            if (detectorType.compare("SHITOMASI") == 0)
            { // there is no response info, so keep the first 50 as they are sorted in descending quality order
                keypoints.erase(keypoints.begin() + maxKeypoints, keypoints.end());
            }
            cv::KeyPointsFilter::retainBest(keypoints, maxKeypoints);
            cout << " NOTE: Keypoints have been limited!" << endl;
        }

        // push keypoints and descriptor for current frame to end of data buffer
        (dataBuffer.end() - 1)->keypoints = keypoints;
        cout << "#2 : DETECT KEYPOINTS done" << endl;

        /* EXTRACT KEYPOINT DESCRIPTORS */

        cv::Mat descriptors;

        descKeypoints((dataBuffer.end() - 1)->keypoints, (dataBuffer.end() - 1)->cameraImg, descriptors, descriptorType);

        // push descriptors for current frame to end of data buffer
        (dataBuffer.end() - 1)->descriptors = descriptors;

        cout << "#3 : EXTRACT DESCRIPTORS done" << endl;

        if (dataBuffer.size() > 1) // wait until at least two images have been processed
        {
            /* MATCH KEYPOINT DESCRIPTORS */
            vector<cv::DMatch> matches;
            string matcherType = "MAT_BF";        // MAT_BF, MAT_FLANN
            string descriptorFamily; 			  // DES_BINARY, DES_HOG
            if( (descriptorType.compare("SIFT")==0) || (descriptorType.compare("AKAZE")==0))
            {
            	descriptorFamily = "DES_HOG";
            }
            else
            {
            	descriptorFamily = "DES_BINARY";
            }


            matchDescriptors((dataBuffer.end() - 2)->keypoints, (dataBuffer.end() - 1)->keypoints,
                             (dataBuffer.end() - 2)->descriptors, (dataBuffer.end() - 1)->descriptors,
                             matches, descriptorFamily, matcherType, selectorType);

            /*calculate displacement*/
            displacement_calculator displ_calc;
            vector<cv::Point2f> displacements;
            double t = (double)cv::getTickCount();
            displ_calc.calc_displacements((dataBuffer.end() - 2)->keypoints, (dataBuffer.end() - 1)->keypoints,matches, displacements);
            t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
            cout << "******* displacement calculation done in " << 1000 * t / 1.0 << " ms" << endl;


            // store matches in current data frame
            (dataBuffer.end() - 1)->kptMatches = matches;

            cout << "#4 : MATCH KEYPOINT DESCRIPTORS done" << endl<< endl<< endl<< endl;

            // visualize matches between current and previous image
            bVis = true;
            if (bVis)
            {
                cv::Mat matchImg = ((dataBuffer.end() - 1)->cameraImg).clone();
                cv::drawMatches((dataBuffer.end() - 2)->cameraImg, (dataBuffer.end() - 2)->keypoints,
                                (dataBuffer.end() - 1)->cameraImg, (dataBuffer.end() - 1)->keypoints,
                                matches, matchImg,
                                cv::Scalar::all(-1), cv::Scalar::all(-1),
                                vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

                string windowName = "Matching keypoints between two camera images";
                cv::namedWindow(windowName, 7);
                cv::imshow(windowName, matchImg);
                cout << "Press key to continue to next image" << endl;
                cv::waitKey(0); // wait for key to be pressed
            }
            bVis = false;
        }

    } // eof loop over all images

    return 0;
}
