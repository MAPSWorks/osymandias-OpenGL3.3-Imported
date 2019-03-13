#pragma once

struct program_spherical {
	const float *mat_viewproj;
	const float *mat_model;
	float world_size;
};

enum LocSpherical {
	LOC_SPHERICAL_MAT_VIEWPROJ,
	LOC_SPHERICAL_MAT_MODEL,
	LOC_SPHERICAL_WORLD_SIZE,
	LOC_SPHERICAL_VERTEX,
	LOC_SPHERICAL_TEXTURE,
};

extern void  program_spherical_use (struct program_spherical *);
extern GLint program_spherical_loc (const enum LocSpherical);