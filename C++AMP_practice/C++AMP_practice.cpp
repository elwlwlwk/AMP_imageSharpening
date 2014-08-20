// C++AMP_practice.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <amp.h>
#include <amp_math.h>
#include <iostream>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>

using namespace std;
using namespace concurrency;
using namespace Concurrency::fast_math;
using namespace cv;
int Sharpen(const Mat& myImage, Mat& Result){
	CV_Assert(myImage.depth() == CV_8U);  // accept only uchar images

	Result.create(myImage.size(), myImage.type());
	const int nChannels = myImage.channels();
	cout << "cpu calculate start" << endl;
	for (int j = 1; j < myImage.rows - 1; ++j)
	{
		const uchar* previous = myImage.ptr<uchar>(j - 1);
		const uchar* current = myImage.ptr<uchar>(j);
		const uchar* next = myImage.ptr<uchar>(j + 1);

		uchar* output = Result.ptr<uchar>(j);
		
		for (int i = nChannels; i < nChannels * (myImage.cols - 1); ++i)
		{
			*output++ = saturate_cast<uchar>(5 * current[i]
				- current[i - nChannels] - current[i + nChannels] - previous[i] - next[i]);
		}
		
	}
	cout << "cpu calculate end" << endl;
	Result.row(0).setTo(Scalar(0));
	Result.row(Result.rows - 1).setTo(Scalar(0));
	Result.col(0).setTo(Scalar(0));
	Result.col(Result.cols - 1).setTo(Scalar(0));
	return 0;
}

int ampSharpen(const Mat& myImage, Mat& Result){
	cout << "c++amp init start" << endl;
	CV_Assert(myImage.depth() == CV_8U);
	Result.create(myImage.size(), myImage.type());
	int image_size = myImage.rows* myImage.cols*3;
	vector<int> vOrigin(image_size);
	vector<int> vResult(image_size);

	vOrigin.assign(&myImage.data[0], &myImage.data[0]+image_size);

	array_view<int, 2> avOrigin(myImage.rows, myImage.cols*3, vOrigin);
	array_view<int, 2> avResult(myImage.rows, myImage.cols*3, vResult);
	avResult.discard_data();
	cout << "c++amp init end" << endl;
	cout << "c++amp calculate start" << endl;
	concurrency::parallel_for_each(avResult.extent, [=](index<2> idx) restrict(amp){
		if (idx[0]<1 || idx[0]>avResult.extent[0]-2 || idx[1]<3 || idx[1]>avResult.extent[1]-4){
			avResult[idx] = avOrigin[idx];
		}
		else{
			avResult[idx] = fmin(fmax(avOrigin[idx] * 5 - avOrigin(idx[0] - 1, idx[1]) - avOrigin(idx[0] + 1, idx[1]) - avOrigin(idx[0], idx[1] - 3) - avOrigin(idx[0], idx[1] + 3), 0), 255);
		}
	});
	avResult.synchronize();
	cout << "c++amp calculate end" << endl;
	vector<uchar> vResult2(vResult.begin(), vResult.end());
	uchar* data = reinterpret_cast<uchar*>(vResult2.data());
	memcpy(Result.data,data, image_size);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	String dir = "test.jpg";
	Mat image = imread(dir,IMREAD_COLOR);
	Mat result, result2;
	int a = min(355, 244);
	int b = max(22, 244);
	Sharpen(image, result);
	ampSharpen(image, result2);

	namedWindow("display", WINDOW_AUTOSIZE); // Create a window for display.
	imshow("display", image); // Show our image inside it.
	//resizeWindow("display", 455, 630);

	namedWindow("result", WINDOW_AUTOSIZE); // Create a window for display.
	imshow("result", result); // Show our image inside it.
	//resizeWindow("result", 455, 630);

	namedWindow("result2", WINDOW_AUTOSIZE); // Create a window for display.
	imshow("result2", result2); // Show our image inside it.
	//resizeWindow("result2", 455, 630);

	//do_it();
	cout << "Hit any key to exit..." << endl;
	waitKey(0);

	return 0;
}