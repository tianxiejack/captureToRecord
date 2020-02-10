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
#include "appsrc.h"
#include "osa_buf.h"


OSA_BufCreate fisrtQuene;
OSA_BufHndl tskfirstQuene;
OSA_BufCreate secondQuene;
OSA_BufHndl tsksecondQuene;


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
		int tmp = 0;
		printf("send sem !  queneCount = %d \n" ,OSA_bufGetBufcount(&tskfirstQuene , tmp));
		leftflag = rightflag = false;
		OSA_semSignal(&semRecord);
	}

	return 0;
}


void* colorConvert(void *)
{
	int bufId;
	Mat grayframe;
	unsigned int tt;
	while(1)
	{
		OSA_semWait(&semRecord, 10*1000);

		//cvtColor(gFullMat, grayframe, CV_YUV2GRAY_UYVY);
//		Mat yyy;
//		cvtColor(gFullMat, yyy, CV_YUV2BGR_YUYV);
//		imshow("haha",yyy);
//		waitKey(1);
//		continue;

		int ret = OSA_bufGetEmpty(&tskfirstQuene, &bufId, 100*1000);
		if(ret == -1)
			continue;
		Mat colorframe = Mat(1080,1920*2,CV_8UC3,tskfirstQuene.bufInfo[bufId].virtAddr);

		tt = OSA_getCurTimeInMsec();
		cvtColor(gFullMat, colorframe, CV_YUV2BGR_YUYV);
		printf("cvtColor need time : %u \n" , OSA_getCurTimeInMsec() - tt );

		OSA_bufPutFull(&tskfirstQuene, bufId);
	}
	return NULL;
}


void* recordVideo(void *)
{
	std::string video_name = "recordDualFrame.avi";
	VideoWriter writer = VideoWriter(video_name, CV_FOURCC('X', '2', '6', '4'), 15, Size(1920*2,1080),1);

	int bufId;
	unsigned int tt;
	while(1)
	{
		int ret = OSA_bufGetFull(&tskfirstQuene, &bufId, 100*1000);
		if(ret == -1)
			continue;

		Mat colorframe = Mat(1080,1920*2,CV_8UC3,tskfirstQuene.bufInfo[bufId].virtAddr);


		//imshow("111" , colorframe);
		//waitKey(1);
		//pushData(gFullMat);
		tt = OSA_getCurTimeInMsec();
		writer.write(colorframe);
		printf("write frame need time : %u \n" , OSA_getCurTimeInMsec() - tt);

		OSA_bufPutEmpty(&tskfirstQuene, bufId);
	}
	//writer.release();
	return NULL;
}


int main()
{
	struct timeval tv;

	fisrtQuene.numBuf = 299;
	for (int i = 0; i < fisrtQuene.numBuf; i++)
	{
		fisrtQuene.bufVirtAddr[i] = (void*)malloc(1920*2*1080*3);
		OSA_assert(fisrtQuene.bufVirtAddr[i] != NULL);
	}
	OSA_bufCreate(&tskfirstQuene, &fisrtQuene);

//	secondQuene.numBuf = 100;
//	for (int i = 0; i < secondQuene.numBuf; i++)
//	{
//		secondQuene.bufVirtAddr[i] = (void*)malloc(1920*2*1080*3);
//		OSA_assert(secondQuene.bufVirtAddr[i] != NULL);
//	}
//	OSA_bufCreate(&tsksecondQuene, &secondQuene);


	OSA_semCreate(&semRecord, 1, 0);

	OSA_ThrHndl gColorConvert;
	OSA_thrCreate(&gColorConvert, colorConvert, 0, 0, 0);

	OSA_ThrHndl gRecord;
	OSA_thrCreate(&gRecord, recordVideo, 0, 0, 0);

	gFullMat = Mat(1080,1920*2,CV_8UC2);
	gLeftMat = gFullMat(Rect(0,0,1920,1080));
	gRightMat = gFullMat(Rect(1920,0,1920,1080));

	//gstInit();



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

	OSA_bufDelete(&tskfirstQuene);
//	OSA_bufDelete(&tsksecondQuene);

	return 0;
}
