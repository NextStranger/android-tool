/*
 * DataFrame.cpp
 *			用于对数据按照MCU协议进行组包,增加校验.
 */

#include "DataFrame.h"
#include <string.h>
#include <log/log.h>


#define CT_CMD_POS 			0		//第1字节为CT和 CMD
#define SMD_POS 				1	
#define SN_PRIORITY_RET 	2		//第2字节为SN和PRORITY,应答帧时为RET
#define DATA_POS 				3 		//第3byte为数据
#define CHECKSUM_SIZE    2		//校验数据的大小

/* CRC high byte */
const uint8_t CRCH[] =
{
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* CRC low byte */
const uint8_t CRCL[] =
{
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
	0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
	0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
	0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
	0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
	0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
	0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
	0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
	0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
	0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
	0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
	0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
	0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
	0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
	0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
	0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
	0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
	0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/*****************************************************************************
** 函数名:  CRC16校验函数
** 函数功能: 16位用查表法计算CRC16校验码
** 输入参数: *pu8Msg -需要计算CRC的数据
** 输入参数: u8Datalen -数据长度
** 返回数据:  CRC16校验结果
*****************************************************************************/
uint16_t GetCRC(uint8_t *pu8Msg, uint16_t u8DataLen)
{
	uint8_t u8CRCHi = 0xFF;  /* High byte of CRC initialized */
	uint8_t u8CRCLo = 0xFF;  /* Low byte of CRC initialized */
	uint8_t u8Index;         /* Index into CRC lookup table */

	while (u8DataLen--)
	{
		u8Index = u8CRCHi ^ *pu8Msg++ ;   /* calculate the CRC */
		u8CRCHi = u8CRCLo ^ CRCH[u8Index];
		u8CRCLo = CRCL[u8Index] ;
	}

	return (u8CRCHi << 8 | u8CRCLo) ;
}



DataFrame::DataFrame():mCT(0),mCMD(0),mSMD(0),mSN(0),mPRIORITY_RET(0),mDATA(0),mDataLen(0),
										mCheckSum(0),
										mFrameLen(0){
	memset(mFrame,0,DATA_FRAME_BUFFER);
}

/**
 * 清除数据包中的所有数据
 */
void DataFrame::clearData(){
	mCT =0;
	mCMD =0;
	mSMD = 0;
	mSN = 0;
	mPRIORITY_RET = 0;
	mDATA = 0;
	mDataLen =0;
	mCheckSum =0;
	mFrameLen = 0;
	memset(mFrame,0,DATA_FRAME_BUFFER);
}

DataFrame::~DataFrame() {
}
/**
 * 创建一个数据帧或者应答帧格式详见协议
 *@param  uint8_t CT  命令类型
 *@param  uint8_t CMD  命令主ID
 *@param  uint8_t SMD  命令子ID
 *@param uint8_t SN  命令流水号,这个从控制器获取
 *@param  uint8_t PRIORITY_RET 如果是数据帧为命令优先级,
 * 			如果是应答帧时为返回结果00 检验成功 0x01为检验失败,0x02为不能识别命令，0x03参数错误
 *@param  uint8_t *DATA 发送的数据
 *@param  uint8_t  DataLen  发送的数据长度
 */
void DataFrame::setData(uint8_t CT,uint8_t CMD,uint8_t SMD,uint8_t SN,uint8_t PRIORITY_RET,uint8_t *DATA,uint8_t  DataLen){
	clearData();
	int len  = DATA_POS+DataLen+CHECKSUM_SIZE;

	if(DATA_FRAME_BUFFER<(len)){
		return;
	}
	mFrameLen = len;
	mCT = CT;
	mCMD = CMD;
	mSMD = SMD;
	mSN = SN;
	mPRIORITY_RET = PRIORITY_RET;
	mFrame[CT_CMD_POS]=((mCT<<7)&0x80) | (mCMD&0x7F);
	mFrame[SMD_POS]=mSMD;
	mFrame[SN_PRIORITY_RET]= ((mSN<<4)&0xF0) | (PRIORITY_RET & 0x0F);
	mDATA = mFrame+DATA_POS;
	mDataLen = DataLen;
	if(NULL != DATA){
		memcpy(mDATA,DATA,mDataLen);
	}else{
		mDataLen = 0;
	}
	mCheckSum = checkSum();

	uint8_t hightCS =(mCheckSum>>8) ;
	uint8_t lowCS =(mCheckSum <<8) >>8;
#ifdef LOG_DEBUG
	printf("hightcs=0x%02x,lowcs=0x%02x",hightCS,lowCS);
#endif
	//memcpy(mFrame+DATA_POS+DataLen,&mCheckSum,CHECKSUM_SIZE);//写入校验值
	memcpy(mFrame+DATA_POS+DataLen,&hightCS,1);//写入校验值
	memcpy(mFrame+DATA_POS+DataLen+1,&lowCS,1);//写入校验值
	//printfData();
}
/**
 * 设置一个原始的数据包，存放从串口接收的数据
 *@param  uint8_t *Frame 数据帧
 *@param  uint8_t FrameLen 数据帧长度
 */
void DataFrame::setFrameData(const uint8_t *Frame,uint8_t FrameLen){
	clearData();
	int len   = FrameLen;
	if(DATA_FRAME_BUFFER<(len)){
		printf("setFrameData  fail  DATA_FRAME_BUFFER >= len %d",len);
		return;
	}

	mFrameLen = FrameLen;
	memcpy(mFrame,Frame,FrameLen);
	mCT = (mFrame[CT_CMD_POS]>>7)&0x1;
	mCMD = (mFrame[CT_CMD_POS]&0x7F);
	mSMD = mFrame[SMD_POS];
	mPRIORITY_RET = mFrame[SN_PRIORITY_RET]&0x0F;
	mSN =  (mFrame[SN_PRIORITY_RET]>>4)&0x0F;
	mDATA = mFrame+DATA_POS;
	mDataLen = FrameLen - DATA_POS - CHECKSUM_SIZE;

	int higtindex = FrameLen-CHECKSUM_SIZE;
	uint8_t hightCS = mFrame[higtindex];
	uint8_t lowCS =mFrame[higtindex+1];
	uint16_t tmp = hightCS<<8;
	tmp = tmp|lowCS;

	mCheckSum = tmp;
	//memcpy(&mCheckSum,mFrame+(FrameLen-CHECKSUM_SIZE),CHECKSUM_SIZE);
	uint16_t checksum = checkSum();
#ifdef LOG_DEBUG
	printf("hightcs=0x%02x,lowcs=0x%02x,ch=0x%02x, checksum =0x%02x",hightCS,lowCS,tmp,checksum);
#endif
	//printfData();
	if(mCheckSum != checksum){
		clearData();
		printf("setFrameData  fail  mCheckSum != checksum ");
		return;
	}
}
/**
 *创建一个ACK的应答帧数据
*@param uint8_t CMD cmd主命令
*@param uint8_t SMD smd子命令
*@param uint8_t SN 发送流水号
 */
void DataFrame::setAck(uint8_t CMD,uint8_t SMD,uint8_t SN){
	setData(1,CMD,SMD,SN,0,0,0);
}

/**
 * 用于存放从上层应用传下来的数据,这里不带checksum，SN应该为0
 * uint8_t *Frame 数据帧
 * uint8_t FrameLen 数据帧长度
 */
void DataFrame::setFrameDataNoCheckSum(const uint8_t *Frame,uint8_t FrameLen){
	clearData();
	mFrameLen = FrameLen+CHECKSUM_SIZE;
		if(DATA_FRAME_BUFFER<(mFrameLen+CHECKSUM_SIZE)){
			mFrameLen = 0 ;
			printf("setFrameDataNoCheckSum  fail  DATA_FRAME_BUFFER mFrameLen+CHECKSUM_SIZE ");

			return;
	}
	memcpy(mFrame,Frame,FrameLen);
	mCT = (mFrame[CT_CMD_POS]>>7)&0x1;
	mCMD = (mFrame[CT_CMD_POS]&0x7F);
	mSMD = mFrame[SMD_POS];
	mPRIORITY_RET = mFrame[SN_PRIORITY_RET]&0x0F;
	mSN = 0;
	mDATA = mFrame+DATA_POS;
	mDataLen = FrameLen-DATA_POS;
	mCheckSum = checkSum();
	memcpy(mFrame+DATA_POS+mDataLen,&mCheckSum,CHECKSUM_SIZE);

	//printf("setFrameDataNoCheckSum------DATA_FRAME_BUFFER mFrameLen+CHECKSUM_SIZE ");
    //printfData();
}
/**
 * 对已有的数据包设置流水号sn，并计算校验值
 * @param uint8_t sn计算校验值
 */
void DataFrame::setSn(uint8_t sn){
	mSN=sn;
	mFrame[SN_PRIORITY_RET]= ((mSN<<4)&0xF0) | (mPRIORITY_RET & 0x0F);
	mCheckSum = checkSum();
	uint8_t hightCS =(mCheckSum>>8) ;
	uint8_t lowCS =(mCheckSum <<8) >>8;
	//printf("---> setsn---hightcs=0x%02x,lowcs=0x%02x",hightCS,lowCS );
	memcpy(mFrame+DATA_POS+mDataLen,&hightCS,1);//写入校验值
	memcpy(mFrame+DATA_POS+mDataLen+1,&lowCS,1);//写入校验值


	//memcpy(mFrame+DATA_POS+mDataLen,&mCheckSum,CHECKSUM_SIZE);

}

/**
 * 计算从 CT到DATA的校验值
 * @return 返回CRC校验值
 */
uint16_t DataFrame::checkSum(){

	return GetCRC(mFrame,DATA_POS+mDataLen);

}

uint16_t DataFrame::checksum( uint8_t *Frame,uint16_t FrameLen)
{
	return GetCRC(Frame,FrameLen);

}


void   DataFrame::printfData(){
	printf("mCT=0x%02x",mCT);
	printf("mCMD=0x%02x,mSMD=0x%02x",mCMD,mSMD);
    //printf("mSMD=0x%02x",mSMD);
	printf("mSN=0x%02x",mSN);
	printf("mPRIORITY_RET=0x%02x",mPRIORITY_RET);
	printf("mDataLen=0x%02x",mDataLen);
	printf("mFrameLen=0x%02x",mFrameLen);

	int i = 0;

	printf("mCheckSum =0x%02x",mCheckSum);
	printf("mCheckSum =%d",mCheckSum);

	for(i=0; i < mDataLen; i++){
		printf("data[%d]=0x%02x",i,mDATA[i]);
	}

	for(i=0; i < mFrameLen; i++){
			printf("framedata[%d]=0x%02x",i,mFrame[i]);
		}

}

//void DataFrame::printfData2(){
//	printf("mCMD=0x%02,xmSMD=0x%02x",mCMD,mSMD);
//	int i = 0;
//	for(i=0; i < mDataLen; i++){
//		printf("data[%d]=0x%02x",i,mDATA[i]);
//		printf("*************************");
//	}
//}

