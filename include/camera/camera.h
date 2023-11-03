/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __CAMERA_H__
#define __CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "hardware/dma.h"
#include "hardware/pio.h"

#include "camera/ov7670.h"

#define CAMERA_WIDTH_DIV8  60
#define CAMERA_HEIGHT_DIV8 60

#define CAMERA_MAX_N_PLANES 3

// Stores a camera frame.
// Can be allocated dynamically using camera_buffer_alloc/camera_buffer_free,
// or you can use some static buffers and populate this structure manually.
// The format_* helpers from format.h can help determine the correct values.
struct camera_buffer {
	uint32_t format;
	uint16_t width;
	uint16_t height;
	uint32_t strides[CAMERA_MAX_N_PLANES];
	uint32_t sizes[CAMERA_MAX_N_PLANES];
	uint8_t *data[CAMERA_MAX_N_PLANES];
};

typedef void (*camera_frame_cb)(struct camera_buffer *buf, void *p);

// Stores format/width/height dependent configuration for PIO and DMA
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
	// Interface for the low level driver
	OV7670_host driver_host;

	// Static PIO and DMA configuration, set at camera_init time
	uint frame_offset;
	uint shift_byte_offset;
	int dma_channels[CAMERA_MAX_N_PLANES];

	// Dynamic configuration set by width/height/format
	struct camera_config config;

	// Frame context, tracking the currently in-progress frame
	struct camera_buffer *volatile pending;
	camera_frame_cb volatile pending_cb;
	void *volatile cb_data;
};

// Provides the platform abstraction for a camera instance.
// Needs to be populated and provided to camera_init.
struct camera_platform_config {
	// i2c access for configuring the sensor
	// Both functions should return the number of bytes transferred
	// (they should behave the same as the SDK i2c functions, with nostop = false)
	int (*i2c_write_blocking)(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len);
	int (*i2c_read_blocking)(void *i2c_handle, uint8_t addr, uint8_t *src, size_t len);
	void *i2c_handle;

	// PIO settings
	PIO pio;
	// xclk_pin must be one of the GPOUT clock pins: 21, 23, 24 or 25
	uint xclk_pin;
	uint xclk_divider;
	// See camera.pio for pin order
	uint base_pin_sm_0;
	uint base_pin_sm_s;
	// 3 DMA channels will be claimed starting at base_dma_channel.
	// Set to -1 for dynamic assignment
	int base_dma_channel;
};

// Initialise the camera.
// The 'camera' structure doesn't need any initialisation, it's passed in only
// to allow the caller to allocate it as they like.
// 'params' must be populated by the caller, and must remain valid and in-scope
// for the whole lifetime of the camera.
//
// Returns 0 on success
int camera_init(struct camera *camera, struct camera_platform_config *params);

// Not implemented, but theoretically would terminate the clock, clear the PIO
// memory etc.
void camera_term(struct camera *camera);

// Configure the camera for a particular format/width/height.
// This requires i2c interaction with the camera, which may be slow, so this
// function exists to let users do that operation "up front" rather than
// incurring an unknown delay at the point of triggering a capture.
//
// Returns 0 on success
int camera_configure(struct camera *camera, uint32_t format, uint16_t width, uint16_t height);

// Capture a frame into the provided buffer, blocking until it is complete.
//
// If the camera is already configured for the same format/width/height as 'into'
// then no reconfiguration happens.
// Otherwise, if allow_reconfigure is true then the camera will first be
// reconfigured, and if not, capture will fail.
//
// Returns 0 on success
int camera_capture_blocking(struct camera *camera, struct camera_buffer *into, bool allow_reconfigure);

// Capture a frame into the provided buffer, calling complete_cb when complete.
// Note: The callback is called from the PIO interrupt handler, so it must be
// safe for use in interrupt context!
//
// If the camera is already configured for the same format/width/height as 'into'
// then no reconfiguration happens.
// Otherwise, if allow_reconfigure is true then the camera will first be
// reconfigured, and if not, capture will fail.
//
// Returns 0 on success
int camera_capture_with_cb(struct camera *camera, struct camera_buffer *into, bool allow_reconfigure,
                           camera_frame_cb complete_cb, void *cb_data);

// Allocate a camera buffer with the provided format, width and height
// Uses malloc() to allocate the structure and the data buffers.
struct camera_buffer *camera_buffer_alloc(uint32_t format, uint16_t width, uint16_t height);

// Free a buffer previously allocated by camera_buffer_alloc.
void camera_buffer_free(struct camera_buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* __CAMERA_H__ */
