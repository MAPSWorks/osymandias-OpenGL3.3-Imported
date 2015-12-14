#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "../quadtree.h"
#include "../bitmap_mgr.h"
#include "../worlds.h"
#include "../viewport.h"
#include "../vector.h"
#include "../tilepicker.h"
#include "../tiledrawer.h"
#include "../layers.h"
#include "../inlinebin.h"
#include "../programs.h"

static int overlay_zoom = 0;
static int colorize_cache = 0;
static struct quadtree *textures = NULL;

static bool
have_anisotropic (void)
{
	static bool init = false;
	static bool have = false;

	if (!init) {
		const char *ext = (const char *)glGetString(GL_EXTENSIONS);
		have = strstr(ext, "GL_EXT_texture_filter_anisotropic");
		init = true;
	}

	return have;
}

static GLuint
texture_from_rawbits (void *rawbits)
{
	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, rawbits);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Apply anisotropic filtering if available:
	if (have_anisotropic()) {
		GLfloat max;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max);
	}

	return id;
}

static void
texture_destroy (void *data)
{
	// HACK: dirty cast from pointer to int:
	GLuint id = (GLuint)(ptrdiff_t)data;

	glDeleteTextures(1, &id);
}

static void
set_zoom_color (int zoom)
{
	float zoomcolors[6][3] = {
		{ 1.0, 0.2, 0.2 },
		{ 0.2, 1.0, 0.2 },
		{ 0.2, 0.2, 1.0 },
		{ 0.7, 0.7, 0.2 },
		{ 0.2, 0.7, 0.7 },
		{ 0.7, 0.2, 0.7 }
	};
	glColor3f(zoomcolors[zoom % 6][0], zoomcolors[zoom % 6][1], zoomcolors[zoom % 6][2]);
}

static bool
init (void)
{
	if (!bitmap_mgr_init())
		return false;

	// OpenGL texture cache:
	textures = quadtree_create(50, NULL, &texture_destroy);
	return true;
}

static void
destroy (void)
{
	quadtree_destroy(&textures);
	bitmap_mgr_destroy();
}

static bool
occludes (void)
{
	return false;
}

static void
paint (void)
{
	struct tilepicker tile;
	struct quadtree_req req;
	struct quadtree_req req_tex;
	int world_zoom = world_get_zoom();
	const struct coords *center = world_get_center();

	// Draw to world coordinates:
	viewport_gl_setup_world();

	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDisable(GL_BLEND);

	// The texture colors are multiplied with this value:
	glColor3f(1.0, 1.0, 1.0);

	for (bool iter = tilepicker_first(&tile); iter; iter = tilepicker_next(&tile)) {

		// If showing the zoom colors overlay, pick proper mixin color:
		if (overlay_zoom)
			set_zoom_color(tile.zoom);

		// The tilepicker can tell us to draw a tile at a different zoom level to the world zoom;
		// we need to correct the geometry to reflect that:
		req.x = ldexpf(tile.pos.x, -(world_zoom - tile.zoom));
		req.y = ldexpf(tile.pos.y, -(world_zoom - tile.zoom));
		req.zoom = tile.zoom;
		req.world_zoom = world_zoom;
		req.center = center;

		req_tex.found_data = NULL;

		// Is the texture already cached in OpenGL?
		if (quadtree_request(textures, &req)) {
			// If the texture is already cached at native resolution,
			// then we're done; else still try to get the native bitmap:
			if (req.found_zoom == tile.zoom) {
				if (colorize_cache) glColor3f(0.3, 1.0, 0.3);

				tiledrawer(&((struct tiledrawer) {
					.pick = &tile,
					.zoom = {
						.world = world_zoom,
						.found = req.found_zoom,
					},
					.texture_id = (GLuint)(ptrdiff_t)req.found_data,
				}));

				if (colorize_cache) glColor3f(1.0, 1.0, 1.0);
				continue;
			}
			// Save found texture:
			memcpy(&req_tex, &req, sizeof(req));
		}
		// Try to load the bitmap and turn it into an OpenGL texture:
		if (bitmap_request(&req)) {
			// If we found a texture, and it's better or equal than
			// the bitmap we came back with, use that instead:
			if (req_tex.found_data != NULL && req_tex.found_zoom >= req.found_zoom) {
				if (colorize_cache) glColor3f(0.3, 1.0, 0.3);

				tiledrawer(&((struct tiledrawer) {
					.pick = &tile,
					.zoom = {
						.world = world_zoom,
						.found = req_tex.found_zoom,
					},
					.texture_id = (GLuint)(ptrdiff_t)req_tex.found_data,
				}));

				if (colorize_cache) glColor3f(1.0, 1.0, 1.0);
				continue;
			}
			GLuint id = texture_from_rawbits(req.found_data);
			if (colorize_cache) glColor3f(0.8, 0.0, 0.0);

			tiledrawer(&((struct tiledrawer) {
				.pick = &tile,
				.zoom = {
					.world = world_zoom,
					.found = req.found_zoom,
				},
				.texture_id = id,
			}));

			if (colorize_cache) glColor3f(1.0, 1.0, 1.0);

			req.zoom = req.found_zoom;
			req.x = req.found_x;
			req.y = req.found_y;

			if (!quadtree_data_insert(textures, &req, (void *)(ptrdiff_t)id))
				texture_destroy((void *)(ptrdiff_t)id);

			continue;
		}
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	program_none();
}

static void
zoom (const unsigned int zoom)
{
	bitmap_zoom_change(zoom);
}

const struct layer *
layer_osm (void)
{
	static struct layer layer = {
		.init     = &init,
		.occludes = &occludes,
		.paint    = &paint,
		.zoom     = &zoom,
		.destroy  = &destroy,
	};

	return &layer;
}
