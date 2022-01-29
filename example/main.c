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

#define CAMERA_PIO      pio0
#define CAMERA_BASE_PIN 16
#define CAMERA_XCLK_PIN 21
#define CAMERA_SDA      0
#define CAMERA_SCL      1

static inline int __i2c_write_blocking(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len)
{
	return i2c_write_blocking((i2c_inst_t *)i2c_handle, addr, src, len, false);
}

static inline int __i2c_read_blocking(void *i2c_handle, uint8_t addr, uint8_t *dst, size_t len)
{
	return i2c_read_blocking((i2c_inst_t *)i2c_handle, addr, dst, len, false);
}

// From http://www.paulbourke.net/dataformats/asciiart/
const char charmap[] = " .:-=+*#%@";

int main() {
	stdio_init_all();

	// Wait some time for USB serial connection
	sleep_ms(3000);

	const uint LED_PIN = PICO_DEFAULT_LED_PIN;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	i2c_init(i2c0, 100000);
	gpio_set_function(CAMERA_SDA, GPIO_FUNC_I2C);
	gpio_set_function(CAMERA_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(CAMERA_SDA);
	gpio_pull_up(CAMERA_SCL);

	struct camera camera;
	struct camera_platform_config platform = {
		.i2c_write_blocking = __i2c_write_blocking,
		.i2c_read_blocking = __i2c_read_blocking,
		.i2c_handle = i2c0,

		.pio = CAMERA_PIO,
		.xclk_pin = CAMERA_XCLK_PIN,
		.xclk_divider = 9,
		.base_pin = CAMERA_BASE_PIN,
		.base_dma_channel = -1,
	};

	int ret = camera_init(&camera, &platform);
	if (ret) {
		printf("camera_init failed: %d\n", ret);
		return 1;
	}

	const uint16_t width = CAMERA_WIDTH_DIV8;
	const uint16_t height = CAMERA_HEIGHT_DIV8;

	struct camera_buffer *buf = camera_buffer_alloc(FORMAT_YUV422, width, height);
	assert(buf);

	while (1) {
		printf("Capturing...\n");
		gpio_put(LED_PIN, 1);
		ret = camera_capture_blocking(&camera, buf, true);
		gpio_put(LED_PIN, 0);
		if (ret != 0) {
			printf("Capture error: %d\n", ret);
		} else {
			printf("Capture success\n");
			int y, x;
			for (y = 0; y < height; y++) {
				char row[width];
				for (x = 0; x < height; x++) {
					uint8_t v = buf->data[0][buf->strides[0] * y + x];
					v /= 26;
					row[x] = charmap[v];
				}
				printf("%s\n", row);
			}
		}

		sleep_ms(5000);
	}
}
