#ifndef _ULIST_H_
#define _ULIST_H_

#define MAX_SUBSCRIBER  10000

#define CARD_VAL_MASK    (0x80)
#define CARD_TMP_MASK    (0x01)

typedef struct Subscriber_tag
{
    unsigned int dwCardNo;
    unsigned char ucFlags;
}Subscriber_T;

extern int ReadListFromFile(void);
extern unsigned int GetListVersion(void);
extern void RebuildList(void);
extern int ModifySubscriber(unsigned int dwCardNo, unsigned char ucValid, unsigned char ucTmp);
extern int WriteListToFile(void);
extern void ResetList(void);
extern Subscriber_T* GetSubscriber(unsigned int dwCardNo);





#endif
