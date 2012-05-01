/*************************************

NAME:Display.c
COPYRIGHT:www.embedsky.net

 *************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "lordweb.h"


#define SCREEN_X            64  //the physical screen size in pixes,should be mutiply of 32
#define VIRTUAL_SCREEN_X    128 //the virtual screen size in pixes, should be mutiply of 32
#define SCREEN_Y            16
#define SCREEN_BUFF_SIZE    (VIRTUAL_SCREEN_X * SCREEN_Y / 8)
#define STARTPAUSETIME	    50 //start pause time = STARTPAUSETIME*TIME_BASE(in main.c)
#define STOPPAUSETIME       80 //stop pasue time = STOPPAUSETIME*TIME_BASE(in main.c)
#define SHIFTTIME	    15 //shift time = SHIFTTIME*TIME_BASE(in main.c)
#define SPACE               4  //space in screen (0-8 char)


static long g_iScreen[SCREEN_Y][VIRTUAL_SCREEN_X/32];
static int  g_xPos;
static size_t      g_iTextLen;
static int  timer;    //timer timer count

//screen shift data processing
static unsigned long Shifthand(long data1,long data2)
{
	unsigned long long temp;
	unsigned char      *ptr;
	int                i;
	int                tempg_xpos;
	temp=(unsigned long)data2;
	temp<<=32;
	temp+=(unsigned long)data1;
	ptr=(unsigned char*)&temp;
	tempg_xpos=g_xPos%32;
	for(i=0;i<4;i++)
	{
		*(ptr+tempg_xpos/8+i)=(*(ptr+tempg_xpos/8+i)<<(tempg_xpos%8))+(*(ptr+tempg_xpos/8+1+i)>>(8-tempg_xpos%8));
	}
	temp=temp>>((tempg_xpos/8)*8);
	return (temp&0x00000000ffffffff);
}

//timing call in main to flush screen
void FlushScreen(void)
{
	int fd;
	int iRow,iColum;
	unsigned long tempdata;
	unsigned char *pByte;
	timer++;
	if(timer==1)
	{
		fd=open("/dev/Lord-Control",0);
		if(fd<0){
			perror("open device Lord-Control");
			return ;
		}
		ioctl(fd, LORD_SEEK_SET, 0);
		for(iRow = 0; iRow < SCREEN_Y; iRow++) {
			for(iColum = 0; iColum < SCREEN_X/32; iColum++) {
				ioctl(fd, LORD_WRITE, g_iScreen[iRow][iColum]);
			}
		}
		ioctl(fd,LORD_END_WRITE,0);
		close(fd);
	}
	else if(timer<STARTPAUSETIME)
	{
		return ;
	}
	else if((((timer-STARTPAUSETIME)%SHIFTTIME)==0)&&(timer<(g_iTextLen+SPACE-8)*8*SHIFTTIME+STARTPAUSETIME))
	{
		if(g_iTextLen<=8)
		{
			return;
		}
		g_xPos++;
		fd=open("/dev/Lord-Control",0);
		if(fd<0){
			perror("open device Lord-Control");
			return ;
		}
		ioctl(fd,LORD_SEEK_SET,0);
		for(iRow = 0; iRow < SCREEN_Y; iRow++) {
			for(iColum = 0; iColum < SCREEN_X/32; iColum++) {
				if((iColum+g_xPos/32+1)==VIRTUAL_SCREEN_X/32)
				{
					tempdata=Shifthand(g_iScreen[iRow][iColum+g_xPos/32],0);
				}
				else if((iColum+g_xPos/32+1)>VIRTUAL_SCREEN_X/32)
				{
					tempdata=Shifthand(0,0);
				}
				else
				{
					tempdata=Shifthand(g_iScreen[iRow][iColum+g_xPos/32],g_iScreen[iRow][iColum+g_xPos/32+1]);
				}
				ioctl(fd, LORD_WRITE, tempdata);
			}
		}
		ioctl(fd,LORD_END_WRITE,0);

		close(fd);
	}
	else if(timer<(((g_iTextLen+SPACE-8)*8*SHIFTTIME)+STARTPAUSETIME+STOPPAUSETIME))
	{
		return ;
	}
	else
	{
		timer=0;
		g_xPos=0;
	}
}
void Display(const char* szText)
{
	int     fd_ascii,fd_hzk16;
	int     iRow,iColum;
	char    *pData;
	long    offset;
	const char  *pText;

	g_iTextLen=strlen(szText);
	g_xPos=0;
	timer=0;
	//Clear the screnn buff
	memset(g_iScreen, 0, sizeof(g_iScreen));
	//Set invalid file discriptor
	fd_ascii = -1;
	fd_hzk16 = -1;

	//Read data from fonts files
	pText = szText;
	for(iColum = 0; iColum < VIRTUAL_SCREEN_X/8; iColum++) {
		if (*pText == 0) //end of string
			break;
		if ((*pText & 0x80) == 0) { //ascii code
			if (fd_ascii < 0) { //font file haven't open yet
				fd_ascii = open("ascii8_16.bin", O_RDONLY); //Try to open the font file
				if (fd_ascii < 0) {
					perror("can not open ascii8_16.bin");
					break;
				}
			}  //if (fd_ascii < 0)
			offset = ((int)(*pText - ' ')) << 4;
			lseek(fd_ascii, offset, SEEK_SET);
			pData = (char*)&g_iScreen[0][0];
			pData += iColum;
			for(iRow = 0; iRow < SCREEN_Y; iRow++) {
				read(fd_ascii, pData, 1);
				pData += VIRTUAL_SCREEN_X/8;
			}
			pText++;
		}
		else { //HZK16
			if (iColum == VIRTUAL_SCREEN_X/8 - 2) //out off range
				break;
			if (fd_hzk16 < 0) { //font file haven't open yet
				fd_hzk16 = open("hzk16.bin", O_RDONLY); //Try to open the font file
				if (fd_hzk16 < 0) {
					perror("can not open hzk16.bin");
					break;
				}
			}  //if (fd_ascii < 0)
			offset = *(const unsigned char*)pText;
			offset = (offset - 0xA1) * 94;
			pText++;
			offset += *(const unsigned char*)pText;
			offset -= 0xA1;
			offset <<= 5;
			//offset = 1410*32;
			lseek(fd_hzk16, offset, SEEK_SET);
			pData = (char*)&g_iScreen[0][0];
			pData += iColum;
			for(iRow = 0; iRow < SCREEN_Y; iRow++) {
				read(fd_hzk16, pData, 2);
				pData += VIRTUAL_SCREEN_X/8;
			}
			iColum++;
			pText++;
		}
	}

	//Close fonts files
	if (fd_ascii > 0)
		close(fd_ascii);
	if (fd_hzk16 > 0)
		close(fd_hzk16);

	//    pData = (char*)&g_iScreen[0][0];
	//    for (iColum = 0; iColum < SCREEN_BUFF_SIZE; iColum++) {
	//        *pData = iColum;
	//        pData++;
	//    }
}

