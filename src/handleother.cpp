/*
 * handleother.cpp
 *
 *  Created on: 2020年2月12日
 *      Author: jet
 */

#include "handleother.hpp"
#include "cuda_convert.cuh"
#include "osa_sem.h"


using namespace std;
using namespace cv;
using namespace cr_osa;

static CEncTrans *enctran = NULL;

#define CORE_CHN_MAX	(4)

static int defaultEncParamTab0[3][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};
static int defaultEncParamTab1[3][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};
static unsigned char *memsI420 = NULL;

static int *userEncParamTab0[3];
static int *userEncParamTab1[3];

static bool enableEncoderFlag = true;

static OSA_BufHndl *imgQEnc = NULL;
static OSA_SemHndl *imgQEncSem = NULL;
static OSA_MutexHndl *cumutex = NULL;



static __inline__ int* getEncParamTab(int chId, int level)
{
	int nEnc = 0;

	nEnc += enableEncoderFlag;

	if(nEnc>1)
		return userEncParamTab1[level];
	return userEncParamTab0[level];
}

int encPrepare()
{
	cumutex = new OSA_MutexHndl;
	OSA_mutexCreate(cumutex);
	cuConvertInit(1, cumutex);


	int width = 1920*2;
	int height = 1080;
	cudaHostAlloc((void**)&memsI420, width*height*3/2, cudaHostAllocDefault);

	memset(userEncParamTab0, 0, sizeof(userEncParamTab0));
	memset(userEncParamTab1, 0, sizeof(userEncParamTab1));
	userEncParamTab0[0] = defaultEncParamTab0[0];
	userEncParamTab0[1] = defaultEncParamTab0[1];
	userEncParamTab0[2] = defaultEncParamTab0[2];
	userEncParamTab1[0] = defaultEncParamTab1[0];
	userEncParamTab1[1] = defaultEncParamTab1[1];
	userEncParamTab1[2] = defaultEncParamTab1[2];


	ENCTRAN_InitPrm enctranInit;
	memset(&enctranInit, 0, sizeof(enctranInit));
	int iLevel = 1;
	enctranInit.iTransLevel = iLevel;
	enctranInit.nChannels = 1;

	int chId = 0;

	int* params = getEncParamTab(chId, iLevel);
	enctranInit.defaultEnable[chId] = enableEncoderFlag;
	enctranInit.imgSize[chId] = cv::Size(1920*2,1080);
	enctranInit.inputFPS[chId] = 20;
	enctranInit.encPrm[chId].fps = 20;
	enctranInit.encPrm[chId].bitrate = params[0];
	enctranInit.encPrm[chId].minQP = params[1];
	enctranInit.encPrm[chId].maxQP = params[2];
	enctranInit.encPrm[chId].minQI = params[3];
	enctranInit.encPrm[chId].maxQI = params[4];
	enctranInit.encPrm[chId].minQB = params[5];
	enctranInit.encPrm[chId].maxQB = params[6];
	enctranInit.srcType[chId] = APPSRC;
	enctranInit.bRtp = false;

	enctran = new CEncTrans;
	OSA_assert(enctran != NULL);

	enctran->create();
	enctran->init(&enctranInit);

	imgQEnc = enctran->m_bufQue[chId];
	imgQEncSem = enctran->m_bufSem[chId];



}


int encstop()
{
	cudaFreeHost(memsI420);
	enctran->stop();
	enctran->destroy();
	return 0;
}


void encTranFrame(const Mat& img)
{
	Mat i420;
	OSA_BufInfo* info = NULL;
	unsigned char *mem = NULL;

	int chId = 0;
	enctran->scheduler(chId);

	if(mem == NULL)
		mem = memsI420;
	i420 = Mat((int)(img.rows+img.rows), img.cols, CV_8UC1, mem);

	int zoomflag = 1;
	cuConvert_async(chId, img, i420, zoomflag);

	enctran->pushData(i420, chId, V4L2_PIX_FMT_YUV420M);

}

