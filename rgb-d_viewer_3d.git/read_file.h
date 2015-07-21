
#ifndef _READ_FILE_H_
#define _READ_FILE_H_

#include "opencv2/opencv.hpp"

#include <iostream>
#include <vector>
#include <fstream>

inline int read_rgb(const char* file, cv::Mat& img)
{
	img = cv::imread( file );
	cv::flip(img,img,0);//Add by gaoyu 2015-7-21图像翻转,否则贴的文理时错的
	if( img.empty() ) {
		std::cout << "read_rgb(): Can not load image.\n";
		return -1;
	}

	return 0;
}

inline int read_depth(const char* file, cv::Mat& img)
{
	std::ifstream ifs(file);
	if ( !ifs.is_open() ) {
		std::cout << "read_depth(): open file failed.\n";
		return -1;
	}
	int height, width; std::vector<unsigned short> data;
	ifs.read( (char*)&height, sizeof(int) );
	ifs.read( (char*)&width, sizeof(int) );
	data.resize( height*width );
	ifs.read( (char*)&data[0], sizeof(unsigned short)*height*width );
	ifs.close();
	img.create(height, width, CV_32FC1);
	for(int i=0; i<img.rows; ++i)
		for(int j=0; j<img.cols; ++j)
			img.at<float>(i,j) = data[i*img.cols+j];

	return 0;
}

inline int read_depth_by_gaoyu(const char* file, cv::Mat& img)
{
	std::ifstream ifs(file);
	if ( !ifs.is_open() ) {
		std::cout << "read_depth(): open file failed.\n";
		return -1;
	}
	int height, width; std::vector<float> data;
	ifs.read( (char*)&height, sizeof(int) );
	ifs.read( (char*)&width, sizeof(int) );
	data.resize( height*width );
	ifs.read( (char*)&data[0], sizeof(float)*height*width );
	ifs.close();
	img.create(height, width, CV_32FC1);
	for(int i=0; i<img.rows; ++i)
		for(int j=0; j<img.cols; ++j)
			img.at<float>(i,j) = data[i*img.cols+j];
//	for(int i = img.rows - 1,k = 0; i >= 0; --i,k++)
//		for(int j=0; j<img.cols; ++j)
//			img.at<float>(k,j) = data[i*img.cols+j];


	return 0;
}

inline int read_bgmodel(const char* file, cv::Mat& img_rgb, cv::Mat& img_depth)
{
	std::ifstream ifs(file);
	if ( !ifs.is_open() ) {
		std::cout << "read_bgmodel(): open file failed.\n";
		return -1;
	}
	int height, width;
	ifs.read( (char*)&height, sizeof(int) );
	ifs.read( (char*)&width, sizeof(int) );
	img_depth.create(height, width, CV_32FC1);
	ifs.read( (char*)img_depth.data, sizeof(float)*height*width ); // dist_mean_
	ifs.seekg( sizeof(float)*height*width, ifs.cur ); // dist_sigma_
	img_rgb.create(height, width, CV_8UC3);
	ifs.read( (char*)img_rgb.data, sizeof(char)*3*height*width ); // avg_rgb_
	ifs.close();

	return 0;
}


#endif // #ifndef _READ_FILE_H_
