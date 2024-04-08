/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    User/BLE/ble_advertising.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BLE_ADVERTISING_H
#define BLE_ADVERTISING_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

typedef struct
{
  uint16_t u16AdvertisingInterval;/*!< Specifies the desired advertising interval. */
  uint8_t u8CalibratedTxPower;   /*!< Specifies the power at 0m. */
  uint16_t u16UnlockingCode;     /*!< Specifies the Code used to unlock station. */
  uint16_t u16KeyCode;  		/*!< Specifies the key used to decode the Unlocking Code */
  uint32_t u32Uptime;             /*!< Specifies the time sinbe the beacon was powered-up or rebooted. */
  struct {
      uint8_t u4State     : 4;
  } sBleStatusByte;
} BLE_Adv_InitTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported Macros -----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void BLE_Adv_Process(void);

void BLE_SetPachaBleID(void);

void BLE_GetMacAddress(char * p_pchOutBuffer);

void BLE_GetMacAddressForLocalName(char * p_pchOutBuffer);

#ifdef __cplusplus
}
#endif

#endif /* BLE_ADVERTISING_H */
