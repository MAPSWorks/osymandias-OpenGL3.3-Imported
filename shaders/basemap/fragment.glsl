#version 130

flat          in vec3 cam;
noperspective in vec3 p;
smooth        in float frag_look_angle;
flat          in float frag_arc_angle;

out vec4 fragcolor;

const float pi = 3.141592653589793238462643383279502884;

const int zoom_min = 0;
const int zoom_max = 19;

// Transform coordinates on a unit sphere to tile coordinates at a given zoom.
bool sphere_to_tile (in vec3 s, in int zoom, out vec2 tile)
{
	// Start with the y coordinate, since it can be out of range in the
	// polar zones. The naive formula is:
	//
	//   2^zoom * (0.5 - atanh(y) / (2 * pi))
	//
	// which can be reduced by combining terms. Observing that:
	//
	//   2 * atanh(y) = log((1 + y) / (1 - y))
	//
	// the naive formula can be rewritten as:
	//
	//   2^(zoom - 1) * (1 + log((1 - y) / (1 + y)) / (2 * pi))
	//
	// Start with the invariant term:
	float y = log((1.0 - s.y) / (1.0 + s.y)) / 2.0 / pi;

	// Reject polar zones:
	if (abs(y) >= 1.0)
		return false;

	// Complete the multiplication:
	tile.y = exp2(zoom - 1) * (1.0 + y);

	// The naive formula for the x coordinate is:
	//
	//  2^zoom * (0.5 + atan(x / z) / (2 * pi))
	//
	// This expression can also be simplified by combining terms:
	tile.x = exp2(zoom - 1) * (1.0 + atan(s.x, s.z) / pi);

	return true;
}

vec4 shade (vec4 color, float det)
{
	// Un-gamma (approx):
	color = sqrt(color);

	// Angle-based shading:
	color.xyz *= vec3(0.75 + det / 4);

	// Restore gamma:
	return color * color;
}

int zoomlevel (float dist, float lat)
{
	// At high latitudes, tiles are smaller and closer together. Decrease
	// the zoom strength based on latitude to keep the number of tiles
	// down. The divisor is an arbitrary number that determines the
	// strength of the attenuation as latitude increases:
	float latdilute = 1.0 - abs(lat) / 12.0;

	// Calculate the parallel (facing the camera) "ground length" swept by
	// one window pixel at the given distance. This is a measure of the
	// level of world detail that the pixel should display. This metric
	// takes into account the camera's distance (for basic zoom level), the
	// viewport width, and the viewing angle (for level of detail):
	float dx = dist * tan(frag_arc_angle) / cos(frag_look_angle);

	// Get zoomlevel based on the distance to the camera and the size of a
	// tile pixel under the current viewing angle. The zoom level is
	// inversely proportional to the square of the distance, and has a
	// logarithmic relationship to tile zoom levels. A fudge constant is
	// added to balance the level of detail against tile dimensions:
	int zoom = int((-log2(dx) - 4.5) * latdilute);

	return clamp(zoom, zoom_min, zoom_max);
}

bool sphere_intersect (in vec3 start, in vec3 dir, out vec3 hit, out float det)
{
	// Find the intersection of a ray with the given start point and
	// direction with a unit sphere centered on the origin. Theory here:
	//
	//   https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
	//
	// Distance of ray origin to world origin:
	float startdist = length(start);

	// Dot product of start position with direction:
	float raydot = dot(start, dir);

	// Calculate the value under the square root:
	det = (raydot * raydot) - (startdist * startdist) + 1.0;

	// If this value is negative, no intersection exists:
	if (det < 0.0)
		return false;

	// Get the time value to intersection point:
	float t = sqrt(det) - raydot;

	// If the intersection is behind us, discard:
	if (t >= 0.0)
		return false;

	// Get the intersection point:
	hit = start + t * dir;
	return true;
}

void main (void)
{
	vec3 hit;
	vec2 tile;
	float det;

	// Cast a ray from the camera to p and intersect with the unit sphere:
	if (sphere_intersect(cam, normalize(cam - p), hit, det) == false)
		discard;

	// Get zoomlevel based on distance to camera:
	int zoom = zoomlevel(distance(hit, cam), atanh(hit.y));

	if (sphere_to_tile(hit, zoom, tile) == false) {
		fragcolor = vec4(vec3(0.4), 1.0);
		return;
	}

	// Checkerboard texture:
	fragcolor = bool((int(tile.x) ^ int(tile.y)) & 1)
		? vec4(vec3(0.6), 1.0)
		: vec4(vec3(0.7), 1.0);

	// Angle-based shading:
	fragcolor = shade(fragcolor, det);
}
