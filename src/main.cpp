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
#include "osa_sem.h"

Mat gFullMat;
Mat gLeftMat;
Mat gRightMat;


#define LEFTCAMERA 0
#define RIGHTCAMERA 1

OSA_SemHndl semRecord;

int callback(void *handle, int chId, int virchId, Mat frame)
{
	static unsigned int t;
	static unsigned int frameIndex = 0;

	static bool leftflag = false , rightflag = false;

	frameIndex++;

	if(frameIndex < 60)
		return 0;

	//printf("time stamp : %u , channelId = %d \n", OSA_getCurTimeInMsec(), chId);

	if(chId == LEFTCAMERA)
	{
		leftflag = true;
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gLeftMat);
		//printf("left copy need time : %u \n" , OSA_getCurTimeInMsec() - t);
	}
	else if(chId == RIGHTCAMERA)
	{
		rightflag = true;
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gRightMat);
		//printf("right copy need time : %u \n" , OSA_getCurTimeInMsec() - t);
	}

	if(leftflag && rightflag)
	{
		printf("send sem \n");
		leftflag = rightflag = false;
		OSA_semSignal(&semRecord);
	}

	return 0;
}



void* recordVideo(void *)
{
	std::string video_name = "recordDualFrame.avi";
	VideoWriter writer = VideoWriter(video_name, CV_FOURCC('X', '2', '6', '4'), 15, Size(1920*2,1080));

	Mat colorframe;
	Mat grayframe;
	static unsigned int time;
	while(1)
	{
		OSA_semWait(&semRecord, 10*1000);
		//cvtColor(gFullMat, grayframe, CV_YUV2GRAY_UYVY);
		cvtColor(gFullMat, colorframe, CV_YUV2BGR_YUYV);
		time = OSA_getCurTimeInMsec();
		//imshow("111" , colorframe);
		//waitKey(1);
		writer.write(colorframe);
		printf("write frame need time : %u \n" , OSA_getCurTimeInMsec() - time);
	}
	writer.release();
	return NULL;
}


int main()
{
	struct timeval tv;


	OSA_semCreate(&semRecord, 1, 0);

	OSA_ThrHndl gRecord;
	OSA_thrCreate(&gRecord, recordVideo, 0, 0, 0);

	gFullMat = Mat(1080,1920*2,CV_8UC2);
	gLeftMat = gFullMat(Rect(0,0,1920,1080));
	gRightMat = gFullMat(Rect(1920,0,1920,1080));

	MultiChVideo cap;
	cap.m_usrFunc = callback;
	cap.creat();
	cap.run(LEFTCAMERA);
	cap.run(RIGHTCAMERA);


	while(1)
	{


		tv.tv_sec = 2;
		tv.tv_usec = 0;
		select(0,0,0,0,&tv);
	}

	return 0;
}
