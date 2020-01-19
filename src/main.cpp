/*
 * main.cpp
 *
 *  Created on: 2020年1月19日
 *      Author: alex
 */


#include "MultiChVideo.hpp"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "osa_thr.h"


Mat gFullMat;
Mat gLeftMat;
Mat gRightMat;


#define LEFTCAMERA 0
#define RIGHTCAMERA 1


int callback(void *handle, int chId, int virchId, Mat frame)
{
	static unsigned int t;

	static unsigned int frameIndex = 0;

	frameIndex++;

	if(frameIndex < 30)
		return 0;

	printf("time stamp : %u , channelId = %d \n", OSA_getCurTimeInMsec(), chId);



	return 0;
	if(chId == LEFTCAMERA)
	{
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gLeftMat);
		printf("left copy need time : %u \n" , OSA_getCurTimeInMsec() - t);
	}
	else if(chId == RIGHTCAMERA)
	{
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gRightMat);
		printf("right copy need time : %u \n" , OSA_getCurTimeInMsec() - t);
	}


	return 0;
}



void* recordVideo(void *)
{
	std::string video_name = "recordDualFrame.avi";
	VideoWriter writer = VideoWriter(video_name, CV_FOURCC('H', '2', '6', '4'), 30, Size(1920*2,1080));

	Mat colorframe;
	Mat grayframe;
	static unsigned int time;
	while(1)
	{
		//cvtColor(gFullMat, grayframe, CV_YUV2GRAY_UYVY);
		cvtColor(gFullMat, colorframe, CV_YUV2BGR_YUYV);
		time = OSA_getCurTimeInMsec();
		//imshow("111" , colorframe);
		//waitKey(1);
		writer.write(colorframe);
		printf("write frame need time : %u \n" , OSA_getCurTimeInMsec() - time);
	}

	return NULL;
}


int main()
{
	struct timeval tv;

	gFullMat = Mat(1080,1920*2,CV_8UC2);
	gLeftMat = gFullMat(Rect(0,0,1920,1080));
	gRightMat = gFullMat(Rect(1920,0,1920,1080));

	MultiChVideo cap;
	cap.m_usrFunc = callback;
	cap.creat();
	cap.run(LEFTCAMERA);
	cap.run(RIGHTCAMERA);

	OSA_ThrHndl gRecord;
	//OSA_thrCreate(&gRecord, recordVideo, 0, 0, 0);

	while(1)
	{


		tv.tv_sec = 2;
		tv.tv_usec = 0;
		select(0,0,0,0,&tv);
	}

	return 0;
}
