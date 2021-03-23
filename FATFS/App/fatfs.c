/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "fatfs.h"

uint8_t retUSER;    /* Return value for USER */
char USERPath[4];   /* USER logical drive path */
FATFS USERFatFS;    /* File system object for USER logical drive */
FIL USERFile;       /* File object for USER */

/* USER CODE BEGIN Variables */
#include <stdio.h>
#include "ffconf.h"
#include "w25qxx_qspi.h"
#define BOOTLOADER_POS "0:update/ROS.bin"
UINT fnum,BIN_Size;
uint8_t readbuff[1];
uint8_t writebuff[1] = {'0'};
uint8_t Update_Buffer[4096];
//IAP升级
void fatfs_test(void)
{
	//挂载文件系统
    f_mount(&USERFatFS,USERPath,1);
	//打开校检文件,检测是否需要升级
    f_open(&USERFile,"0:update/verify.txt",FA_READ|FA_WRITE);
    f_read(&USERFile,readbuff,1,&fnum);
	if(readbuff[0] == '1'){/*如果需要升级*/
		f_lseek(&USERFile,0);
		f_write(&USERFile,writebuff,1,&fnum);
		f_close(&USERFile);//关闭校检文件
		//===================================开始升级===================================//
		f_open(&USERFile,BOOTLOADER_POS,FA_READ);//打开APP的bin文件
		BIN_Size = f_size(&USERFile);
		int freq = BIN_Size>>12;
		int remain = BIN_Size%4096;
		printf("Update ing...\r\n");
		
		for(int i=0;i<freq;i++)
		{
			W25qxx_EraseSector(i<<12);
			f_read(&USERFile,Update_Buffer,4096,&fnum);
			W25qxx_WriteNoCheck(Update_Buffer,i<<12,4096);
			f_lseek(&USERFile,(i+1)<<12);
//			printf("update %d\r\n",i);
		}
		if(remain != 0)
		{
			W25qxx_EraseSector(freq<<12);
			f_lseek(&USERFile,freq<<12);
			f_read(&USERFile,Update_Buffer,remain,&fnum);
			W25qxx_Write(Update_Buffer,freq<<12,remain);
		}
		printf("update over!\r\n");
		f_close(&USERFile);
	}else{                 /*如果不需要升级*/
		f_close(&USERFile);//关闭校检文件
	}
	
}
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the USER driver ###########################*/
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
	
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
