// ImageSegmentation.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp> 
#include <time.h>
#include <stack>
using namespace cv;
using namespace std;

//Convierte imagen a binaria usando un umbral
void ImageThreshold(const Mat &sourceImage, Mat &outImage, double umbral);
// k-means algorithm
double kmean(const Mat &sourceImage, Mat &labelImage, int k, std::vector<Vec3d> &start_colors);
void stackRegionGrow(Mat &image, Mat &outImage, Point p);
void recursiveRegionGrow(Mat &image, Mat &outImage, Point p);
void recursiveRegionGrowRGB(Mat &image, Mat &outR, Mat &outG, Mat &outB, Point p);
int main()
{
	Mat image, gray, grayStack, recursiveGray; 
	
	image = imread("C:\\Users\\Joe\\Pictures\\fruits.jpg", CV_LOAD_IMAGE_COLOR);
	

	if (!image.data) {
		std::cout << "Could not find the image" << std::endl;
		return -1;
	}
	cv::resize(image, image, Size(), 0.25, 0.25, INTER_NEAREST);

	cvtColor(image, gray, CV_RGB2GRAY);
	stack<Point> stack, stackR;

	while (stack.size() <= 64) {
		int x = rand() % gray.cols;
		int y = rand() % gray.rows;

		if (gray.at<uchar>(y, x) > 150) {
			stack.push(Point(x, y));
			stackR.push(Point(x, y));
		}
	}

	namedWindow("Original", WINDOW_AUTOSIZE);
	cv::imshow("Original", image);

	//Growing-region
	cvtColor(image, grayStack, CV_RGB2GRAY);
	Mat outImage = Mat(gray.rows, gray.cols, gray.type(), Scalar(0));
	while (stack.size() > 0) {
		Point currentPoint = stack.top();
		stack.pop();
		stackRegionGrow(grayStack, outImage, currentPoint);
	}
	imshow("Stack GR", outImage);
	//imshow("StackOr", grayStack);

	////Recursive-growing region
	cvtColor(image, recursiveGray, CV_RGB2GRAY);
	Mat outImageRecursive = Mat(recursiveGray.rows, recursiveGray.cols, recursiveGray.type(), Scalar(0));

	while (stackR.size() > 0) {
		Point currentPoint = stackR.top();
		stackR.pop();
		recursiveRegionGrow(recursiveGray, outImageRecursive, currentPoint);
	}
	cv::imshow("orRec", recursiveGray);
	cv::imshow("Recursive GR", outImageRecursive);

	imwrite("C:\\Users\\Joe\\Pictures\\lenaRec.jpg", outImageRecursive);
	imwrite("C:\\Users\\Joe\\Pictures\\lenaStack.jpg", outImage);
	waitKey(0);

	return 0;
}

void ImageThreshold(const Mat &sourceImage, Mat &outImage, double umbral)
{
	if (outImage.empty())
		outImage = Mat(sourceImage.rows, sourceImage.cols, sourceImage.type());
	MatConstIterator_<Vec3b> it_in = sourceImage.begin<Vec3b>(), it_in_end = sourceImage.end<Vec3b>();
	MatIterator_<Vec3b> it_out = outImage.begin<Vec3b>();
	bool t;
	for (; it_in != it_in_end; ++it_in, ++it_out)
	{
		(*it_out)[0] = 0;
		(*it_out)[1] = 0;
		(*it_out)[2] = 0;
		t = (*it_in)[0] > umbral;
		t |= (*it_in)[1] > umbral;
		t |= (*it_in)[2] > umbral;
		if (t)
		{
			(*it_out)[0] = 255;
			(*it_out)[1] = 255;
			(*it_out)[2] = 255;
		}
	}
}

// k segmentation of a color image
double kmean(const Mat &sourceImage, Mat &labelImage, int k, std::vector<Vec3d> &start_colors)
{
	srand(time(NULL));
	if (labelImage.empty())
		labelImage = Mat(sourceImage.rows, sourceImage.cols, sourceImage.type());
	Mat sampImage;
	Mat labImage;
	int j, n, changes, currentClass, kclass;
	double distance, mindistance;
	cv::resize(sourceImage, sampImage, Size(), 0.125, 0.125, INTER_NEAREST);
	//    imshow("Sampled", sampImage);
	labImage = sampImage.clone();
	//  imshow("Small", sampImage);
	MatConstIterator_<Vec3b> it_in = sampImage.begin<Vec3b>(), it_in_end = sampImage.end<Vec3b>();
	MatIterator_<Vec3b> it_label = labImage.begin<Vec3b>();
	Vec3d red(0.5, 0, 0);
	Vec3d green(0, 0.5, 0);
	Vec3d blue(0, 0, 0.5);
	Vec3d black(0.0, 0.0, 0.0);
	Vec3d white(1.0, 1.0, 1.0);
	Vec3d dis;
	std::vector<Vec3d> mean_colors, newmean_colors, five_colors;
	std::vector<int> size_colors;
	mean_colors.resize(k, black);
	newmean_colors.resize(k, black);
	size_colors.resize(k, 0);
	five_colors.resize(5, 0);
	five_colors[0] = red;
	five_colors[1] = green;
	five_colors[2] = blue;
	five_colors[3] = black;
	five_colors[4] = white;
	if (start_colors.size() == (unsigned int)k)
	{
		for (n = 0; n < k; n++)
		{
			mean_colors[n] = start_colors[n];
		}
	}
	else
	{
		start_colors.resize(k, 0);
		if (k == 5)
		{
			mean_colors[0] = red;
			mean_colors[1] = green;
			mean_colors[2] = blue;
			mean_colors[3] = black;
			mean_colors[4] = white;
		}
		else
		{
			for (n = 0; n < k; n++)
			{
				for (j = 0; j < 3; j++) mean_colors[n][j] = (double)rand() / RAND_MAX;
			}
		}
	}
	changes = 0;
	int loops = 0;
	do
	{
		//        printf("%d Changes=%d  (%d, %8.4f),(%d, %8.4f),(%d, %8.4f),(%d, %8.4f),(%d, %8.4f) \n",loops,changes,size_colors[0],mean_colors[0][0],size_colors[1],mean_colors[1][1],size_colors[2],mean_colors[2][2],size_colors[3],mean_colors[3][0],size_colors[4],mean_colors[4][0]);
		changes = 0;
		kclass = 0;
		for (n = 0; n < k; n++)
		{
			size_colors[n] = 0;
			newmean_colors[n][0] = 0;
			newmean_colors[n][1] = 0;
			newmean_colors[n][2] = 0;
		}
		for (it_in = sampImage.begin<Vec3b>(), it_label = labImage.begin<Vec3b>(); it_in != it_in_end; ++it_in, ++it_label)
		{
			currentClass = (*it_label)[0] % k;
			kclass = 0;
			mindistance = 0;
			for (j = 0; j < 3; j++)
			{
				dis[j] = (double)((*it_in)[j]) / 255.0 - mean_colors[0][j];
				mindistance += dis[j] * dis[j];
			}
			for (n = 1; n < k; n++)
			{
				distance = 0;
				for (j = 0; j < 3; j++)
				{
					dis[j] = (double)((*it_in)[j]) / 255.0 - mean_colors[n][j];
					distance += dis[j] * dis[j];
				}
				if (distance < mindistance)
				{
					mindistance = distance;
					kclass = n;
				}
			}
			changes += (currentClass != kclass);
			(*it_label)[0] = kclass;
			++size_colors[kclass];
			for (j = 0; j < 3; j++)
			{
				newmean_colors[kclass][j] += (double)((*it_in)[j]) / 255.0;
			}
		}
		for (n = 0; n < k; n++)
		{
			if (size_colors[n] > 0)
			{
				for (j = 0; j < 3; j++)
				{
					mean_colors[n][j] = newmean_colors[n][j] / size_colors[n];
				}
			}
		}
		//        imshow("SmallLabels", 20*labImage);
		//        printf("%d Changes=%d  (%d, %8.4f),(%d, %8.4f),(%d, %8.4f),(%d, %8.4f),(%d, %8.4f) \n",loops,changes,size_colors[0],mean_colors[0][0],size_colors[1],mean_colors[1][1],size_colors[2],mean_colors[2][2],size_colors[3],mean_colors[3][0],size_colors[4],mean_colors[4][0]);
		++loops;
	} while ((changes > 100) && (loops < 100));
	MatConstIterator_<Vec3b> it_inSource = sourceImage.begin<Vec3b>(), it_in_endSource = sourceImage.end<Vec3b>();
	MatIterator_<Vec3b> it_inLabel = labelImage.begin<Vec3b>();
	for (it_inSource = sourceImage.begin<Vec3b>(), it_inLabel = labelImage.begin<Vec3b>(); it_inSource != it_in_endSource; ++it_inSource, ++it_inLabel)
	{
		currentClass = 0;
		kclass = 0;
		mindistance = 0;
		for (j = 0; j < 3; j++)
		{
			dis[j] = (double)((*it_inSource)[j]) / 255.0 - mean_colors[0][j];
			mindistance += dis[j] * dis[j];
		}
		for (n = 1; n < k; n++)
		{
			distance = 0;
			for (j = 0; j < 3; j++)
			{
				dis[j] = (double)((*it_inSource)[j]) / 255.0 - mean_colors[n][j];
				distance += dis[j] * dis[j];
			}
			if (distance < mindistance)
			{
				mindistance = distance;
				kclass = n;
			}
		}
		if (k != 5)
		{
			(*it_inLabel)[0] = mean_colors[kclass][0] * 255;
			(*it_inLabel)[1] = mean_colors[kclass][1] * 255;
			(*it_inLabel)[2] = mean_colors[kclass][2] * 255;
		}
		//            (*it_inLabel)[0]= kclass*50;
		//            (*it_inLabel)[1]= kclass*50;
		//            (*it_inLabel)[2]= kclass*50;
		else
		{
			(*it_inLabel)[0] = five_colors[kclass][0] * 255;
			(*it_inLabel)[1] = five_colors[kclass][1] * 255;
			(*it_inLabel)[2] = five_colors[kclass][2] * 255;
		}
	}
	for (n = 0; n < k; n++)
	{
		start_colors[n] = mean_colors[n];
	}
	return 0;
}

void stackRegionGrow(Mat &image, Mat &outImage, Point p) {
	stack<Point> stack;
	stack.push(p);
	float seedValue = image.at<uchar>(p.y, p.x);

	float threshValue = 50;

	float minVal = 50;
	if (seedValue - threshValue > minVal) {
		minVal = seedValue - threshValue;
	}
	float maxVal = 255;
	if (seedValue + threshValue < maxVal) {
		maxVal = seedValue + threshValue;
	}

	while (!stack.empty()) {

		Point currentP = stack.top();
		stack.pop();

		float pixelValue = image.at<uchar>(currentP.y, currentP.x);
		if (pixelValue > minVal && pixelValue < maxVal) {
			outImage.at<uchar>(currentP.y, currentP.x) = 255;
			image.at<uchar>(currentP.y, currentP.x) = 0;

			//Add neighbors
			if (currentP.x > 0) {
				stack.push(Point(currentP.x - 1, currentP.y));
			}
			if (currentP.y > 0) {
				stack.push(Point(currentP.x, currentP.y - 1));
			}
			if (currentP.x < image.cols - 1) {
				stack.push(Point(currentP.x + 1, currentP.y));
			}
			if (currentP.y < image.rows - 1) {
				stack.push(Point(currentP.x, currentP.y + 1));
			}
		}

	}
}


void recursiveRegionGrow(Mat &image, Mat &outImage, Point p) {

	if (p.x > 0 && p.x < (image.cols - 1) && p.y > 0 && p.y < (image.rows - 1)) {
		
		if (image.at<uchar>(p.y, p.x) > 150) {
			outImage.at<uchar>(p.y, p.x) = 255;
			image.at<uchar>(p.y, p.x) = 0;
			recursiveRegionGrow(image, outImage, Point(p.x, p.y + 1));
			recursiveRegionGrow(image, outImage, Point(p.x + 1, p.y));
			recursiveRegionGrow(image, outImage, Point(p.x, p.y - 1));
			recursiveRegionGrow(image, outImage, Point(p.x - 1, p.y));
		}
		image.at<uchar>(p.y, p.x) = 0;	
	}
}

//void recursiveRegionGrowRGB(Mat &image, Mat &outR, Mat &outG, Mat &outB, Point p) {
//
//	if (p.x >= 0 && p.x < (image.cols - 1) && p.y >= 0 && p.y < (image.rows - 1)) {
//
//		if (image.at<uchar>(p.y, p.x) > 150) {
//			outImage.at<uchar>(p.y, p.x) = 255;
//			image.at<uchar>(p.y, p.x) = 0;
//			recursiveRegionGrow(image, outImage, Point(p.x - 1, p.y));
//			recursiveRegionGrow(image, outImage, Point(p.x, p.y + 1));
//			recursiveRegionGrow(image, outImage, Point(p.x + 1, p.y));
//			recursiveRegionGrow(image, outImage, Point(p.x, p.y - 1));
//		}
//	}
//}

//used to iterate an image
//MatIterator_<Vec3b> it_in = image.begin<Vec3b>(), it_in_end = image.end<Vec3b>();
//for (; it_in != it_in_end; ++it_in) {
//	(*it_in)[0] = 0;
//	(*it_in)[1] = 0;
//	(*it_in)[2] = 0;
//}
