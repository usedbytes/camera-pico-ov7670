/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "camera/camera.h"
#include "camera/format.h"
// include of tflite framework
#include "../model/tflite_model.h"
#include "../model/ml_model.h"

#define CAMERA_PIO           pio0
#define CAMERA_BASE_PIN_SM_0 10
#define CAMERA_BASE_PIN_SM_s 12
#define CAMERA_XCLK_PIN      21
#define CAMERA_SDA      2
#define CAMERA_SCL      3

#define THR_PREDICT 0.1
#define THR_RATIO_FACE_PER_FRAME 0.1
#define NUMBER_FRAME_CAL 10

MLModel ml_model(tflite_model, 150 * 1024);

static inline int __i2c_write_blocking(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len)
{
	return i2c_write_blocking((i2c_inst_t *)i2c_handle, addr, src, len, false);
}

static inline int __i2c_read_blocking(void *i2c_handle, uint8_t addr, uint8_t *dst, size_t len)
{
	return i2c_read_blocking((i2c_inst_t *)i2c_handle, addr, dst, len, false);
}


int8_t* data_input = nullptr;
int8_t* data_output = nullptr;
float scale;
int32_t zero_point;
float output_scale;
int32_t output_zero_point;
int size_input;
int size_output; 
uint8_t slick_row = 0;
uint8_t slick_col = 0;
int8_t visit[64] = {0};
int8_t slick[NUMBER_FRAME_CAL][64] = {0};
int8_t res[64] = {0};

int bfs(int8_t frame[64],int8_t mask[64], int sx, int sy);

int main() {
	int frame_id = 0;
	uint64_t start = 0;
	stdio_init_all();

	// Wait some time for USB serial connection
	sleep_ms(3000);

	if (!ml_model.init()) {
        printf("Failed to initialize ML model!\n");
        while (1) { tight_loop_contents(); }
    }
    else {
        printf("|    ML model initialize OK    |\n");
		size_input = ml_model.input_size();
		printf("Size input: %d, ", size_input);
		size_output = ml_model.output_size();
		printf("Size output: %d, ", size_output);

		scale = ml_model.input_scale(); 
		printf("Input Scale: %f, ",scale);
		zero_point = ml_model.input_zero_point();
		printf("Input Zero Point: %d\r\n", zero_point);

		output_scale = ml_model.output_scale(); 
		printf("Output Scale: %f, ",output_scale);
		output_zero_point = ml_model.output_zero_point();
		printf("Output Zero Point: %d\r\n", output_zero_point);
    }

	const uint LED_PIN = PICO_DEFAULT_LED_PIN;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	gpio_put(LED_PIN, 0);
	i2c_init(i2c1, 100000);
	gpio_set_function(CAMERA_SDA, GPIO_FUNC_I2C);
	gpio_set_function(CAMERA_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(CAMERA_SDA);
	gpio_pull_up(CAMERA_SCL);

	struct camera camera;
	struct camera_platform_config platform = {
		.i2c_write_blocking = __i2c_write_blocking,
		.i2c_read_blocking = __i2c_read_blocking,
		.i2c_handle = i2c1,

		.pio = CAMERA_PIO,
		.xclk_pin = CAMERA_XCLK_PIN,
		.xclk_divider = 9,
		.base_pin_sm_0 = CAMERA_BASE_PIN_SM_0,
		.base_pin_sm_s = CAMERA_BASE_PIN_SM_s,
		.base_dma_channel = -1,
	};
	
	int ret = camera_init(&camera, &platform);
	if (ret < 0) {
		printf("camera_init failed: %d\n", ret);
		while (1) {
			gpio_put(LED_PIN, 1);
			sleep_ms(200);
			gpio_put(LED_PIN, 0);
			sleep_ms(200);
		}
	}
	const uint16_t width = CAMERA_WIDTH_DIV8;
	const uint16_t height = CAMERA_HEIGHT_DIV8;

	struct camera_buffer *buf = camera_buffer_alloc(FORMAT_YUV422, width, height);
	assert(buf);
	data_input = (int8_t*)malloc(size_input);
	data_output = (int8_t*)malloc(size_output);

	memset(data_input, 0, size_input);
	while (1) {
		printf("[%03dx%03d] %04d$", width, height, frame_id);
		ret = camera_capture_blocking(&camera, buf, true);
		if (ret != 0) {
			gpio_put(LED_PIN, 1);
			sleep_ms(500);
			gpio_put(LED_PIN, 0);
		} else {
			int y, x, i, j;
			for (y = 0; y < height; y++) {
				for (x = 0; x < width; x++) {
					data_input[64 * y + x] = buf->data[0][buf->strides[0] * y + x] + zero_point;
					uint8_t v = buf->data[0][buf->strides[0] * y + x];
					char snum[4];
    				int n = sprintf(snum, "%d", v);
					printf(" %s", snum);
				}
			}
			printf("\n");
			printf("{%03dx%03d} %04d$", 8, 8, frame_id);
			start = time_us_64();
			if (ml_model.predict(data_input, data_output, THR_PREDICT) > 0) {
				for (int s = 0; s < 8; s++) {
					for (int e = 0; e < 8; e++) { 
						printf(" %d", data_output[e * 8 + s]);
						if(data_output[e * 8 + s] != 0 && visit[e * 8 + s] == 0)
						slick[slick_row][slick_col] = bfs(data_output, visit, e, s);
						slick_col += 1;
					}
				}
				memset(visit, 0, 64);
				slick_col = 0;
				slick_row += 1;
			}
			else
				printf("Predict Fail\n");

			memset(data_input, 0, size_input);
			printf("\n");
			if(frame_id == NUMBER_FRAME_CAL){
				printf("Small block has face: ");
				for(int r = 0; r < NUMBER_FRAME_CAL; r++){
					for(int c = 0; c < 64; c++){
						res[c] += slick[r][c];
					}
				}
				for(int c = 0; c < 64; c++){
					float ratio = (float)res[c]/NUMBER_FRAME_CAL;
					if(ratio > THR_RATIO_FACE_PER_FRAME){
						printf("%.2f, ", ratio);
						gpio_put(LED_PIN, 1);
						sleep_ms(200);
						gpio_put(LED_PIN, 0);
						sleep_ms(200);
					}
				}
				frame_id = 0;
				slick_row = 0;
				memset(res, 0, 64);
				memset(slick, 0 , NUMBER_FRAME_CAL*64);
			}
			frame_id++;
			// if (frame_id >= 1000)
			// 	frame_id = 0;
			
		}
	}
}

int bfs(int8_t frame[64], int8_t mask[64], int sx, int sy){
	int moveX[] = {0, 0, 1, -1};
	int moveY[] = {1, -1, 0, 0};
	int u, v, x, y, countLoop = 0;
	int n = 8;
	int m = 8;
	int numFrame = 1;
	int queue[100];
	queue[countLoop] = sx;
	queue[countLoop + 1] = sy;
	mask[n*sx+sy] = 1;
	while(countLoop >= 0){
		x = queue[countLoop];
		y = queue[countLoop+1];
		countLoop-=2;
		for(int i = 0; i < 4; i++){
			u = x + moveX[i];
			v = y + moveY[i];
			if((u > n) || (u < 1)){
				continue;
			}
			if((v > m) || (v < 1)){
				continue;
			}
			if((frame[n*u+v] != 0) && (mask[n*u+v] == 0)){
				numFrame += 1;
				mask[n*u+v] = 1;
				countLoop+=2;
				queue[countLoop] = u;
				queue[countLoop + 1] = v;
			}
		}
	}

	return numFrame;
}