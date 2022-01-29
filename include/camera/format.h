/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <stdint.h>

#define FORMAT_CODE(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define FORMAT_YUYV   FORMAT_CODE('Y', 'U', 'Y', 'V')
#define FORMAT_RGB565 FORMAT_CODE('R', 'G', '1', '6')
#define FORMAT_YUV422 FORMAT_CODE('Y', 'U', '1', '6')

uint8_t format_num_planes(uint32_t format);
uint8_t format_bytes_per_pixel(uint32_t format, uint8_t plane);
uint8_t format_hsub(uint32_t format, uint8_t plane);
uint32_t format_stride(uint32_t format, uint8_t plane, uint16_t width);
uint32_t format_plane_size(uint32_t format, uint8_t plane, uint16_t width, uint16_t height);

#endif /* __FORMAT_H__ */
