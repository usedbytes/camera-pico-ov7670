/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdint.h>

#include "hardware/pio.h"

#include "camera/ov7670.h"

#define CAMERA_WIDTH_DIV8  80
#define CAMERA_HEIGHT_DIV8 60

#define CAMERA_MAX_N_PLANES 3

struct camera_buffer {
	uint32_t format;
	uint16_t width;
	uint16_t height;
	uint32_t strides[3];
	uint32_t sizes[3];
	uint8_t *data[3];
};

typedef void (*camera_frame_cb)(struct camera_buffer *buf, void *p);

struct camera_config {
	uint32_t format;
	uint16_t width;
	uint16_t height;
	uint dma_transfers[CAMERA_MAX_N_PLANES];
	uint dma_offset[CAMERA_MAX_N_PLANES];
	dma_channel_config dma_cfgs[CAMERA_MAX_N_PLANES];
	pio_sm_config sm_cfgs[4];
};

struct camera {
	OV7670_host driver_host;

	uint frame_offset;
	uint shift_byte_offset;
	int dma_channels[CAMERA_MAX_N_PLANES];
	struct camera_config config;

	struct camera_buffer *volatile pending;
	camera_frame_cb volatile pending_cb;
	void *volatile cb_data;
};

struct camera_platform_config {
	// i2c access for configuring the sensor
	int (*i2c_write_blocking)(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len);
	int (*i2c_read_blocking)(void *i2c_handle, uint8_t addr, uint8_t *src, size_t len);
	void *i2c_handle;

	// PIO settings
	PIO pio;
	uint xclk_pin;
	uint xclk_divider;
	// See camera.pio for pin order
	uint base_pin;
	// -1 for dynamic assignment
	int base_dma_channel;
};

int camera_init(struct camera *camera, struct camera_platform_config *params);
void camera_term(struct camera *camera);

int camera_configure(struct camera *camera, uint32_t format, uint16_t width, uint16_t height);

int camera_capture_blocking(struct camera *camera, struct camera_buffer *into, bool allow_reconfigure);

int camera_capture_with_cb(struct camera *camera, struct camera_buffer *into, bool allow_reconfigure,
                           camera_frame_cb complete_cb, void *cb_data);

struct camera_buffer *camera_buffer_alloc(uint32_t format, uint16_t width, uint16_t height);
void camera_buffer_free(struct camera_buffer *buf);

#endif /* __CAMERA_H__ */
