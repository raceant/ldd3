/*************************************

NAME:dumplist.c
COPYRIGHT:www.ribeto.com
This program is a testing tools, It print the black/white list on console

*************************************/
#include<stdio.h>
#include<string.h>

int main(int argc, char **argv)
{
    FILE    *pFile;
    unsigned int dwVer;
    unsigned int dwRecord;
    unsigned int dwCardNo;
    unsigned char ucFalg;

	pFile = fopen("ulist.dat", "rb");
	if (pFile == 0) {
        printf("Fail to open ulist.dat\n");
		return -1;
    	}

    if (fread(&dwVer, sizeof(int), 1, pFile))
        printf("List version: %d\n", dwVer);
    dwRecord = 0;
    while(!feof(pFile)) {
        if (fread(&dwCardNo, sizeof(dwCardNo), 1, pFile) <= 0)
            break;
        if (fread(&ucFalg, sizeof(ucFalg), 1, pFile) <= 0)
            break;
        dwRecord++;
        printf("%04d:%06d:%02x\n", dwRecord, dwCardNo, ucFalg);
    }
    
        
	fclose(pFile);
    return 0;

}



