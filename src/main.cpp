/**
*******************************************************************************
* @file   main.cpp
* @brief  Main program body
*******************************************************************************
* Main program for collecting data and testing NanoEdge AI solution
*
* Compiler Flags
* -DDATA_LOGGING : data logging mode for collecting data
* -DNEAI_EMU     : test mode with NanoEdge AI Emulator 
* -DNEAI_LIB     : test mode with NanoEdge AI Library
*
* @note   if no compiler flag then data logging mode by default
*******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "LIS3DH.h"
#include "DFPlayerMini.h"
#include "WS2812.h"
#include "PixelArray.h"

#if !defined(DATA_LOGGING) && !defined(NEAI_LIB)
#define DATA_LOGGING
#endif


#ifndef DATA_LOGGING
#include "NanoEdgeAI.h"
#endif

/* Defines -------------------------------------------------------------------*/

#ifdef DATA_LOGGING
#define DATA_INPUT_USER 256
#define AXIS_NUMBER 3
#define LOG_NUMBER 100
#else
#define LEARNING_NUMBER 50 /* Number of learning signals */
#endif

#define NUM_COLORS 		5
#define WS2812_BUF 		18
#define STATE_NOMINAL	1
#define STATE_ANOMALY_1	2
#define STATE_ANOMALY_2	3
#define THRESHOLD		50

/* Objects -------------------------------------------------------------------*/
Serial pc (USBTX, USBRX);
I2C lis3dh_i2c (D12, A6); // (I2C_SDA, I2C_SCL)
LIS3DH lis3dh (lis3dh_i2c, LIS3DH_G_CHIP_ADDR, LIS3DH_DR_LP_1R6KHZ, LIS3DH_FS_2G);
DFPlayerMini mp3 (D5, D4);
PixelArray px(WS2812_BUF);
WS2812 ws(D9, WS2812_BUF, 0, 5, 5, 0);

/********************** Prototypes **********************/

void init (void);
#ifdef DATA_LOGGING
void data_logging_mode(void);
#endif
#ifdef NEAI_LIB
void neai_library_test_mode(void);
#endif
void lis_fill_array (void);
void led_set_buffer (void);
void led_set_intensity (uint8_t led_intensity);
void led_display_italian_flag (void);
void led_learning (void);
void led_black (void);


/* Variables -----------------------------------------------------------------*/
/* Define colors to be used, here we are using white, red, orange, green, blue & black */
int colorbuf[NUM_COLORS] = {0x002f00,0xffffff,0x2f0000,0x00002f,0x000000};
float lis_acc_x, lis_acc_y, lis_acc_z, last_lis_acc_x, last_lis_acc_y ,last_lis_acc_z = 0;
float lis_buffer[DATA_INPUT_USER * AXIS_NUMBER] = {0}, lis3dh_xyz[AXIS_NUMBER] = {0};
#ifdef NEAI_LIB
uint8_t similarity = 0;
uint16_t learn_cpt = 0, detect_cpt = 0;;
#endif

/* BEGIN CODE-----------------------------------------------------------------*/
/**
  * @brief  The application entry point
  *
  * @retval int
  */
int main()
{
	/* Initialization */
	init();
	
#ifdef DATA_LOGGING
	/* Data logging mode */
	/* Compiler flag: -DDATA_LOGGING */
	data_logging_mode();
#endif

#ifdef NEAI_LIB
	/* NanoEdge AI Library test mode */
	/* Compiler flag -DNEAI_LIB */
	neai_library_test_mode();
#endif
}

/* Functions definition ------------------------------------------------------*/

void init ()
{
#ifdef NEAI_LIB
	NanoEdgeAI_initialize();
#endif
	/* Leds */
	ws.useII(WS2812::PER_PIXEL);
	led_set_buffer();
	/* Low intensity, better for the video */
	led_set_intensity(150);
	/* MP3 volume, max is 30 */
	mp3.mp3_set_volume(28);

	if (lis3dh.read_id() != 0x33) {
		pc.printf("ERROR: Accelerometer not found");
	}
	
	
}

#ifdef DATA_LOGGING
/**
 * @brief  Data logging process
 *
 * @param  None
 * @retval None
 */
void data_logging_mode()
{
	while(1) {
			/* Wait one seconds before launching logging process */
			thread_sleep_for(1000);

			/* Logging process */
			for (uint8_t ilog = 0; ilog < LOG_NUMBER; ilog++) {
				lis_fill_array();
			}
	}
}
#endif

#ifdef NEAI_LIB
/**
 * @brief  Testing process with NanoEdge AI library
 *
 * @param  None
 * @retval None
 */
void neai_library_test_mode()
{
	/* Learning process */
	/* Wait one seconds before starting learning process */
	thread_sleep_for(1000);

	/* Learning process for one speed */
	for (uint16_t i = 0; i < LEARNING_NUMBER; i++) {
		/* Display blue progress bar */
		ws.write_offsets(px.getBuf(), 2 * WS2812_BUF / 3 - (i % 7), 2 * WS2812_BUF / 3 - (i % 7), 2 * WS2812_BUF / 3 - (i % 7));

		lis_fill_array();
		NanoEdgeAI_learn(lis_buffer);
		pc.printf("%d\n", (int)(learn_cpt * 100) / LEARNING_NUMBER);
				
	}

	for (uint8_t i = 0; i < 3; i++) {
		led_learning();
		thread_sleep_for(100);
		led_black();
		thread_sleep_for(100);
	}

	while(1) {
		lis_fill_array();
		similarity = NanoEdgeAI_detect(lis_buffer);
		pc.printf("%d\n", similarity);
		/* Counter to be sure that it's coffee time */
		if (similarity < THRESHOLD) {
			detect_cpt++;
		}
		else {
			/* If detect is higher than the defined threshold
			   we reset our cpt */
			detect_cpt = 0;
		}
		if (detect_cpt >= 3) {
			/* Display leds & play music */
			led_display_italian_flag();
			mp3.mp3_play(1);
			/* Wait for music time */
			thread_sleep_for(77000);
			/* Shut off leds & turn music off too */
			led_black();
			mp3.mp3_stop();
		}
	}
}
#endif


void lis_fill_array () // We get 256 samples of acc 3 axes
{
	for (uint16_t i = 0; i < DATA_INPUT_USER; i++) {
		if (lis3dh.data_ready()) { // A new data is available
			lis3dh.read_data(&lis3dh_xyz[0]);
			lis_buffer[AXIS_NUMBER * i] = lis3dh_xyz[0];
			lis_buffer[(AXIS_NUMBER * i) + 1] = lis3dh_xyz[1];
			lis_buffer[(AXIS_NUMBER * i) + 2] = lis3dh_xyz[2];
		} else {
			i--; // New data not ready
		}
	}
#ifndef NEAI_LIB
	/* Print accelerometer buffer for data logging and neai emulator test modes */
	for (uint16_t isample = 0; isample < AXIS_NUMBER * DATA_INPUT_USER - 1; isample++) {
		pc.printf("%.4f ", lis_buffer[isample]);
	}
	pc.printf("%.4f\n", lis_buffer[AXIS_NUMBER * DATA_INPUT_USER - 1]);
#endif	
}


void led_set_buffer ()
{
	/* Green, white & red for italian flag */
	for (int i = 0; i < WS2812_BUF / 3; i++) {
		px.Set(i, colorbuf[(i / 2) % NUM_COLORS]);
	}
	/* Blue for learning */
	for (int i = WS2812_BUF / 3; i < 2 * WS2812_BUF / 3; i++) {
		px.Set(i, colorbuf[3]);
	}
	/* Black to stop using leds */
	for (int i = 2 * WS2812_BUF / 3; i < WS2812_BUF; i++) {
		px.Set(i, colorbuf[4]);
	}
}

void led_set_intensity (uint8_t led_intensity)
{
	/* Define the same intensity for the whole leds */
	for (int j = 0; j < WS2812_BUF; j++) {
		px.SetI(j, led_intensity);
	}
	/* Decrease only white leds intensity */
	px.SetI(2, 30);
	px.SetI(3, 30);
}

void led_display_italian_flag ()
{
	/* Display italian flag */
	ws.write(px.getBuf());
}

void led_learning ()
{
	/* Display the 6 leds in blue */
	ws.write_offsets(px.getBuf(), WS2812_BUF / 3, WS2812_BUF / 3, WS2812_BUF / 3);
}

void led_black ()
{
	/* Display the 6 leds in black = leds off */
	ws.write_offsets(px.getBuf(), 2 * WS2812_BUF / 3, 2 * WS2812_BUF / 3, 2 * WS2812_BUF / 3);
}
