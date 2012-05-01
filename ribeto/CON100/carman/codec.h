#ifndef _CODEC_H_
#define _CODEC_H_


#define A1_ERROR_OK         0
#define A1_ERROR_UNKNOW     1
#define A1_ERROR_CRC        2   //CRC error
#define A1_ERROR_VERSION    3   //Unsuportted message version
#define A1_ERROR_TYPE       4   //Unsuportted message type
#define A1_BUFF_SHORTAGE    5   //No enough buff

#define HEART_BEAT_REQ		(0x01)
#define HEART_BEAT_ACK		(0x02)
#define SET_CLOCK_REQ		(0x03)
#define SET_CLOCK_ACK		(0x04)
#define CARD_PRESENT_REQ	(0x05)
#define CARD_PRESENT_ACK	(0x06)
#define GATE_OPEN_REQ		(0x07)
#define GATE_OPEN_ACK		(0x08)
#define LIST_VERSION_INQ	(0x09)
#define LIST_VERSION_RSP	(0x0A)
#define LIST_VERSION_REQ	(0x0B)
#define LIST_VERSION_ACK	(0x0C)
#define LIST_RESET_REQ		(0x0D)
#define LIST_RESET_ACK		(0x0E)
#define LIST_MODIFY_REQ		(0x0F)
#define LIST_MODIFY_ACK		(0x10)
#define OFFLINE_TRAN_REQ	(0x11)
#define OFFLINE_TRAN_ACK	(0x12)
#define VEHICLE_PRESENT_REQ	(0x13)
#define VEHICLE_PRESENT_ACK	(0x14)
#define LED_SHOW_REQ		(0x15)
#define LED_SHOW_ACK		(0x16)
#define RESET_REQ			(0x17)
#define RESET_ACK			(0x18)

#define MAX_LIST_RECORD     (256)
#define MAX_SHOW_LENGTH     (32)
//Message IE
typedef struct A1Time_tag
{
    unsigned char    ucYear;
    unsigned char    ucMonth;
    unsigned char    ucDay;
    unsigned char    ucHour;
    unsigned char    ucMinute;
    unsigned char    ucSecond;
}A1Time_T;

typedef struct CardInfo_tag
{
    unsigned char   ucValid;
    unsigned char   ucTmp;
    unsigned int    dwCardNo;
}CardInfo_T;


//Meaages
typedef struct HeartBeatReq_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
}HeartBeatReq_T,HeartBeatAck_T,SetClockAck_T,CardPresentAck_t,GateOpenReq_T
 ,GateOpenAck_T,ListVerInq_T,ListVerAck_T,ListResetReq_T,ListResetAck_T
 ,ListModifyAck_T,LedShowAck_T
 ,ResetReq_T,ResetAck_T;

typedef struct SetClockReq_tag
{
    unsigned char   ucVersion;
    unsigned char   ucSource;
    unsigned char   ucType;
    A1Time_T        tTime;
}SetClockReq_T;

typedef struct CardPresentReq_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
    unsigned char    ucLowBattery;
    unsigned int     dwCardNo;
}CardPresentReq_T;

typedef struct ListVerInq_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
    unsigned int     dwListVersion;
}ListVerRsp_T,ListVersionReq_T;

typedef struct ListModifyReq_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
    unsigned char    ucRecordNum;
    CardInfo_T       tCardInfo[MAX_LIST_RECORD];
}ListModifyReq_T;

typedef struct OffLineTransReq_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
    unsigned char    ucTransId;
    unsigned int     dwCardNo;
    A1Time_T         tTime;
}OffLineTransReq_T,VehiclePresentReq_T,VehiclePresentAck_T;

typedef struct OffLineTransAck_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
    unsigned char    ucTransId;
}OffLineTransAck_T;

typedef struct LedShowReq_tag
{
    unsigned char    ucVersion;
    unsigned char    ucSource;
    unsigned char    ucType;
    char             szShow[MAX_SHOW_LENGTH + 1];
}LedShowReq_T;

typedef union
{
    struct HeartBeatReq_tag     tHeatBeat;
    struct SetClockReq_tag      tSetClockReq;
    struct CardPresentReq_tag   tCardPresentReq;
    struct ListVerInq_tag       tListVerInq;
    struct ListModifyReq_tag    tListModifyReq;
    struct OffLineTransReq_tag  tOffLineTransReq;
    struct OffLineTransAck_tag  tOffLineTransAck;
    struct LedShowReq_tag       tLedShowReq;
}A1Msg_T;

extern int A1Encode(unsigned char *buff, int *length, const A1Msg_T* pMsg);
extern int A1Decode(const unsigned char *buff, int length, A1Msg_T* pMsg);

#endif
