#include <string.h>
#include "codec.h"

static void PutDword(unsigned char* pWork, unsigned int dwData)
{
    *pWork = dwData >> 24;
    pWork++;
    *pWork = (dwData >> 16) & 0xFF;;
    pWork++;
    *pWork = (dwData >> 8) & 0xFF;
    pWork++;
    *pWork = dwData & 0xFF;
}

static unsigned int GetDword(const unsigned char* pWork)
{
    unsigned int dwData;
    dwData = *pWork;
    dwData <<= 8;
    pWork++;
    dwData |= *pWork;
    dwData <<= 8;
    pWork++;
    dwData |= *pWork;
    dwData <<= 8;
    pWork++;
    dwData |= *pWork;
    return dwData;
}

//encode the information A1Msg_T,the using socket to traslation
int A1Encode(unsigned char *buff, int *length, const A1Msg_T* pMsg)
{
    unsigned char   *pWork;

    if (pMsg->tHeatBeat.ucVersion != 0x10)
        return A1_ERROR_VERSION;
    if (*length < 4)
        return A1_BUFF_SHORTAGE;
    pWork = buff;
    *pWork = pMsg->tHeatBeat.ucVersion;
    pWork++;
    *pWork = pMsg->tHeatBeat.ucSource;
    pWork++;
    *pWork = pMsg->tHeatBeat.ucType;
    pWork++;
    switch(pMsg->tHeatBeat.ucType) {
        case HEART_BEAT_REQ:
        case HEART_BEAT_ACK:
        case SET_CLOCK_ACK:
        case CARD_PRESENT_ACK:
        case GATE_OPEN_REQ:
        case GATE_OPEN_ACK:
        case LIST_VERSION_INQ:
        case LIST_VERSION_ACK:
        case LIST_RESET_REQ:
        case LIST_RESET_ACK:
        case LED_SHOW_ACK:
        case RESET_REQ:
        case RESET_ACK:
        case LIST_MODIFY_ACK:
            break;
        case SET_CLOCK_REQ:
            if (*length < 10)
                return A1_BUFF_SHORTAGE;
            memcpy(pWork, &((SetClockReq_T*)pMsg)->tTime, sizeof(A1Time_T));
            pWork += sizeof(A1Time_T);
            break;
        case CARD_PRESENT_REQ:
            if (*length < 9)
                return A1_BUFF_SHORTAGE;
            *pWork = ((CardPresentReq_T*)pMsg)->ucLowBattery ? 1 : 0;
            pWork++;
            PutDword(pWork, ((CardPresentReq_T*)pMsg)->dwCardNo);
            pWork += 4;
            break;
        case LIST_VERSION_RSP:
        case LIST_VERSION_REQ:
            if (*length < 8)
                return A1_BUFF_SHORTAGE;
            PutDword(pWork, ((ListVerRsp_T*)pMsg)->dwListVersion);
            pWork += 4;
            break;
        case LIST_MODIFY_REQ: {
                int         iRecord;
                CardInfo_T  *pCardInfo;
                if (*length < 5 + 5 * (((ListModifyReq_T*)pMsg)->ucRecordNum))
                    return A1_BUFF_SHORTAGE;
                *pWork = ((ListModifyReq_T*)pMsg)->ucRecordNum;
                pWork++;
                pCardInfo = ((ListModifyReq_T*)pMsg)->tCardInfo;
                for (iRecord = 0; iRecord < ((ListModifyReq_T*)pMsg)->ucRecordNum; iRecord++) {
                    *pWork = pCardInfo->ucValid ? 0x80 : 0;
                    *pWork |= pCardInfo->ucTmp ? 0x01 : 0;
                    pWork++;
                    PutDword(pWork, pCardInfo->dwCardNo);
                    pWork+=4;
                    pCardInfo++;
                }
            }
            break;
        case OFFLINE_TRAN_REQ:
            if (*length < 15)
                return A1_BUFF_SHORTAGE;
            *pWork = ((OffLineTransReq_T*)pMsg)->ucTransId;
            pWork++;
            PutDword(pWork, ((OffLineTransReq_T*)pMsg)->dwCardNo);
            pWork += 4;
            memcpy(pWork, &((OffLineTransReq_T*)pMsg)->tTime, sizeof(A1Time_T));
            pWork += sizeof(A1Time_T);
            break;
        case OFFLINE_TRAN_ACK:
        case VEHICLE_PRESENT_REQ:
        case VEHICLE_PRESENT_ACK:
            if (*length < 5)
                return A1_BUFF_SHORTAGE;
            *pWork = ((OffLineTransAck_T*)pMsg)->ucTransId;
            pWork++;
            break;
        case LED_SHOW_REQ: {
                int iStrLen;
                iStrLen = strlen(((LedShowReq_T*)pMsg)->szShow) + 1;
                if (*length < 4 + iStrLen)
                    return A1_BUFF_SHORTAGE;
                memcpy(pWork, ((LedShowReq_T*)pMsg)->szShow, iStrLen);
                pWork += iStrLen;
            }
            break;
        default:
            return A1_ERROR_TYPE;    
    }
    *pWork = 0; //Todo: Caculate CRC
    pWork++;
    *length = (int)(pWork - buff);
    return A1_ERROR_OK;
}

//decode the information of A1Msg_T recieve from socket
int A1Decode(const unsigned char *buff, int length, A1Msg_T* pMsg)
{
    const unsigned char   *pWork;

    if (*buff != 0x10)
        return A1_ERROR_VERSION;
    if (length < 4)
        return A1_BUFF_SHORTAGE;
    //Todo CRC check
    pWork = buff;
    pMsg->tHeatBeat.ucVersion = *pWork;
    pWork++;
    pMsg->tHeatBeat.ucSource = *pWork;
    pWork++;
    pMsg->tHeatBeat.ucType = *pWork;
    pWork++;
    switch(pMsg->tHeatBeat.ucType) {
        case HEART_BEAT_REQ:
        case HEART_BEAT_ACK:
        case SET_CLOCK_ACK:
        case CARD_PRESENT_ACK:
        case GATE_OPEN_REQ:
        case GATE_OPEN_ACK:
        case LIST_VERSION_INQ:
        case LIST_VERSION_ACK:
        case LIST_RESET_REQ:
        case LIST_RESET_ACK:
        case LED_SHOW_ACK:
        case RESET_REQ:
        case RESET_ACK:
        case LIST_MODIFY_ACK:
            break;
        case SET_CLOCK_REQ:
            if (length < 10)
                return A1_BUFF_SHORTAGE;
            memcpy(&((SetClockReq_T*)pMsg)->tTime, pWork, sizeof(A1Time_T));
            pWork += sizeof(A1Time_T);
            break;
        case CARD_PRESENT_REQ:
            if (length < 9)
                return A1_BUFF_SHORTAGE;
            ((CardPresentReq_T*)pMsg)->ucLowBattery = *pWork & 0x01;
            pWork++;
            ((CardPresentReq_T*)pMsg)->dwCardNo = GetDword(pWork);
            pWork += 4;
            break;
        case LIST_VERSION_RSP:
        case LIST_VERSION_REQ:
            if (length < 8)
                return A1_BUFF_SHORTAGE;
            ((ListVerRsp_T*)pMsg)->dwListVersion = GetDword(pWork);
            pWork += 4;
            break;
        case LIST_MODIFY_REQ: {
                int         iRecord;
                CardInfo_T  *pCardInfo;
                ((ListModifyReq_T*)pMsg)->ucRecordNum = *pWork;
                pWork++;
                if (length < 5 + 5 * (((ListModifyReq_T*)pMsg)->ucRecordNum))
                    return A1_BUFF_SHORTAGE;
                pCardInfo = ((ListModifyReq_T*)pMsg)->tCardInfo;
                for (iRecord = 0; iRecord < ((ListModifyReq_T*)pMsg)->ucRecordNum; iRecord++) {
                    pCardInfo->ucValid = (*pWork & 0x80) ? 1 : 0;
                    pCardInfo->ucTmp = (*pWork & 0x01) ? 1 : 0;
                    pWork++;
                    pCardInfo->dwCardNo = GetDword(pWork);
                    pWork+=4;
                    pCardInfo++;
                }
            }
            break;
        case OFFLINE_TRAN_REQ:
            if (length < 15)
                return A1_BUFF_SHORTAGE;
            ((OffLineTransReq_T*)pMsg)->ucTransId = *pWork;
            pWork++;
            ((OffLineTransReq_T*)pMsg)->dwCardNo = GetDword(pWork);
            pWork += 4;
            memcpy(&((OffLineTransReq_T*)pMsg)->tTime, pWork, sizeof(A1Time_T));
            pWork += sizeof(A1Time_T);
            break;
        case OFFLINE_TRAN_ACK:
        case VEHICLE_PRESENT_REQ:
        case VEHICLE_PRESENT_ACK:
            if (length < 5)
                return A1_BUFF_SHORTAGE;
            ((OffLineTransAck_T*)pMsg)->ucTransId = *pWork;
            pWork++;
            break;
        case LED_SHOW_REQ: {
                int iStrLen;
                iStrLen = strlen(pWork) + 1;
                if ((length < 4 + iStrLen) || (iStrLen > MAX_SHOW_LENGTH + 1))
                    return A1_BUFF_SHORTAGE;
                memcpy(((LedShowReq_T*)pMsg)->szShow, pWork, iStrLen);
                pWork += iStrLen;
            }
            break;
        default:
            return A1_ERROR_TYPE;    
    }
    return A1_ERROR_OK;
}
