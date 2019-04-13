#include <stdbool.h>

#include <GL/gl.h>

#include "../camera.h"
#include "../glutil.h"
#include "../camera.h"
#include "../layers.h"
#include "../programs.h"
#include "../programs/basemap.h"
#include "../worlds.h"

static uint32_t vao, vbo;
static const struct glutil_vertex vertex[4] = {
	[0] = { -1.0, -1.0 },
	[1] = {  1.0, -1.0 },
	[2] = {  1.0,  1.0 },
	[3] = { -1.0,  1.0 },
};

static void
paint (const struct camera *cam)
{
	program_basemap_use(&((struct program_basemap) {
		.mat_model_inv    = world_get_matrix_inverse(),
		.mat_viewproj_inv = cam->matrix.inverse.viewproj,
		.mat_view_inv     = cam->matrix.inverse.view,
	}));

	glBindVertexArray(vao);
	glutil_draw_quad();
	program_none();
}

static void
destroy (void)
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

static bool
init (void)
{
	// Generate vertex buffer and array objects:
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	// Bind buffer and vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	// Link 'vertex' attribute:
	glutil_vertex_link(program_basemap_loc_vertex());

	// Copy vertices to buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

	return true;
}

// Export public methods:
LAYER(30) = {
	.init    = &init,
	.paint   = &paint,
	.destroy = &destroy,
};
