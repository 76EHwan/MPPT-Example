/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
#define TFT96
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sdmmc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "camera.h"
#include "st7789_lcd.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define LCD_ShowString	LCD7789_ShowString
#define LCD_Test		LCD7789_Test
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define CAM_FRAME_WIDTH   320
#define CAM_FRAME_HEIGHT  240
#define CAM_PIXEL_BYTES   2

uint8_t camera_buf[CAM_FRAME_WIDTH * CAM_FRAME_HEIGHT * CAM_PIXEL_BYTES] __attribute__((section(".RAM_D2")));

// FATFS
//FATFS fs __attribute__((section(".RAM_D2"), aligned(32)));
FIL fil __attribute__((section(".RAM_D2"), aligned(32)));
UINT bw;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
#ifdef TFT96
#define FrameWidth   160    // 카메라 실제 출력 (QQVGA)
#define FrameHeight  120
#define LCD_W        240    // LCD landscape 폭
#define LCD_H        135    // LCD landscape 높이
#elif TFT18
#define FrameWidth   128
#define FrameHeight  160
#define LCD_W        160
#define LCD_H        128
#endif
// picture buffer
uint16_t pic[FrameHeight][FrameWidth] __attribute__((aligned(32)));
uint16_t lcd_buf[LCD_H][LCD_W] __attribute__((aligned(32)));

uint32_t DCMI_FrameIsReady;
uint32_t Camera_FPS = 0;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void MPU_Config(void) {
	MPU_Region_InitTypeDef MPU_InitStruct = { 0 };

	/* Disables the MPU */
	HAL_MPU_Disable();

	/* Configure the MPU attributes for the QSPI 256MB without instruction access */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.BaseAddress = QSPI_BASE;
	MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
	MPU_InitStruct.SubRegionDisable = 0x00;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Configure the MPU attributes for the QSPI 8MB (QSPI Flash Size) to Cacheable WT */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER1;
	MPU_InitStruct.BaseAddress = QSPI_BASE;
	MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
	MPU_InitStruct.SubRegionDisable = 0x00;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Setup AXI SRAM in Cacheable WB */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = D1_AXISRAM_BASE;
	MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Enables the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void CPU_CACHE_Enable(void) {
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
//	SCB_EnableDCache();
}

void LED_Blink(uint32_t Hdelay, uint32_t Ldelay) {
	HAL_GPIO_WritePin(PE3_GPIO_Port, PE3_Pin, GPIO_PIN_SET);
	HAL_Delay(Hdelay - 1);
	HAL_GPIO_WritePin(PE3_GPIO_Port, PE3_Pin, GPIO_PIN_RESET);
	HAL_Delay(Ldelay - 1);
}

uint8_t SD_Mount(void) {
	FRESULT res = f_mount(&fs, "", 1);  // "" = 기본 드라이브
	if (res != FR_OK) {
		printf("SD Mount Failed: %d\r\n", res);
		return 0;
	}
	printf("SD Mount OK\r\n");
	return 1;
}

uint8_t Camera_Capture(void) {
	// CONTINUOUS 모드 일시 정지
	HAL_DCMI_Suspend(&hdcmi);

	// 현재 pic 버퍼를 camera_buf로 복사
	memcpy(camera_buf, pic, sizeof(pic));

	// CONTINUOUS 모드 재개
	HAL_DCMI_Resume(&hdcmi);

	printf("Capture OK\r\n");
	return 1;
}

uint8_t Save_BMP(const char *filename) {
	FRESULT res;
	UINT bw;

	// ── BMP 헤더 ──────────────────────────────
	uint32_t rowSize = FrameWidth * 3;          // 160*3 = 480 (4의 배수 OK)
	uint32_t pixelSize = rowSize * FrameHeight;   // 480*120 = 57,600
	uint32_t fileSize = 54 + pixelSize;

	uint8_t header[54] = { 0 };

	// File header (14 bytes)
	header[0] = 'B';
	header[1] = 'M';
	header[2] = (fileSize) & 0xFF;
	header[3] = (fileSize >> 8) & 0xFF;
	header[4] = (fileSize >> 16) & 0xFF;
	header[5] = (fileSize >> 24) & 0xFF;
	header[10] = 54;   // 픽셀 데이터 오프셋

	// DIB header (40 bytes)
	header[14] = 40;                          // 헤더 크기
	header[18] = (FrameWidth) & 0xFF;
	header[19] = (FrameWidth >> 8) & 0xFF;
	// height 음수 = top-down (정방향)
	int32_t negH = -FrameHeight;
	header[22] = (negH) & 0xFF;
	header[23] = (negH >> 8) & 0xFF;
	header[24] = (negH >> 16) & 0xFF;
	header[25] = (negH >> 24) & 0xFF;
	header[26] = 1;    // color planes
	header[28] = 24;   // bits per pixel (RGB888)
	// compression = 0 (none) → 이미 0

	// ── 파일 열기 ─────────────────────────────
	res = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		printf("BMP Open Failed: %d\r\n", res);
		return 0;
	}

	// ── 헤더 쓰기 ─────────────────────────────
	f_write(&fil, header, 54, &bw);

	// ── 픽셀 변환 & 쓰기 (행 단위) ────────────
	// pic[x][y]: x=가로(0~159), y=세로(0~119)
	static uint8_t rowBuf[FrameWidth * 3] __attribute__((section(".RAM_D2"), aligned(32)));
	uint16_t *flatPic = (uint16_t*) pic;

	for (int y = 0; y < FrameHeight; y++) {
		for (int x = 0; x < FrameWidth; x++) {
			uint16_t raw = flatPic[y * FrameWidth + x];
			uint16_t pixel = (raw >> 8) | (raw << 8);  // 바이트 스왑

			uint8_t r = ((pixel >> 11) & 0x1F) << 3;
			uint8_t g = ((pixel >> 5) & 0x3F) << 2;
			uint8_t b = (pixel & 0x1F) << 3;

			rowBuf[x * 3 + 0] = b;
			rowBuf[x * 3 + 1] = g;
			rowBuf[x * 3 + 2] = r;
		}
		f_write(&fil, rowBuf, rowSize, &bw);
	}

	f_close(&fil);
	printf("Saved: %s (%lu bytes)\r\n", filename, fileSize);
	return 1;
}

void SD_WriteTest(void) {
	FRESULT res;
	FIL testFile;
	UINT bw;
	static char testData[] = "SD Card Write Test OK!\r\n";
	char resultMsg[64];

	// SD 카드 상태 먼저 확인
	HAL_SD_CardStateTypeDef cardState = HAL_SD_GetCardState(&hsd1);
	sprintf(resultMsg, "CardState: %ld   ", cardState);
	LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, (uint8_t*) resultMsg);
	HAL_Delay(200);

	// hsd1 에러 상태 확인
	sprintf(resultMsg, "SD Err: %lu   ", hsd1.ErrorCode);
	LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, (uint8_t*) resultMsg);
	HAL_Delay(200);

	// 1. 마운트
	res = f_mount(&fs, "", 1);
	sprintf(resultMsg, "Mount: %d   ", res);
	LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, (uint8_t*) resultMsg);
	HAL_Delay(100);
	if (res != FR_OK)
		return;

	// 2. 파일 열기
	res = f_open(&testFile, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	sprintf(resultMsg, "Open: %d   ", res);
	LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, (uint8_t*) resultMsg);
	HAL_Delay(100);
	if (res != FR_OK)
		return;

	// 3. 쓰기
	res = f_write(&testFile, testData, strlen(testData), &bw);
	sprintf(resultMsg, "Write: %d (%dB)", res, bw);
	LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, (uint8_t*) resultMsg);
	HAL_Delay(100);

	// 4. 닫기
	f_close(&testFile);

	if (res == FR_OK && bw > 0) {
		LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
				(uint8_t*) "SD Test PASS!  ");
	} else {
		LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
				(uint8_t*) "SD Test FAIL!  ");
	}
	HAL_Delay(200);
//    f_unmount("");

}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

#ifdef W25Qxx
  SCB->VTOR = QSPI_BASE;
#endif
	MPU_Config();
	CPU_CACHE_Enable();

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_DCMI_Init();
	MX_I2C1_Init();
	MX_SPI4_Init();
	MX_TIM1_Init();
	MX_SDMMC1_SD_Init();
	MX_FATFS_Init();
	/* USER CODE BEGIN 2 */
	uint8_t text[64];

	LCD_Test();

	SD_WriteTest();  // ← 여기서 테스트
	HAL_Delay(3000);

	sprintf((char*) &text, "Camera Not Found");
	LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 16, text);

	//	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
	//	HAL_Delay(10);
#ifdef TFT96
	Camera_Init_Device(&hi2c1, FRAMESIZE_QQVGA);
#elif TFT18
	Camera_Init_Device(&hi2c1, FRAMESIZE_QQVGA2);
	#endif
	//clean Ypos 58
	ST7789_LCD_Driver.FillRect(&st7789_pObj, 0, 58, ST7789Ctx.Width, 16, BLACK);

	if (!SD_Mount()) {
		// Error_Handler() 대신 경고만 표시하고 계속 진행
		LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
				(uint8_t*) "SD Mount Fail");
		HAL_Delay(2000);
	}

	// 메인 루프
	uint32_t photo_count = 0;

	while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) {

		sprintf((char*) &text, "Camera id:0x%x   ", hcamera.device_id);
		LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, text);

		LED_Blink(5, 500);

		sprintf((char*) &text, "LongPress K1 to Run");
		LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12, text);

		LED_Blink(5, 500);
	}

	HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t) pic,
			sizeof(pic) / 4);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		if (DCMI_FrameIsReady) {
			DCMI_FrameIsReady = 0;

			char filename[32];
			sprintf(filename, "p%04lu.bmp", photo_count++);

			// 상태 표시
			LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
					(uint8_t*) "Capturing...");

			if (Camera_Capture()) {
				LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
						(uint8_t*) "Saving...");
				uint8_t ret = Save_BMP(filename);

				if (ret) {
					LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
							(uint8_t*) "Save OK!");
				} else {
					LCD_ShowString(0, 58, ST7789Ctx.Width, 16, 12,
							(uint8_t*) "Save FAIL!");
					FRESULT res = f_open(&fil, filename,
					FA_CREATE_ALWAYS | FA_WRITE);
					if (res != FR_OK) {
						char errMsg[32];
						sprintf(errMsg, "Open Fail:%d", res);
						LCD_ShowString(0, 46, ST7789Ctx.Width, 16, 12,
								(uint8_t*) errMsg);
						return 0;
					}
				}
			}

#ifdef TFT96
			for (int dy = 0; dy < LCD_H; dy++) {
				int sy = (dy * FrameHeight) / LCD_H;  // 0~119 매핑
				for (int dx = 0; dx < LCD_W; dx++) {
					lcd_buf[dy][dx] = pic[sy][(dx * FrameWidth) / LCD_W]; // 0~159 매핑
				}
			}
			ST7789_FillRGBRect(&st7789_pObj, 0, 0, (uint8_t*) lcd_buf, LCD_W,
					LCD_H);
#endif
//			LED_Blink(1, 1);
		}
		// HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Supply configuration update enable
	 */
	HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

	while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48
			| RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 5;
	RCC_OscInitStruct.PLL.PLLN = 96;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLQ = 10;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1
			| RCC_CLOCKTYPE_D1PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
	HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI48, RCC_MCODIV_4);
}

/* USER CODE BEGIN 4 */

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
	static uint32_t count = 0, tick = 0;

	if (HAL_GetTick() - tick >= 1000) {
		tick = HAL_GetTick();
		Camera_FPS = count;
		count = 0;
	}
	count++;

	DCMI_FrameIsReady = 1;
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1) {
		LED_Blink(5, 250);
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
