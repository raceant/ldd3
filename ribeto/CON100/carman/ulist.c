/*************************************

NAME:ulist.c
COPYRIGHT:www.ribeto.com
This module maintain black/white lists of RIBETO vehicle managment system.

*************************************/
#include<stdio.h>
#include<string.h>
#include "Ulist.h"

unsigned char   s_ucCleanFlag = 0;          //whether the list is clean(eg:all records are in order and valid)
unsigned int    s_dwListVersion;                //The version of balck/white list
unsigned int    s_dwSubscribers;                //The number of subscribers kept in s_tSubscriber[]
Subscriber_T    s_tSubscriber[MAX_SUBSCRIBER];  //Information of subscribers(main part of balck/white list)

//return "0",if success.
int ReadListFromFile(void)
{
    FILE    *pFile;
    int     rv;

    ResetList();
	pFile = fopen("ulist.dat", "rb");
	if (pFile == 0) //file not exist
		return 0;
    
    rv = 0;
    if (!feof(pFile))
        fread(&s_dwListVersion, sizeof(int), 1, pFile);
    else 
        rv = -1;
    while(!feof(pFile)) {
        if (s_dwSubscribers >= MAX_SUBSCRIBER)
            break;
        if (fread(&s_tSubscriber[s_dwSubscribers].dwCardNo, 
            sizeof(s_tSubscriber[0].dwCardNo), 1, pFile) <= 0)
            break;
        if (fread(&s_tSubscriber[s_dwSubscribers].ucFlags, 
            sizeof(s_tSubscriber[0].ucFlags), 1, pFile) <= 0)
            break;
        s_dwSubscribers++;
    }
        
	fclose(pFile);
    return rv;
}

unsigned int GetListVersion(void)
{
    return s_dwListVersion;
}

void SetListVersion(unsigned int dwVersionNew)
{
    s_dwListVersion = dwVersionNew;
}

void ResetList(void)
{
    s_dwListVersion = -1; //invalid version num
    s_dwSubscribers = 0; //no members in list
    s_ucCleanFlag = 1;
}

//return "0",if success.
int WriteListToFile(void)
{
    FILE    *pFile;
    unsigned int dw1;
    
	pFile = fopen("ulist.dat", "wb+");
    if (pFile == 0)
        return -1;
    
    fwrite(&s_dwListVersion, sizeof(int), 1, pFile);//write the list version on the first location of the ulist.dat file
    if (s_dwSubscribers > MAX_SUBSCRIBER)
        s_dwSubscribers = MAX_SUBSCRIBER;
    for (dw1 = 0; dw1 < s_dwSubscribers; dw1++) {
        fwrite(&s_tSubscriber[dw1].dwCardNo, 
            sizeof(s_tSubscriber[0].dwCardNo), 1, pFile);
        fwrite(&s_tSubscriber[dw1].ucFlags, 
            sizeof(s_tSubscriber[0].ucFlags), 1, pFile);
    }
    fclose(pFile);
    return 0;
}

Subscriber_T* GetSubscriber(unsigned int dwCardNo)
{
    unsigned int dw1;
    Subscriber_T    *pSubscriber;
    //if (!s_ucCleanFlag) {
        pSubscriber = s_tSubscriber;
        for (dw1 = 0; dw1 < s_dwSubscribers; dw1++) {
            if (pSubscriber->dwCardNo == dwCardNo)
                return pSubscriber;
            pSubscriber++;
        }
   // }
    return 0;
}

void RebuildList(void)
{
    unsigned int    dwTmp;
    Subscriber_T    *pSubscriber1;
    Subscriber_T    *pSubscriber2;
    unsigned int dw2;
    
    if (s_ucCleanFlag)
        return;

     s_ucCleanFlag = 1;
     //remove "invalid" record
     pSubscriber2 = s_tSubscriber;
     dwTmp = 0;
     for (pSubscriber1 = s_tSubscriber; pSubscriber1 !=  s_tSubscriber + s_dwSubscribers; pSubscriber1++) {
        while (0 == (pSubscriber1->ucFlags & CARD_VAL_MASK)) { //skip the "invalid" record
            pSubscriber1++;
            if (pSubscriber1 ==  s_tSubscriber + s_dwSubscribers) 
                break;
        }
        if (pSubscriber1 ==  s_tSubscriber + s_dwSubscribers) 
            break;
        if (pSubscriber2 != pSubscriber1)
            memcpy(pSubscriber2, pSubscriber1, sizeof(Subscriber_T));
        pSubscriber2++;
        dwTmp++;
    }
    s_dwSubscribers = dwTmp;
}

//return "0" if success
int ModifySubscriber(unsigned int dwCardNo, unsigned char ucValid, unsigned char ucTmp)
{
    Subscriber_T    *pSubscriber;
    pSubscriber = GetSubscriber(dwCardNo);

    if ((0 != ucValid) && (0 == pSubscriber)) { //Add new record
        //Don't have enough memory,rebuild the list and see if we can make some room.
        if (s_dwSubscribers >= MAX_SUBSCRIBER) 
            RebuildList();
        if (s_dwSubscribers >= MAX_SUBSCRIBER) //Still don't have enough memory
            return -1;
        pSubscriber = &s_tSubscriber[s_dwSubscribers];
        s_dwSubscribers++;
        pSubscriber->dwCardNo = dwCardNo;
        s_ucCleanFlag = 0;
    }
    if (pSubscriber) { //Modify an old record
        if (ucValid) 
            pSubscriber->ucFlags |= CARD_VAL_MASK;
        else {
            pSubscriber->ucFlags &= ~CARD_VAL_MASK;
            s_ucCleanFlag = 0;
        }
        if (ucTmp) 
            pSubscriber->ucFlags |= CARD_TMP_MASK;
        else
            pSubscriber->ucFlags &= ~CARD_TMP_MASK;
    }
    return 0;
}



