#pragma once

#include <stdint.h>

struct program_basemap {
	const float *mat_model_inv;
	const float *mat_view_inv;
	float vp_angle;
	float vp_height;
	float vp_width;
};

extern int32_t program_basemap_loc_vertex (void);
extern void    program_basemap_use (const struct program_basemap *);
