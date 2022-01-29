/**
 * Copyright (c) 2021-2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

#include "camera/camera.h"
#include "camera/format.h"
#include "camera/ov7670.h"

#include "camera.pio.h"

#define CAMERA_PIO_FRAME_SM  0

struct camera *volatile irq_ctxs[2];

static inline void __camera_isr(struct camera *camera)
{
	if (!camera || !camera->pending) {
		return;
	}

	if (camera->pending_cb) {
		camera->pending_cb(camera->pending, camera->cb_data);
	}

	camera->pending = NULL;
}

static void camera_isr_pio0(void)
{
	struct camera *camera = irq_ctxs[0];
	__camera_isr(camera);

	pio_interrupt_clear(pio0, 0);
}

static void camera_isr_pio1(void)
{
	struct camera *camera = irq_ctxs[1];
	__camera_isr(camera);

	pio_interrupt_clear(pio1, 0);
}

static void camera_pio_init(struct camera *camera)
{
	struct camera_platform_config *platform = camera->driver_host.platform;
	PIO pio = platform->pio;

	hard_assert(pio == pio0 || pio == pio1);

	camera->shift_byte_offset = pio_add_program(pio, &camera_pio_shift_byte_program);
	camera->frame_offset = pio_add_program(pio, &camera_pio_frame_program);
	for (int i = 0; i < 4; i++) {
		camera_pio_init_gpios(pio, i, platform->base_pin);
	}
	pio->inte0 = (1 << (8 + CAMERA_PIO_FRAME_SM));

	if (pio == pio0) {
		irq_ctxs[0] = camera;
		irq_set_exclusive_handler(PIO0_IRQ_0, camera_isr_pio0);
		irq_set_enabled(PIO0_IRQ_0, true);
	} else if (pio == pio1) {
		irq_ctxs[1] = camera;
		irq_set_exclusive_handler(PIO1_IRQ_0, camera_isr_pio1);
		irq_set_enabled(PIO1_IRQ_0, true);
	}
}

static bool camera_detect(struct camera_platform_config *platform)
{
	const uint8_t reg = OV7670_REG_PID;
	uint8_t val = 0;

	int tries = 5;
	while (tries--) {
		int ret = platform->i2c_write_blocking(platform->i2c_handle, OV7670_ADDR, &reg, 1);
		if (ret != 1) {
			sleep_ms(100);
			continue;
		}

		ret = platform->i2c_read_blocking(platform->i2c_handle, OV7670_ADDR, &val, 1);
		if (ret != 1) {
			sleep_ms(100);
			continue;
		}

		if (val == 0x76) {
			break;
		}

		sleep_ms(100);
	}

	return val == 0x76;
}

int camera_init(struct camera *camera, struct camera_platform_config *params)
{
	OV7670_status status;

	*camera = (struct camera){ 0 };

	clock_gpio_init(params->xclk_pin, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, params->xclk_divider);

	camera->driver_host = (OV7670_host){
		.pins = &(OV7670_pins){
			.enable = -1,
			.reset = -1,
		},
		.platform = params,
	};

	// Some settling time for the clock
	sleep_ms(300);

	// Try and check that the camera is present
	if (!camera_detect(params)) {
		return -1;
	}

	// Note: Frame rate is ignored
	status = OV7670_begin(&camera->driver_host, OV7670_COLOR_YUV, OV7670_SIZE_DIV8, 0.0);
	if (status != OV7670_STATUS_OK) {
		return -1;
	}

	if (params->base_dma_channel >= 0) {
		for (int i = 0; i < CAMERA_MAX_N_PLANES; i++) {
			dma_channel_claim(params->base_dma_channel + i);
			camera->dma_channels[i] = params->base_dma_channel + i;
		}
	} else {
		for (int i = 0; i < CAMERA_MAX_N_PLANES; i++) {
			camera->dma_channels[i] = dma_claim_unused_channel(true);
		}
	}

	camera_pio_init(camera);

	// TODO: Set an initial frame config here?
	// camera_configure(camera, FORMAT_RGB565, CAMERA_WIDTH_DIV8, CAMERA_HEIGHT_DIV8);

	return 0;
}

void camera_term(struct camera *camera)
{
	// TODO :-)
}

static OV7670_colorspace ov7670_colorspace_from_format(uint32_t format)
{
	switch (format) {
	case FORMAT_RGB565:
		return OV7670_COLOR_RGB;
	case FORMAT_YUYV:
		/* Fallthrough */
	case FORMAT_YUV422:
		return OV7670_COLOR_YUV;
	}

	return 0;
}

static enum dma_channel_transfer_size camera_transfer_size(uint32_t format, uint8_t plane)
{
	switch (format) {
	case FORMAT_YUYV:
		/* Fallthrough */
	case FORMAT_RGB565:
		return DMA_SIZE_32;
	case FORMAT_YUV422:
		return plane ? DMA_SIZE_8 : DMA_SIZE_16;
	default:
		return 0;
	}
}

static uint8_t __dma_transfer_size_to_bytes(enum dma_channel_transfer_size dma)
{
	switch (dma) {
	case DMA_SIZE_8:
		return 1;
	case DMA_SIZE_16:
		return 2;
	case DMA_SIZE_32:
		return 4;
	}

	return 0;
}

static const pio_program_t *camera_get_pixel_loop(uint32_t format)
{
	switch (format) {
	case FORMAT_YUYV:
		return &pixel_loop_yuyv_program;
	case FORMAT_RGB565:
		// XXX: Should really use a 16-bit loop and swap to 2 bytes
		// per chunk instead of 4.
		return &pixel_loop_yuyv_program;
	case FORMAT_YUV422:
		return &pixel_loop_yu16_program;
	default:
		return NULL;
	}
}

static void camera_pio_configure(struct camera *camera)
{
	struct camera_platform_config *platform = camera->driver_host.platform;

	// First disable everything
	pio_set_sm_mask_enabled(platform->pio, 0xf, false);

	// Then reset everything
	pio_restart_sm_mask(platform->pio, 0xf);

	// Drain the FIFOs
	for (int i = 0; i < 4; i++) {
		pio_sm_clear_fifos(platform->pio, i);
	}

	// Patch the pixel loop
	const pio_program_t *pixel_loop = camera_get_pixel_loop(camera->config.format);
	camera_pio_patch_pixel_loop(platform->pio, camera->frame_offset, pixel_loop);

	// Finally we can configure and enable the state machines
	uint8_t num_planes = format_num_planes(camera->config.format);
	for (int i = 0; i < num_planes; i++) {
		pio_sm_init(platform->pio, i + 1, camera->shift_byte_offset, &camera->config.sm_cfgs[i + 1]);
		pio_sm_set_enabled(platform->pio, i + 1, true);
	}

	pio_sm_init(platform->pio, CAMERA_PIO_FRAME_SM, camera->frame_offset, &camera->config.sm_cfgs[CAMERA_PIO_FRAME_SM]);
	pio_sm_set_enabled(platform->pio, CAMERA_PIO_FRAME_SM, true);
}

int camera_configure(struct camera *camera, uint32_t format, uint16_t width, uint16_t height)
{
	if (width != CAMERA_WIDTH_DIV8 || height != CAMERA_HEIGHT_DIV8) {
		// TODO: Handle other sizes
		return -1;
	}

	struct camera_platform_config *platform = camera->driver_host.platform;

	OV7670_set_format(platform, ov7670_colorspace_from_format(format));
	OV7670_set_size(platform, OV7670_SIZE_DIV8);

	camera->config.sm_cfgs[CAMERA_PIO_FRAME_SM] =
		camera_pio_get_frame_sm_config(platform->pio, CAMERA_PIO_FRAME_SM, camera->frame_offset, platform->base_pin);

	uint8_t num_planes = format_num_planes(format);
	for (int i = 0; i < num_planes; i++) {
		enum dma_channel_transfer_size xfer_size = camera_transfer_size(format, i);

		dma_channel_config c = dma_channel_get_default_config(camera->dma_channels[i]);
		channel_config_set_transfer_data_size(&c, xfer_size);
		channel_config_set_read_increment(&c, false);
		channel_config_set_write_increment(&c, true);
		channel_config_set_dreq(&c, pio_get_dreq(platform->pio, i + 1, false));

		camera->config.dma_cfgs[i] = c;

		uint8_t xfer_bytes = __dma_transfer_size_to_bytes(xfer_size);
		camera->config.dma_offset[i] = 4 - xfer_bytes,
		camera->config.dma_transfers[i] = format_plane_size(format, i, width, height) / xfer_bytes,

		camera->config.sm_cfgs[i + 1] = camera_pio_get_shift_byte_sm_config(platform->pio, i + 1,
							camera->shift_byte_offset, platform->base_pin,
							xfer_bytes * 8);
	}

	camera->config.format = format;
	camera->config.width = width;
	camera->config.height = height;

	camera_pio_configure(camera);

	return 0;
}

static uint8_t camera_pixels_per_chunk(uint32_t format)
{
	switch (format) {
	case FORMAT_YUYV:
		/* Fallthrough */
	case FORMAT_RGB565:
		return 2;
	case FORMAT_YUV422:
		return 2;
	default:
		return 1;
	}
}

static int camera_do_frame(struct camera *camera, struct camera_buffer *buf, camera_frame_cb complete_cb, void *cb_data,
		           bool allow_reconfigure, bool blocking)
{
	if (camera->pending) {
		return -2;
	}

	if ((camera->config.format != buf->format) ||
	    (camera->config.width != buf->width) ||
	    (camera->config.height != buf->height)) {
		if (allow_reconfigure) {
			camera_configure(camera, buf->format, buf->width, buf->height);
		} else {
			return -1;
		}
	}

	struct camera_platform_config *platform = camera->driver_host.platform;

	uint8_t num_planes = format_num_planes(camera->config.format);
	for (int i = 0; i < num_planes; i++) {
		dma_channel_configure(camera->dma_channels[i],
				&camera->config.dma_cfgs[i],
				buf->data[i],
				((char *)&platform->pio->rxf[i + 1]) + camera->config.dma_offset[i],
				camera->config.dma_transfers[i],
				true);
	}

	uint32_t num_loops = buf->width / camera_pixels_per_chunk(buf->format);

	camera->pending = buf;
	camera->pending_cb = complete_cb;
	camera->cb_data = cb_data;

	camera_pio_trigger_frame(platform->pio, num_loops, buf->height);

	if (blocking) {
		while (camera->pending) {
			sleep_ms(1);
		}
	}

	return 0;
}

int camera_capture_blocking(struct camera *camera, struct camera_buffer *into, bool allow_reconfigure)
{
	return camera_do_frame(camera, into, NULL, NULL, allow_reconfigure, true);
}

int camera_capture_with_cb(struct camera *camera, struct camera_buffer *into, bool allow_reconfigure,
                           camera_frame_cb complete_cb, void *cb_data)
{
	return camera_do_frame(camera, into, complete_cb, cb_data, allow_reconfigure, false);
}

struct camera_buffer *camera_buffer_alloc(uint32_t format, uint16_t width, uint16_t height)
{
	struct camera_buffer *buf = malloc(sizeof(*buf));
	if (!buf) {
		return NULL;
	}

	*buf = (struct camera_buffer){
		.format = format,
		.width = width,
		.height = height,
	};

	uint8_t num_planes = format_num_planes(format);
	for (int i = 0; i < num_planes; i++) {
		buf->strides[i] = format_stride(format, i, width);
		buf->sizes[i] = format_plane_size(format, i, width, height);
		buf->data[i] = malloc(buf->sizes[i]);
		if (!buf->data[i]) {
			goto error;
		}
	}

	return buf;

error:
	for (int i = 0; i < num_planes; i++) {
		if (buf->data[i]) {
			free(buf->data[i]);
		}
	}
	free(buf);
	return NULL;
}

void camera_buffer_free(struct camera_buffer *buf)
{
	if (buf == NULL) {
		return;
	}

	uint8_t num_planes = format_num_planes(buf->format);
	for (int i = 0; i < num_planes; i++) {
		if (buf->data[i]) {
			free(buf->data[i]);
		}
	}
	free(buf);
}

// Adafruit driver functions

void OV7670_print(char *str)
{
	// TODO
}

int OV7670_read_register(void *platform, uint8_t reg)
{
	struct camera_platform_config *pcfg = (struct camera_platform_config *)platform;
	uint8_t value;

	pcfg->i2c_write_blocking(pcfg->i2c_handle, OV7670_ADDR, &reg, 1);
	pcfg->i2c_read_blocking(pcfg->i2c_handle, OV7670_ADDR, &value, 1);

	return value;
}

void OV7670_write_register(void *platform, uint8_t reg, uint8_t value)
{
	struct camera_platform_config *pcfg = (struct camera_platform_config *)platform;

	pcfg->i2c_write_blocking(pcfg->i2c_handle, OV7670_ADDR, (uint8_t[]){ reg, value }, 2);
}
