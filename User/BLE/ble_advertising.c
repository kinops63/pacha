/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    User/BLE/ble_advertising.c
  * @author  Clément Macquet
  * @brief
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "ble.h"
#include "pacha_beacon.h"
#include "ble_advertising.h"
#include "system_stm32wbxx.h"
#include "main.h"


/* Exported types ------------------------------------------------------------*/


/* Private types -------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ADVERTISING_INTERVAL_INCREMENT (16)
#define DEFAULT_BEACON_SEC                 (1000000/CFG_TS_TICK_VAL)  /**< 1s */
#define DEFAULT_BLE_DEVICE_NAME_PREFIX "\tPACHA"
#define DEFAULT_BLE_DEVICE_NAME        "PACHA_00000000"

#define BLE_GAP_ADV_MAX_SIZE              (255)
#define ADV_DATA_SIZE_MAX                 (BLE_GAP_ADV_MAX_SIZE - 3u)   /* -> 31 for BLE 4.2, 255 for BLE 5.0 */
#define SR_ADV_DATA_SIZE_MAX              (BLE_GAP_ADV_MAX_SIZE - 3u)   /* -> 31 for BLE 4.2, 255 for BLE 5.0 */
#define ADV_SENS_DATA_SIZE                ((uint8_t)0u)
#define ADV_INFO_SIZE                     ((uint8_t)0u)
#define DEVICE_NAME_SIZE_MAX			  (31)
#define DEVICE_NAME_SIZE                  (uint8_t)(sizeof(DEFAULT_BLE_DEVICE_NAME))

/* Private variables ---------------------------------------------------------*/
uint8_t ble_adv;
uint8_t TimerBLEAdv_Id;
BLE_Adv_InitTypeDef BLE_Adv_InitStruct;

static char g_sLocalName[DEVICE_NAME_SIZE_MAX] = {};
static uint8_t g_u8SizeLocalName = 0;
static uint8_t g_u8LocalNameDefined = 0;

/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint8_t u8StatusbitmapToUint(BLE_Adv_InitTypeDef *p_sBLE_Adv_Init);

/* Private functions ---------------------------------------------------------*/
static tBleStatus BLE_Adv_Init(BLE_Adv_InitTypeDef *p_sBLE_Adv_Init)
{
  tBleStatus ret = BLE_STATUS_SUCCESS;
  uint16_t p_u16AdvertisingInterval = (BLE_Adv_InitStruct.u16AdvertisingInterval * ADVERTISING_INTERVAL_INCREMENT / 10);
  uint8_t u8BleStatusByte = u8StatusbitmapToUint(p_sBLE_Adv_Init);
 // uint8_t u8BleStatusByte = 0;
  uint8_t service_data[] =
  {
    14,                                                                      /*< Length. */
    AD_TYPE_SERVICE_DATA,                                                    /*< Service Data data type value. */
    0xAA, 0xFE,                                                              /*< 16-bit Eddystone UUID. */
    0x5D,                                                                    /*< Pacha frame type. A définir*/
	(p_sBLE_Adv_Init->u8CalibratedTxPower & 0xFF),
	(p_sBLE_Adv_Init->u16UnlockingCode & 0xFF00) >> 8,                        /*< Pacha Unlocking Code. */
    (p_sBLE_Adv_Init->u16UnlockingCode & 0x00FF),
    (p_sBLE_Adv_Init->u16KeyCode & 0xFF00) >> 8,                             /*< Pacha uKeyCode. */
    (p_sBLE_Adv_Init->u16KeyCode & 0x00FF),
    (p_sBLE_Adv_Init->u32Uptime & 0xFF000000) >> 24,                          /*< Time since power-on or reboot. */
    (p_sBLE_Adv_Init->u32Uptime & 0x00FF0000) >> 16,
    (p_sBLE_Adv_Init->u32Uptime & 0x0000FF00) >> 8,
    (p_sBLE_Adv_Init->u32Uptime & 0x000000FF),
	(u8BleStatusByte & 0xFF)
  };

  /* Set local name*/
  if (g_u8LocalNameDefined == 0)
  {
	  BLE_SetPachaBleID();
	  g_u8LocalNameDefined = 1;
  }


  /* Disable scan response. */
  hci_le_set_scan_response_data(0, NULL);
  //uint8_t taillechaine = sizeof(g_sLocalName);

/* Put the device in a non-connectable mode. */
  ret = aci_gap_set_discoverable(ADV_NONCONN_IND,                          				/*< Advertise as non-connectable, undirected. */
		  	  	  	  	  	  	 p_u16AdvertisingInterval, p_u16AdvertisingInterval, 	/*< Set the advertising interval as 700 ms (0.625 us increment). */
                                 GAP_PUBLIC_ADDR, NO_WHITE_LIST_USE,       			    /*< Use the public address, with no white list. */
								 g_u8SizeLocalName, (uint8_t*) &g_sLocalName,        /*< Do not use a local name. */
                                 0, NULL,                                 			 	/*< Do not include the service UUID list. */
                                 0, 0);                                    				/*< Do not set a slave connection interval. */

  if (ret != BLE_STATUS_SUCCESS)
  {
    return ret;
  }

  /* Remove the TX power level advertisement (this is done to decrease the packet size). */
  //ret = aci_gap_delete_ad_type(AD_TYPE_TX_POWER_LEVEL);

  if (ret != BLE_STATUS_SUCCESS)
  {
    return ret;
  }

  /* Update the service data. */
  ret = aci_gap_update_adv_data(sizeof(service_data), service_data);

  if (ret != BLE_STATUS_SUCCESS)
  {
    return ret;
  }

  return ret;
}

static void BLE_Adv(void)
{
	tBleStatus ret;

  if(ble_adv == TRUE)
  { /* Advertising of TLM */
    ret = aci_gap_set_non_discoverable();

    if (ret != BLE_STATUS_SUCCESS)
    {
      while(1);
    }


    /* No OTA */
    BLE_Adv_Init(&BLE_Adv_InitStruct);

    ble_adv = FALSE;

    /* Wait 1s */
    HW_TS_Start(TimerBLEAdv_Id, DEFAULT_BEACON_SEC);
  }
  else
  { /* Advertising of UUID or URL */
    ret = aci_gap_set_non_discoverable();

    if (ret != BLE_STATUS_SUCCESS)
    {
      while(1);
    }


    /* No OTA */
    BLE_Adv_Init(&BLE_Adv_InitStruct);
    ble_adv = TRUE;
    /* 10s of URL advertise */
    HW_TS_Start(TimerBLEAdv_Id, DEFAULT_BEACON_SEC * 10);
  }
}

static uint8_t u8StatusbitmapToUint(BLE_Adv_InitTypeDef *p_sBLE_Adv_Init)
{
	uint8_t l_u8BleStatusByte=0;
	l_u8BleStatusByte += p_sBLE_Adv_Init->sBleStatusByte.u4State << 4;
	return l_u8BleStatusByte;
}

/* Exported functions --------------------------------------------------------*/
void BLE_Adv_Process(void)
{

  BLE_Adv_InitStruct.u16AdvertisingInterval = ADVERTISING_INTERVAL_IN_MS;
  BLE_Adv_InitStruct.u16UnlockingCode = 0x3C3D;
  BLE_Adv_InitStruct.u16KeyCode = 0x4E4F;
  BLE_Adv_InitStruct.u32Uptime = 543210;
  BLE_Adv_InitStruct.u8CalibratedTxPower = CALIBRATED_TX_POWER_AT_0_M;

  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &(TimerBLEAdv_Id), hw_ts_SingleShot, BLE_Adv);

  BLE_Adv_Init(&BLE_Adv_InitStruct);
  ble_adv = TRUE;

  /* 10s of URL advertise */
  HW_TS_Start(TimerBLEAdv_Id, DEFAULT_BEACON_SEC * 10);

}

void BLE_SetPachaBleID(void)
{
	char l_achBleMacAddress[] = "00000000";
	char l_pchDefaultName[DEVICE_NAME_SIZE_MAX] = {};
	//uint8_t l_u8Size = 0u;
	BLE_GetMacAddressForLocalName(l_achBleMacAddress);
	g_u8SizeLocalName = snprintf( (char*)l_pchDefaultName, DEVICE_NAME_SIZE_MAX, "%s_%s",
			DEFAULT_BLE_DEVICE_NAME_PREFIX, l_achBleMacAddress);
	g_u8SizeLocalName = (g_u8SizeLocalName > DEVICE_NAME_SIZE_MAX)? DEVICE_NAME_SIZE_MAX : g_u8SizeLocalName;
	memcpy(g_sLocalName, l_pchDefaultName, g_u8SizeLocalName);
}

void BLE_GetMacAddress(char * p_pchOutBuffer)
{

	uint8_t* l_sDeviceAddr = NULL;
	tBleStatus ret;
   if(p_pchOutBuffer != NULL)
   {
	  ret = hci_read_bd_addr(l_sDeviceAddr);
	  if (ret != BLE_STATUS_SUCCESS)
	  {
		printf("Problem BLE");
	  }
      sprintf(p_pchOutBuffer, "%02X:%02X:%02X:%02X:%02X:%02X",
         l_sDeviceAddr[5],
         l_sDeviceAddr[4],
         l_sDeviceAddr[3],
         l_sDeviceAddr[2],
         l_sDeviceAddr[1],
         l_sDeviceAddr[0]);
   }
}

void BLE_GetMacAddressForLocalName(char * p_pchOutBuffer)
{
	uint8_t l_sDeviceAddr [4] = {};
	uint8_t l_u8Size = 0u;
	tBleStatus ret;
   if(p_pchOutBuffer != NULL)
   {
	  ret = hci_read_bd_addr(l_sDeviceAddr);
	  if (ret != BLE_STATUS_SUCCESS)
	  {
	    printf("Problem BLE");
	  }
	  l_u8Size=sprintf(p_pchOutBuffer, "%02X%02X%02X%02X",
      	 l_sDeviceAddr[3],
         l_sDeviceAddr[2],
         l_sDeviceAddr[1],
         l_sDeviceAddr[0]);
   }
}
