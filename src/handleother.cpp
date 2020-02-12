/*
 * handleother.cpp
 *
 *  Created on: 2020年2月12日
 *      Author: jet
 */

#include "handleother.hpp"

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

static int *userEncParamTab0[3];
static int *userEncParamTab1[3];

static bool enableEncoderFlag = true;
/*
static __inline__ int* getEncParamTab(int chId, int level)
{
	int nEnc = 0;
	for(int i = 0; i < 1; i++)
		nEnc += enableEncoderFlag[i];
	if(nEnc>1)
		return userEncParamTab1[chId][level];
	return userEncParamTab0[chId][level];
}

int encPrepare()
{
	ENCTRAN_InitPrm enctranInit;
	memset(&enctranInit, 0, sizeof(enctranInit));
	enctranInit.iTransLevel = 1;
	enctranInit.nChannels = 1;
	for(int chId=0; chId<1; chId++){
//				int* params = getEncParamTab(chId, iLevel);
				enctranInit.defaultEnable[chId] = enableEncoderFlag[chId];
				enctranInit.imgSize[chId] = channelsImgSize[chId];
				enctranInit.inputFPS[chId] = chInf->fps;
				enctranInit.encPrm[chId].fps = chInf->fps;
				enctranInit.encPrm[chId].bitrate = params[0];
				enctranInit.encPrm[chId].minQP = params[1];
				enctranInit.encPrm[chId].maxQP = params[2];
				enctranInit.encPrm[chId].minQI = params[3];
				enctranInit.encPrm[chId].maxQI = params[4];
				enctranInit.encPrm[chId].minQB = params[5];
				enctranInit.encPrm[chId].maxQB = params[6];
				enctranInit.srcType[chId] = APPSRC;
			}



	enctran = new CEncTrans;
	OSA_assert(enctran != NULL);


}
*/
