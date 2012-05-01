/*************************************

NAME:main.c
COPYRIGHT:www.ribeto.com
This is the main module of RIBETO vehicle managment system.

*************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "lordweb.h"
#include "codec.h"
#include "misc.h"
#include "Ulist.h"


#define BUFLEN 255
#define CARMAN_PORT 10083
#define MAX_OFF_RECORD  10000

/*Time constant value definition*/
#define TIMER_BASE           (20)    //Timer time base

#define TIME_SUBSCRIBER     (600)   //If both RFID card and vechile is detected in TIME_CARD(ms), 
                                    //We can suggest that a valid subscriber is present.
#define TIME_NO_VEHICLE     (500)   //Only if non vehicle detected condition is kept in at least TIME_NEW_VEHICLE(ms),
                                    //We can suggest that the vehicle realy moved away.
#define TIME_GATE_OPEN      (1000)  //Gate open signal length
#define TIME_VP_REQ         (100)   //Interval between VEHICLE_PRESENT_REQs(sending)
#define TIME_CP_REQ         (200)   //Interval between CARD_PRESENT_REQs(sending)
#define TIME_HEART_BEAT     (5000)  //Interval between HEART_BEAT_REQs(sending)

#define TIME_RESET TIME_HEART_BEAT  //Interval between RESET_REQs(sending)
#define LINK_FAIL_TIMER     (2 * TIME_HEART_BEAT)
#define VEHICLE_PRSNT_TIMER     (100)

#define INIT_MSG(msg,type) \
    memset(&msg, 0, sizeof(msg));\
    msg.ucVersion = 0x10;\
    msg.ucSource = g_config.ucSource;\
    msg.ucType = type

typedef struct OffRecord_tag
{
    int             iSec;
    unsigned int    dwCardNo;
}OffRecord_T;

//configure the station when the network is down,whether work atuo or not work (if true the station wrok else not work) 
typedef enum {ATUOTRUE,ATUOFLASE}E_AtuoWork;

//configure the station when vehicle is out and  only have card signal whether report to host PC(true is report else not report)
typedef enum{REPORTTRUE,REPORTFLASE}E_Report;

static unsigned int s_dwOffRecord;
static OffRecord_T s_tOffRecord[MAX_OFF_RECORD];



extern void Display(const char* szText);//display the usr string
extern void FlushScreen(void);        //flush screen cycle
extern int open_port(int port_no);
extern int set_opt(int fd, int nBits, int nSpeed, char nEvent, int nStop);


//station information
static struct {
    unsigned char ucSource;        //device id
    E_AtuoWork    atuoflag;        //atuo work flag
    E_Report      reportflag;      //if only have card signed whether report flag 
    struct sockaddr_in peeraddr;   //sever ip address
    struct sockaddr_in localaddr;  //local ip address
}g_config;

//read and analysis the configure fle
static void ReadConfigureFile(void)
{
    FILE *file;
    size_t len;
    int    i;
    int    j;
    char   *ptr;
    char buf[128];
    file=fopen("/etc/carman.conf","r");
    if(file==NULL)
    {
	perror("have no carman.conf file or open this error.\n");
	perror("we will use default configure:\n");
	perror("device   id = 100\n");
	perror("not net atuo work = TRUE\n");
	perror("only card signal report = TRUE\n");
	perror("server   ip = 192.168.1.120\n");
	perror("local    ip = 192.168.1.121\n");
	g_config.ucSource=100;
	g_config.atuoflag=ATUOTRUE;
	g_config.reportflag=REPORTTRUE;
	inet_pton(AF_INET,"192.168.1.120",&g_config.peeraddr.sin_addr);
	inet_pton(AF_INET,"192.168.1.121",&g_config.localaddr.sin_addr);
	return ;
    }
    while(!feof(file))
    {
        memset(buf,128,0);
        if (!fgets(buf,128,file))
        {
            break;
        }
        if((buf[0]=='#')||(buf[0]=='\n'))//the first char is '#' or '\n' ,we needn't analysis this line
        {
            continue;
        }
        else
        {
            len=strlen(buf);
            for(i=0;i<len;i++)//delete space
            {
                if((buf[i]==' ')||(buf[i]=='\t'))
                {
                    for(j=i;j<len;j++)
                    {
                        buf[j]=buf[j+1];
                    }
                    i--;
                }
            }
            len=strlen(buf);
            if(buf[len-1]=='\n')//if the last char is \n we will using 0 replace
            {
                buf[len-1]='\0';
            }
            for(i=0;i<len;i++)       //convert case
            {
                if((buf[i]>='A')&&(buf[i]<='Z'))
                {
                    buf[i]=buf[i]+32;
                }
            }
            if ('\0' == buf[0])
            {
                continue;
            }
            ptr=strtok(buf,"=");
            if(!strcmp("deviceid",ptr))
            {
                g_config.ucSource=(unsigned char)atoi(strtok(NULL,"="));
            }
            else if(!strcmp("serverip",ptr))
            {
                inet_pton(AF_INET,strtok(NULL,"="),&g_config.peeraddr.sin_addr);
            }
            else if(!strcmp("localip",ptr))
            {
                inet_pton(AF_INET,strtok(NULL,"="),&g_config.localaddr.sin_addr);
            }
			else if(!strcmp("atuowork",ptr))
				{
					if(!strcmp("true",strtok(NULL,"=")))
						{
							g_config.atuoflag=ATUOTRUE;
						}
					else
						{
							g_config.atuoflag=ATUOFLASE;
						}
				}
			else if(!strcmp("atuoreport",ptr))
				{
					if(!strcmp("true",strtok(NULL,"=")))
						{
							g_config.reportflag=REPORTTRUE;
						}
					else
						{
							g_config.reportflag=REPORTFLASE;
						}
				}
            else
            {  
                continue;
            }
        }
    }
    
}

//send message to sever
static int SendMessage(const void* pMsg, int sock)
{
    unsigned char ucBuff[2048];
    int iLength;
    int iRv;

    iLength =  sizeof(ucBuff);
    if (A1_ERROR_OK != A1Encode(ucBuff, &iLength, (const A1Msg_T*)pMsg))//encode the msg
{
        return 0;
}
    return sendto(sock, ucBuff, iLength, 0
        , (struct sockaddr*)&g_config.peeraddr
        , sizeof(struct sockaddr_in));
}


/******************************************************************************/

int main(int argc, char **argv)
{
    int     sockfd;
    int     rs485_fd;
    char    rx_buff[256];
    int     nRead;
    unsigned char ucResetAckFlag;       //whether RESET_REQ is acked(eg.RESET_ACK received)
    unsigned char ucCardAckFlag;        //whether CARD_PRESENT_REQ is acked(eg.CARD_PRESENT_ACK received) 
    unsigned char ucCurVehicles;        //This value increased when a new vehicle is detected
    unsigned char ucAckVehicles;        //The last acked vehicle(eg.VEHICLE_PRESENT_ACK for which is received)
    unsigned char ucOffTranID;          //The TransID used in OFFLINE_TRAN_REQ message
     
	unsigned int dwTickCurrent;	        //The current time(in ticks)
	unsigned int dwTickVehicleOn;       //The latest time when a vehicle is detected;
	unsigned int dwTickCard; 	        //The latest time when a RFID card is detected;
	unsigned int dwTickVehicleReq;	    //The last time when a vehicle detected condition is reported
	                                    //(VEHICLE_PRESENT_REQ message is sent)
	unsigned int dwTickTimer;            //Timer time
    unsigned int dwTickCardReq;         //The last time when a card detected condition is reported
                                        //(CARD_PRESENT_REQ message is sent)
    unsigned int dwTickHeartbeatReq;    //When last HEART_BEAT_REQ message is sent
    unsigned int dwTickHeartbeatAck;    //When last HEART_BEAT_ACK message is received
    unsigned int dwTickResetReq;        //When last RESET_REQ message is sent
    unsigned int dwGateOpenReq;         //When last GATE_OPEN_REQ message is received
    unsigned int dwCurCardNo;           //The number of the last detected card.
    A1Msg_T tA1Msg;

    //global value initialization
    s_dwOffRecord = 0;

    //Read configure information from file.
    memset(&g_config, 0, sizeof(g_config));

    ReadConfigureFile();

    //Read black/white lists from file.
    ReadListFromFile();
    
    //Display welcom infomation on led screen 
    Display("\xBB\xB6\xD3\xAD\xB9\xE2\xC1\xD9 Ribeto");
    //Close the gate
    excute_cmd(LORD_DOWN_ON);
    excute_cmd(LORD_UP_OFF);

    //Initiate the main socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<0){
		perror("socket creating err in main\n");
		return 1;
	}
	g_config.peeraddr.sin_family=AF_INET;
	g_config.peeraddr.sin_port=htons(CARMAN_PORT);
	g_config.localaddr.sin_family=AF_INET;
	g_config.localaddr.sin_port=htons(CARMAN_PORT);
	if(bind(sockfd, (struct sockaddr*)&g_config.localaddr
        , sizeof(struct sockaddr_in))<0) {	
		perror("bind local address err in main!\n");
		return 2;
	}

    //open rs485
    rs485_fd = open_port(2);
    if (rs485_fd < 0)
        return 3;
    set_opt(rs485_fd, 8, 1200, 'N', 1);

 
    ucCurVehicles = 0;
    ucAckVehicles = -1;
    ucResetAckFlag = 0;
    ucCardAckFlag = 0;
    ucOffTranID = 0;
    dwCurCardNo = 0;
    
	dwTickCurrent = GetTicks();
	dwTickVehicleOn = dwTickCurrent - TIME_NO_VEHICLE;
	dwTickCard = dwTickCurrent - TIME_SUBSCRIBER;
    dwTickVehicleReq = dwTickCurrent - TIME_VP_REQ;
    dwTickHeartbeatReq = dwTickCurrent - TIME_HEART_BEAT;
    dwTickHeartbeatAck = dwTickCurrent;
    dwTickResetReq = dwTickCurrent - TIME_RESET;
    dwTickCardReq = dwTickCurrent - TIME_CP_REQ;
    dwGateOpenReq = dwTickCurrent - TIME_GATE_OPEN;
    dwTickTimer   = dwTickCurrent ;

    while(1) {
        usleep(8000); //wait some time
		dwTickCurrent = GetTicks();
        //Test if a vehicle can be detected currently
		if (test_vd108()) {
            if (dwTickCurrent - dwTickVehicleOn >= TIME_NO_VEHICLE)
                ucCurVehicles++;
			dwTickVehicleOn = dwTickCurrent;
        }
        //Test if there is a RFID card.
        //Todo:Here we treat RS485 protocol as packet-oriented,(actually it is not in the case)
        //So this should be rewritten.
		nRead = read(rs485_fd, rx_buff, sizeof(rx_buff)-1);
        if (4 == nRead) {
			dwTickCard = dwTickCurrent;
			dwCurCardNo = rx_buff[0];
			dwCurCardNo <<= 8;
			dwCurCardNo += rx_buff[1];
			dwCurCardNo <<= 8;
			dwCurCardNo += rx_buff[2];
			dwCurCardNo <<= 8;
			dwCurCardNo += rx_buff[3];
            ucCardAckFlag = 0;
        }

        //Keep sending heart beat message every TIME_HEART_BEAT(ms)
        if (dwTickCurrent - dwTickHeartbeatReq >= TIME_HEART_BEAT) {
            HeartBeatReq_T  tReq;
            INIT_MSG(tReq,HEART_BEAT_REQ);
            SendMessage(&tReq, sockfd);
            dwTickHeartbeatReq = dwTickCurrent;
        }
	    //Timer
	    if(dwTickCurrent - dwTickTimer >= TIMER_BASE ) {
	        FlushScreen();
	        dwTickTimer=dwTickCurrent;
	    }
        //Close gate if no GATE_OPEN_REQ received in TIME_GATE_OPEN(ms)
        if (dwTickCurrent -dwGateOpenReq >= TIME_GATE_OPEN)
            excute_cmd(LORD_UP_OFF);

        if (dwTickCurrent - dwTickHeartbeatAck <= LINK_FAIL_TIMER) { //link OK, work in On-line mode
            //Keep sending RESET_REQ message every TIME_RESET(ms) if not acked.
            if (!ucResetAckFlag && (dwTickCurrent - dwTickResetReq >= TIME_RESET)) {
                ResetReq_T      tReq;
                INIT_MSG(tReq,RESET_REQ);
                SendMessage(&tReq,sockfd);
                dwTickResetReq = dwTickCurrent;
            }
            //If a vehicle is nearby, keep notifing the host PC every TIME_VP_REQ until acked.
            if ((dwTickCurrent - dwTickVehicleOn <= TIME_SUBSCRIBER)      
                && (dwTickCurrent - dwTickVehicleReq >= TIME_VP_REQ)
                && (ucCurVehicles != ucAckVehicles)){
                VehiclePresentReq_T tReq;
                INIT_MSG(tReq,VEHICLE_PRESENT_REQ);
                tReq.ucTransId = ucCurVehicles;
                SendMessage(&tReq,sockfd);
                dwTickVehicleReq = dwTickCurrent;
            }
            //If card is nearby and atuoreport is true notifing the host PC ever TIME_CP_REQ.
            if (g_config.reportflag == REPORTTRUE
                && (dwTickCurrent - dwTickCard <= TIME_SUBSCRIBER) //recently a card is detected
                && (dwTickCurrent - dwTickCardReq >= TIME_CP_REQ)
                && (0 == ucCardAckFlag)){
                    CardPresentReq_T tReq;
                    INIT_MSG(tReq,CARD_PRESENT_REQ);
                    tReq.dwCardNo = dwCurCardNo;
                    SendMessage(&tReq,sockfd);
                    dwTickCardReq = dwTickCurrent;
                    //printf("Tx:CARD_PRESENT_REQ %d\n", dwTickCurrent);
            }
            else
            {
                //If both vehicle and card is nearby, keep notifing the host PC every TIME_CP_REQ.
                if ((dwTickCurrent - dwTickCard <= TIME_SUBSCRIBER) //recently a card is detected
                    && (dwTickCurrent - dwTickVehicleOn <= TIME_SUBSCRIBER) //recently a vehicle is detected
                    && (dwTickCurrent - dwTickCardReq >= TIME_CP_REQ)
                    && (0 == ucCardAckFlag)){
                        CardPresentReq_T tReq;
                        INIT_MSG(tReq,CARD_PRESENT_REQ);
                        tReq.dwCardNo = dwCurCardNo;
                        SendMessage(&tReq,sockfd);
                        dwTickCardReq = dwTickCurrent;
                        //printf("Tx:CARD_PRESENT_REQ %d\n", dwTickCurrent);
                }
            }
            //Report offline transaction
            if (s_dwOffRecord) {
                OffLineTransReq_T   tReq;
                struct tm           *pTm;
                INIT_MSG(tReq,OFFLINE_TRAN_REQ);
                tReq.ucTransId = ucOffTranID;
                tReq.dwCardNo = s_tOffRecord[s_dwOffRecord-1].dwCardNo;
                pTm = gmtime((time_t*)&s_tOffRecord[s_dwOffRecord-1].iSec);
                tReq.tTime.ucYear = pTm->tm_year - 100;
                tReq.tTime.ucMonth = pTm->tm_mon;
                tReq.tTime.ucDay = pTm->tm_mday;
                tReq.tTime.ucHour = pTm->tm_hour;
                tReq.tTime.ucMinute = pTm->tm_min;
                tReq.tTime.ucSecond = pTm->tm_sec;
                SendMessage(&tReq,sockfd);
            }
        }
        else {//link failure, work in Off-line mode
        	if(g_config.atuoflag==ATUOTRUE)
        	{
                if ((dwTickCurrent - dwTickCard <= TIME_SUBSCRIBER)
                    && (dwTickCurrent - dwTickVehicleOn <= TIME_SUBSCRIBER)
                    && (ucCurVehicles != ucAckVehicles)){
                        Subscriber_T    *pSubscriber;

                        ucAckVehicles = ucCurVehicles;
                        pSubscriber = GetSubscriber(dwCurCardNo);
                        if (pSubscriber && (pSubscriber->ucFlags & CARD_VAL_MASK)
                            && !(pSubscriber->ucFlags & CARD_TMP_MASK)) {
                            struct timeval  tv;
                            struct timezone tz;
                            //Save offline transaction information
                            gettimeofday(&tv, &tz); //get current time an time zone
                            if (s_dwOffRecord < MAX_OFF_RECORD) {
                                s_tOffRecord[s_dwOffRecord].dwCardNo = dwCurCardNo;
                                s_tOffRecord[s_dwOffRecord].iSec = tv.tv_sec;
                                s_dwOffRecord++;
                            }
                            dwGateOpenReq = dwTickCurrent;
                            excute_cmd(LORD_UP_ON); //open the gate
                     }
                }
            }
        }


		nRead = recv(sockfd, rx_buff, sizeof(rx_buff)-1, MSG_DONTWAIT);
        if (nRead <= 0)
            continue;
        if(A1_ERROR_OK != A1Decode(rx_buff, nRead, &tA1Msg))
            continue;
        switch(tA1Msg.tHeatBeat.ucType) {
            /****messages about black/white lists operation******/
            case LIST_VERSION_INQ: {
                    ListVerRsp_T tAck;
                    INIT_MSG(tAck,LIST_VERSION_RSP);
                    tAck.dwListVersion = GetListVersion();
                    SendMessage(&tAck,sockfd);
                }
                break;
            case LIST_VERSION_REQ: {
                    ListVerAck_T tAck;
                    SetListVersion(((ListVersionReq_T*)&tA1Msg)->dwListVersion);
                    RebuildList();
                    WriteListToFile();
                    INIT_MSG(tAck,LIST_VERSION_ACK);
                    SendMessage(&tAck,sockfd);
                }
                break;
            case LIST_RESET_REQ: {
                    ListResetAck_T tAck;
                    ResetList();
                    INIT_MSG(tAck,LIST_RESET_ACK);
                    SendMessage(&tAck,sockfd);
                }
                break;
            case LIST_MODIFY_REQ: {
                    int             iTmp;
                    ListModifyAck_T tAck;
                    CardInfo_T      *pCardInfo;
                    pCardInfo = ((ListModifyReq_T*)&tA1Msg)->tCardInfo;
                    for(iTmp = 0; iTmp < ((ListModifyReq_T*)&tA1Msg)->ucRecordNum; iTmp++)
                        ModifySubscriber(pCardInfo[iTmp].dwCardNo
                        ,pCardInfo[iTmp].ucValid
                        ,pCardInfo[iTmp].ucTmp);
                    INIT_MSG(tAck,LIST_MODIFY_ACK);
                    SendMessage(&tAck,sockfd);
                }
                break;
                
            /****messages about link status operation******/
            case HEART_BEAT_REQ: {
                    //response with "heartbeat ack"
                    HeartBeatAck_T   tAck;
                    memset(&tAck, 0, sizeof(tAck));
                    tAck.ucVersion = 0x10;
                    tAck.ucSource = g_config.ucSource;
                    tAck.ucType = HEART_BEAT_ACK;
                    SendMessage(&tAck,sockfd);
                }
                break;
            case HEART_BEAT_ACK:
                dwTickHeartbeatAck = dwTickCurrent; //Reset link guard timer
                break;
            case RESET_ACK:
                ucResetAckFlag = 1;
                break;

            case SET_CLOCK_REQ: {
                    struct timeval  tv;
                    struct timezone tz;
                    struct tm       tm;
                    SetClockReq_T *pReq = (SetClockReq_T*)&tA1Msg;
                    SetClockAck_T   tAck;

                    gettimeofday(&tv, &tz); //get current time an time zone
                    memset(&tm, 0, sizeof(tm));
                    tm.tm_year = 100 + pReq->tTime.ucYear;
                    tm.tm_mon = pReq->tTime.ucMonth;
                    tm.tm_mday = pReq->tTime.ucDay;
                    tm.tm_hour = pReq->tTime.ucHour;
                    tm.tm_min = pReq->tTime.ucMinute;
                    tm.tm_sec = pReq->tTime.ucSecond;
                    tv.tv_sec = mktime(&tm);
                    settimeofday(&tv, &tz);
                    INIT_MSG(tAck,SET_CLOCK_ACK);
                    SendMessage(&tAck,sockfd);
                }
                break;
            case CARD_PRESENT_ACK:
                //printf("RX:CARD_PRESENT_ACK\n");
                ucCardAckFlag = 1;
                break;
            case VEHICLE_PRESENT_ACK:
                ucAckVehicles = ((VehiclePresentAck_T*)&tA1Msg)->ucTransId;
                break;
            case GATE_OPEN_REQ: {
                    HeartBeatAck_T   tAck;
                    dwGateOpenReq = dwTickCurrent;
                    excute_cmd(LORD_UP_ON); //open the gate
                    INIT_MSG(tAck,GATE_OPEN_ACK);
                    SendMessage(&tAck,sockfd);
                }
                break;
            case LED_SHOW_REQ: {
                    LedShowAck_T   tAck;
                    Display(((LedShowReq_T*)&tA1Msg)->szShow);
                    INIT_MSG(tAck,LED_SHOW_ACK);
                    SendMessage(&tAck,sockfd);
                }
                break;
            case OFFLINE_TRAN_ACK:
                if (ucOffTranID == ((OffLineTransAck_T*)&tA1Msg)->ucTransId) {
                    ucOffTranID++;
                    if (s_dwOffRecord)
                    s_dwOffRecord--;
                }
                break;
        } //switch
        //timer_poll();
        //read(vd108_fd,&vd108_stat,sizeof(vd108_stat));
        //printf("vd108_stat=%d\n", vd108_stat);
        //timer1();
    }
    close(rs485_fd);
}