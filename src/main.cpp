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
#include "handleother.hpp"


#define LEFTCAMERA 0
#define RIGHTCAMERA 1


OSA_BufCreate fisrtQuene;
OSA_BufHndl tskfirstQuene;
OSA_BufCreate secondQuene;
OSA_BufHndl tsksecondQuene;


Mat gFullMat;
Mat gLeftMat;
Mat gRightMat;


OSA_SemHndl semRecord;

bool stopInsertBuffer = false;


int callback(void *handle, int chId, int virchId, Mat frame)
{
	static unsigned int t;
	static unsigned int frameIndex = 0;

	static bool leftflag = false , rightflag = false;

	//printf("time stamp : %u , channelId = %d \n", OSA_getCurTimeInMsec(), chId);
	//if(stopInsertBuffer)
	//	return 0;

	if(chId == LEFTCAMERA)
	{
		leftflag = true;
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gLeftMat);
		//printf("left copy need time : %u \n" , OSA_getCurTimeInMsec() - t);

		rightflag = true;
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gRightMat);
		//printf("right copy need time : %u \n" , OSA_getCurTimeInMsec() - t);
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
		int bufnum = OSA_bufGetBufcount(&tskfirstQuene , tmp);
		//printf("send sem !  queneCount = %d \n" ,bufnum);
		if(bufnum > 297)
		{
			//stopInsertBuffer = true;
		}

		leftflag = rightflag = false;

		frameIndex++;

		if(frameIndex%3)
		{
			OSA_semSignal(&semRecord);
			frameIndex = 0;
		}
	}

	return 0;
}


int pictureCall(int chId,Mat frame)
{
	static unsigned int t;
	static unsigned int frameIndex = 0;

	static bool leftflag = false , rightflag = false;

	//printf("time stamp : %u , channelId = %d \n", OSA_getCurTimeInMsec(), chId);
	if(stopInsertBuffer)
		return 0;

	if(chId == LEFTCAMERA)
	{
		leftflag = true;
		t = OSA_getCurTimeInMsec();
		frame.copyTo(gLeftMat);
		//printf("left copy need time : %u \n" , OSA_getCurTimeInMsec() - t);


		rightflag = true;
		frame.copyTo(gRightMat);
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

		leftflag = rightflag = false;

		frameIndex++;

		if(frameIndex%10)
		{
			OSA_semSignal(&semRecord);
			frameIndex = 0;
		}
	}

	return 0;
}

void* colorConvert(void *)
{
	int bufId;
	Mat grayframe;
	unsigned int tt;
	static unsigned int frameindex = 0;
	while(1)
	{

		OSA_semWait(&semRecord, 1000*1000);
		if(frameindex < 50)
		{
			cvtColor(gFullMat, grayframe, CV_YUV2GRAY_UYVY);
			imshow("haha",grayframe);
			waitKey(1);
		}
		else
			encTranFrame(gFullMat);

		frameindex ++;

#if 0
		int ret = OSA_bufGetEmpty(&tskfirstQuene, &bufId, 1000*1000);
		if(ret == -1)
			continue;
		Mat colorframe = Mat(1080,1920*2,CV_8UC2,tskfirstQuene.bufInfo[bufId].virtAddr);

#if 1
		tt = OSA_getCurTimeInMsec();
		//cvtColor(gFullMat, colorframe, CV_YUV2BGR_YUYV);
		gFullMat.copyTo(colorframe);
		printf("cvtColor need time : %u \n" , OSA_getCurTimeInMsec() - tt );
#endif

		OSA_bufPutFull(&tskfirstQuene, bufId);

#endif
	}
	return NULL;
}


void* recordVideo(void *)
{
	std::string video_name = "recordDualFrame.avi";
	VideoWriter writer = VideoWriter(video_name, CV_FOURCC('X', '2', '6', '4'), 15, Size(1920*2,1080),1);

	int bufId;
	unsigned int tt;
	int tmp = 0;
	int bufnum;

	while(1)
	{
		if(stopInsertBuffer)
		{
			bufnum = OSA_bufGetBufcount(&tskfirstQuene , tmp);
			if(bufnum < 3)
				stopInsertBuffer = false;
		}

		int ret = OSA_bufGetFull(&tskfirstQuene, &bufId, 100*1000);

		if(ret == -1)
			continue;

		Mat colorframe = Mat(1080,1920*2,CV_8UC2,tskfirstQuene.bufInfo[bufId].virtAddr);


		//imshow("111" , colorframe);
		//waitKey(1);


		//pushData(colorframe);

#if 0
		tt = OSA_getCurTimeInMsec();
		writer.write(colorframe);
		printf("write frame need time : %u \n" , OSA_getCurTimeInMsec() - tt);
#endif
		OSA_bufPutEmpty(&tskfirstQuene, bufId);
	}
	//writer.release();
	return NULL;
}


int main(int argc,char* argv[])
{
	struct timeval tv;

	encPrepare();
	gstInit();


	fisrtQuene.numBuf = 30;
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




#if 1
	MultiChVideo cap;
	cap.m_usrFunc = callback;
	cap.creat();
	cap.run(LEFTCAMERA);
	//cap.run(RIGHTCAMERA);

#else

	Mat leftpicture = imread("left.jpg");
	Mat rightpicture = imread("right.jpg");


#endif

	while(1)
	{

#if 1
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		select(0,0,0,0,&tv);
#else
		tv.tv_sec = 0;
		tv.tv_usec = 32*1000;
		select(0,0,0,0,&tv);
		pictureCall(LEFTCAMERA,leftpicture);

		tv.tv_sec = 0;
		tv.tv_usec = 1*1000;
		select(0,0,0,0,&tv);
		pictureCall(RIGHTCAMERA,rightpicture);

#endif
	}



	OSA_bufDelete(&tskfirstQuene);
//	OSA_bufDelete(&tsksecondQuene);

	//encstop();

	return 0;
}
