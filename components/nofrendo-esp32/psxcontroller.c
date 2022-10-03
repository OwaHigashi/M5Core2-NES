// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "psxcontroller.h"
#include "sdkconfig.h"

#define bit_joypad1_select 0
#define bit_joypad1_start  3
#define bit_joypad1_up     4
#define bit_joypad1_right  5
#define bit_joypad1_down   6
#define bit_joypad1_left   7
#define bit_soft_reset     12
#define bit_joypad1_a      13
#define bit_joypad1_b      14
#define bit_hard_reset     15

extern void ft6x06_init(uint16_t dev_addr);
extern bool ft6x36_direct_read(int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2, uint8_t* count);
extern esp_err_t m5core2_speaker(bool on);

int psxReadInput() {

	static bool speaker_on = false;
	uint16_t retval = 0;
	uint8_t key_value = 0xff;
	int16_t	x[2] = {0 ,0};
	int16_t	y[2] = {0, 0};
	uint8_t	count = 0;

	ft6x36_direct_read(&x[0], &y[0], &x[1], &y[1], &count);

	if(count > 2) return 0;

	for(int i = 0; i < count; i++) {
		if (x[i] < 100 && y[i] < 90)	{
			retval |=  1 << bit_joypad1_up;
		}
		else if (x[i] >= 100 && x[i] < 200 && y[i] < 90) {
			retval |=  1 << bit_joypad1_down;
		}
		else if (x[i] >= 200 && y[i] < 90) {
			retval |=  1 << bit_joypad1_a;
		}
		else if (x[i] < 100 && y[i] >= 90 && y[i] < 180)	{
			retval |= 1 << bit_joypad1_left;
		}	
		else if (x[i] >= 100 && x[i] < 200 && y[i] >= 90 && y[i] < 180)	{
			retval |= 1 << bit_joypad1_right;
		}	
		else if (x[i] >= 200 && y[i] >= 90 && y[i] < 180) {
			retval |=  1 << bit_joypad1_b;
		}	
		else if (x[i] < 100 && y[i] >= 180) {
			retval |=  1 << bit_joypad1_select;
		}	
		else if (x[i] >= 100 && x[i] < 200 && y[i] >= 180) {
			retval |=  1 << bit_joypad1_start;
		}	
		else if (x[i] >= 200 && y[i] >= 180) {
			speaker_on = !speaker_on;
			m5core2_speaker(speaker_on);
		}	
	}

	return (int)retval;
}

void psxcontrollerInit() {

    printf("FT6X06 detected...\n");   
	
	ft6x06_init(0x38);
}
