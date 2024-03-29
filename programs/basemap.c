#include <stdbool.h>
#include <GL/gl.h>

#include "../inlinebin.h"
#include "../programs.h"
#include "basemap.h"

enum	{ MAT_MODEL_INV
	, MAT_VIEW_INV
	, VP_ANGLE
	, VP_HEIGHT
	, VP_WIDTH
	, VERTEX
	} ;

static struct input inputs[] =
	{ [MAT_MODEL_INV]    = { .name = "mat_model_inv", .type = TYPE_UNIFORM }
	, [MAT_VIEW_INV]     = { .name = "mat_view_inv",  .type = TYPE_UNIFORM }
	, [VP_ANGLE]         = { .name = "vp_angle",      .type = TYPE_UNIFORM }
	, [VP_HEIGHT]        = { .name = "vp_height",     .type = TYPE_UNIFORM }
	, [VP_WIDTH]         = { .name = "vp_width",      .type = TYPE_UNIFORM }
	, [VERTEX]           = { .name = "vertex",        .type = TYPE_ATTRIBUTE }
	,                      { .name = NULL }
	} ;

struct program program_basemap __attribute__((section(".programs"))) =
	{ .name     = "basemap"
	, .vertex   = { .src = SHADER_BASEMAP_VERTEX }
	, .fragment = { .src = SHADER_BASEMAP_FRAGMENT }
	, .inputs   = inputs
	} ;

int32_t
program_basemap_loc_vertex (void)
{
	return inputs[VERTEX].loc;
}

void
program_basemap_use (const struct program_basemap *values)
{
	glUseProgram(program_basemap.id);
	glUniformMatrix4fv(inputs[MAT_MODEL_INV].loc, 1, GL_FALSE, values->mat_model_inv);
	glUniformMatrix4fv(inputs[MAT_VIEW_INV].loc, 1, GL_FALSE, values->mat_view_inv);
	glUniform1f(inputs[VP_ANGLE].loc,  values->vp_angle);
	glUniform1f(inputs[VP_HEIGHT].loc, values->vp_height);
	glUniform1f(inputs[VP_WIDTH].loc,  values->vp_width);
}
