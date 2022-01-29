/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "camera/format.h"

uint8_t format_num_planes(uint32_t format)
{
	switch (format) {
	case FORMAT_YUYV:
		/* Fallthrough */
	case FORMAT_RGB565:
		return 1;
	case FORMAT_YUV422:
		return 3;
	default:
		return 0;
	}
}

uint8_t format_bytes_per_pixel(uint32_t format, uint8_t plane)
{
	switch (format) {
	case FORMAT_YUYV:
		/* Fallthrough */
	case FORMAT_RGB565:
		return 2;
	case FORMAT_YUV422:
		return 1;
	default:
		return 0;
	}
}

uint8_t format_hsub(uint32_t format, uint8_t plane)
{
	switch (format) {
	case FORMAT_YUYV:
		/* Fallthrough */
	case FORMAT_RGB565:
		return 1;
	case FORMAT_YUV422:
		return plane ? 2 : 1;
	default:
		return 0;
	}
}

uint32_t format_stride(uint32_t format, uint8_t plane, uint16_t width)
{
	return format_bytes_per_pixel(format, plane) * width / format_hsub(format, plane);
}

uint32_t format_plane_size(uint32_t format, uint8_t plane, uint16_t width, uint16_t height)
{
	return format_stride(format, plane, width) * height;
}
