#include <iostream>
#include <chrono>

#include "gfx_math.h"
#include "gfx.h"
#include "model.h"

#define WIDTH (1 * 240)
#define HEIGHT (1 * 240)
const float width = (float)(WIDTH);
const float height = (float)(HEIGHT);

const float ambient_contrib = 0.6;
const Vec3 light_dir = Vec3({1, 1, -1}).Normalize();
const float fov = 3.0; // in no units in particular. higher is 'more zoomed in'
const float near = 1.0;
const float far = 100.0;
const bool do_backface_culling = true;
const Vec3 view_dir_cam_space = Vec3(0, 0, -1.0);
const bool DB = false;

// transforms from [-1, 1] to [0, Width]
Vec2 NDCtoPixels(const Vec2 &v)
{
	float newx = ((v.x / 2.0) + 0.5) * WIDTH;
	float newy = ((v.y / 2.0) + 0.5) * HEIGHT;
	return {newx, newy};
}
Vec3 NDCtoPixels3(const Vec3 &v)
{
	float newx = ((v.x / 2.0) + 0.5) * WIDTH;
	float newy = ((v.y / 2.0) + 0.5) * HEIGHT;
	return {newx, newy, v.z};
}
// pixel coordinates [0, WIDTH), [0, HEIGHT) to NDC [-1, 1], [-1, 1]
Vec3 PixelToNDC(const int x, const int y)
{
	const float xf = (float)x;
	const float yf = (float)y;
	Vec3 pixel_NDC = Vec3(2 * xf / width - 1.0f, 2 * yf / height - 1.0f, 0.0);
	return pixel_NDC;
}
Vec3 PixelToNDC3(const float xf, const float yf, const float z)
{
	Vec3 pixel_NDC = Vec3(2 * xf / width - 1.0f, 2 * yf / height - 1.0f, z);
	return pixel_NDC;
}

uint32_t color_buffer[HEIGHT][WIDTH];
float depth_buffer[HEIGHT][WIDTH];

const Color clear_color = {1.0, 1.0, 1.0};
int pixels_checked = 0;
int pixels_filled = 0;

inline void fill_tri(int i, Vec3 v1, Vec3 v2, Vec3 v3, uint32_t color_buf[HEIGHT][WIDTH], float depth_buf[HEIGHT][WIDTH])
{
	// pre-compute 1 over z
	v1.z = 1 / v1.z;
	v2.z = 1 / v2.z;
	v3.z = 1 / v3.z;

	const Rect bb = bounding_box2d(v1.toVec2(), v2.toVec2(), v3.toVec2());
	const Rect bb_pixel = {.min = NDCtoPixels(bb.min), .max = NDCtoPixels(bb.max)};

	// indices into buffer that determine the bounding box
	const int minx = (int)max(0, bb_pixel.min.x - 1);
	const int miny = (int)max(0, bb_pixel.min.y - 1);
	const int maxx = (int)min(bb_pixel.max.x + 1, WIDTH);
	const int maxy = (int)min(bb_pixel.max.y + 1, HEIGHT);

	// We don't interpolate vertex attributes, we're filling only one tri at a time -> all this stuff is constant
	const Vec3 world_normal = normals[i];
	const float amt = world_normal.Dot(light_dir);

	const Material mat = materials[faces[i].matID];
	const float diff_contrib = amt * (1.0 - ambient_contrib);

	const Vec3 amb = mat.diffuse.toVec3() * ambient_contrib; //{mat.diffuse.r * ambient_contrib, mat.diffuse.g * ambient_contrib, mat.diffuse.b * ambient_contrib};
	const Vec3 dif = mat.diffuse.toVec3() * diff_contrib;

	for (int y = miny; y < maxy; y += 1)
	{
		for (int x = minx; x < maxx; x += 1)
		{
			pixels_checked++;
			const tri_info ti = insideTri(v1, v2, v3, PixelToNDC(x, y));
			if (ti.inside)
			{

				float depth = 1 / (ti.w1 * v1.z + ti.w2 * v2.z + ti.w3 * v3.z);

				if (depth > near && depth < far && depth < depth_buf[y][x])
				{
					pixels_filled++;
					color_buf[y][x] = Vec3ToColor(amb + dif).toIntColor(); // Vec3ToColor({fabs(world_normal.x), fabs(world_normal.y), fabs(world_normal.z)}); //
					depth_buf[y][x] = depth;
				}
			}
		}
	}
}

int sign(int a)
{
	if (a < 0)
	{
		return -1;
	}
	return 1;
}

struct bresenham_info
{
	int x;
	int y;
	int p;
};

typedef void (*bresenham_step_func)(int dx, int dy, bresenham_info &bh);

void bresenham_step_steep_left(int dx, int dy, bresenham_info &bh)
{
	bh.y++;
	if (bh.p < 0)
	{
		bh.p = bh.p + 2 * -dx;
	}
	else
	{
		bh.p = bh.p + 2 * -dx - 2 * dy;
		bh.x--;
	}
}
void bresenham_step_shallow_left(int dx, int dy, bresenham_info &bh)
{
	int starty = bh.y;
	while (bh.y == starty && bh.x > 0)
	{
		bh.x--;
		if (bh.p < 0)
		{
			bh.p = bh.p + 2 * dy;
		}
		else
		{
			bh.p = bh.p + 2 * dy - 2 * -dx;
			bh.y++;
		}
	}
	while (bh.y == starty + 1 && bh.x > 0)
	{
		bh.x--;
		if (bh.p < 0)
		{
			bh.p = bh.p + 2 * dy;
		}
		else
		{
			bh.x++;
			break;
		}
	}
}
void bresenham_step_shallow_right(int dx, int dy, bresenham_info &bh)
{
	int starty = bh.y;
	while (bh.y == starty && bh.x < WIDTH)
	{
		bh.x++;
		if (bh.p < 0)
		{
			bh.p = bh.p + 2 * dy;
		}
		else
		{
			bh.p = bh.p + 2 * dy - 2 * dx;
			bh.y++;
		}
	}
	while (bh.y == starty + 1 && bh.x < WIDTH)
	{
		bh.x++;
		if (bh.p < 0)
		{
			bh.p = bh.p + 2 * dy;
		}
		else
		{
			bh.x--;
			break;
		}
	}
}
void bresenham_step_steep_right(int dx, int dy, bresenham_info &bh)
{
	bh.y++;
	if (bh.p < 0)
	{
		bh.p = bh.p + 2 * dx;
	}
	else
	{
		bh.p = bh.p + 2 * dx - 2 * dy;
		bh.x++;
	}
}

struct bresenham_config
{
	int dx;
	int dy;
	bresenham_info info;
	bresenham_step_func step_func;
	void stepY()
	{
		step_func(dx, dy, info);
	}
};

bresenham_config make_line(Vec3 v0_pixels, Vec3 v1_pixels)
{
	int x0 = (int)(v0_pixels.x + .5);
	int y0 = (int)(v0_pixels.y + .5);

	int x1 = (int)(v1_pixels.x + .5);
	int y1 = (int)(v1_pixels.y + .5);

	int dy = y1 - y0;
	int dx = x1 - x0;

	// line is steep downwards or shallow
	bool shallow = true;
	if (abs(dy) > abs(dx))
	{
		shallow = false;
	}
	// line is going left/right
	bool left = true;
	if (dx > 0)
	{
		left = false;
	}

	if (!left && !shallow)
	{
		return {.dx = dx, .dy = dy, .info = {.x = x0, .y = y0, .p = 2 * dx - dy}, .step_func = bresenham_step_steep_right};
	}
	else if (left && !shallow)
	{
		return {.dx = dx, .dy = dy, .info = {.x = x0, .y = y0, .p = 2 * -dx - dy}, .step_func = bresenham_step_steep_left};
	}
	else if (!left && shallow)
	{
		return {.dx = dx, .dy = dy, .info = {.x = x0, .y = y0, .p = 2 * dy - dx}, .step_func = bresenham_step_shallow_right};
	}
	else
	{ // left shallow
		return {.dx = dx, .dy = dy, .info = {.x = x0, .y = y0, .p = 2 * dy + dx}, .step_func = bresenham_step_shallow_left};
	}
}


float lerp(float start, float end, float T){
	return T * start + (1 - T) * end;
}

void bresenhamTri(int i, Vec3 v1, Vec3 v2, Vec3 v3, uint32_t color, uint32_t color_buf[HEIGHT][WIDTH], float depth_buf[HEIGHT][WIDTH])
{
	// std::cerr << "v1 = " << v1 << ", v2 = " << v2 << ", v3 = " << v3 << std::endl;
	//  top is higher on the screen (lower y value)
	Vec3 top;
	Vec3 mid;
	Vec3 low;
	// low is lower on the screen (higher y value)

	// Sort vectors
	{
		// original vector less than other vector
		bool v1_lt_v2 = v1.y <= v2.y;
		bool v1_lt_v3 = v1.y <= v3.y;
		bool v2_lt_v1 = v2.y <= v1.y;
		bool v2_lt_v3 = v2.y <= v3.y;
		bool v3_lt_v1 = v3.y <= v1.y;
		bool v3_lt_v2 = v3.y <= v2.y;

		// cursed if tree
		if (v1_lt_v2 && v1_lt_v3)
		{
			top = v1;
			if (v2_lt_v3)
			{
				mid = v2;
				low = v3;
			}
			else
			{
				mid = v3;
				low = v2;
			}
		}
		else if (v2_lt_v1 && v2_lt_v3)
		{

			top = v2;
			if (v1_lt_v3)
			{
				mid = v1;
				low = v3;
			}
			else
			{
				mid = v3;
				low = v1;
			}
		}
		else
		{
			top = v3;
			if (v1_lt_v2)
			{
				mid = v1;
				low = v2;
			}
			else
			{
				mid = v2;
				low = v1;
			}
		}
	}

	Vec3 topPixels = NDCtoPixels3(top);
	Vec3 midPixels = NDCtoPixels3(mid);
	Vec3 lowPixels = NDCtoPixels3(low);

	// std::cerr << "topP = " << topPixels << ", midP = " << midPixels << ", lowP = " << lowPixels << std::endl;

	int miny = (int)(topPixels.y + .5);
	int midy = (int)(midPixels.y + .5);
	int maxy = (int)(lowPixels.y + .5);

	bresenham_config top2mid = make_line(topPixels, midPixels);
	bresenham_config top2low = make_line(topPixels, lowPixels);
	bresenham_config mid2low = make_line(midPixels, lowPixels);

	// take the inverse so that the linear interpolation below can do perspective correct interpolation
	float topInvDepth = 1 / top.z;
	float midInvDepth = 1 / mid.z;
	float lowInvDepth = 1 / low.z;

	int top_half_height = midy - miny;
	int bot_half_height = maxy - midy + 1;

	if (bot_half_height == 1)
	{
		if (top_half_height == 0)
		{
			// there are no pixels we can draw here
			return;
		}
		// no need to do all the work for one more row that will be the same (also second half breaks sometimes)

		top_half_height += 1;
		midy++;
		bot_half_height = 0;
	}

	float short_side_inv_depth = topInvDepth;
	float long_side_inv_depth = topInvDepth;

	float short_side_inv_depth_per_y = (midInvDepth - topInvDepth) / (float)top_half_height;
	float short_side2_inv_depth_per_y = (lowInvDepth - midInvDepth) / (float)bot_half_height;
	float long_side_inv_depth_per_y = (lowInvDepth - topInvDepth) / (float)(top_half_height + bot_half_height);

	// std::cerr << "Starting Part 1 " << i << std::endl;

	while (top2low.info.y < midy && top_half_height > 0)
	{
		int width = abs(top2low.info.x - top2mid.info.x);
		int direction = sign(top2low.info.x - top2mid.info.x);

		float inv_depth_per_x = (long_side_inv_depth - short_side_inv_depth) / (float)width;
		float inv_depth = short_side_inv_depth;
		int y = top2low.info.y;
		int x = top2mid.info.x;
		while (x != top2low.info.x + direction) // + direction for <= vibes rather than <
		{
			float depth = 1 / inv_depth;
			// std::cerr << "(" << x << ", " << y << ")" << std::endl;
			//  std::cerr << "top depth " << depth << std::endl;

			if (depth > near && depth < far && depth < depth_buf[y][x])
			{
				pixels_filled++;
				color_buf[y][x] = color;
				depth_buf[y][x] = depth;
			}

			inv_depth += inv_depth_per_x;
			x += direction;
		}

		top2low.stepY();
		top2mid.stepY();
		short_side_inv_depth += short_side_inv_depth_per_y;
		long_side_inv_depth += long_side_inv_depth_per_y;
	}
	short_side_inv_depth = midInvDepth;
	// std::cerr << "Starting Part 2" << std::endl;
	while (top2low.info.y <= maxy && bot_half_height > 0)
	{
		int width = abs(top2low.info.x - mid2low.info.x);
		int direction = sign(top2low.info.x - mid2low.info.x);

		float inv_depth_per_x = (long_side_inv_depth - short_side_inv_depth) / (float)width;
		float inv_depth = short_side_inv_depth;

		int y = top2low.info.y;
		int x = mid2low.info.x;
		while (x != top2low.info.x + direction) // + direction for <= vibes rather than <
		{
			float depth = 1 / inv_depth;
			// std::cerr << "(" << x << ", " << y << ")" << std::endl;
			// std::cerr << "bot depth " << depth << std::endl;
			if (depth > near && depth < far && depth < depth_buf[y][x])
			{
				pixels_filled++;
				color_buf[y][x] = color;
				depth_buf[y][x] = depth;
			}

			inv_depth += inv_depth_per_x;
			x += direction;
		}

		top2low.stepY();
		mid2low.stepY();
		short_side_inv_depth += short_side2_inv_depth_per_y;
		long_side_inv_depth += long_side_inv_depth_per_y;
	}
}

inline void fill_tri_trishaped(int i, Vec3 ov1, Vec3 ov2, Vec3 ov3, uint32_t color_buf[HEIGHT][WIDTH], float depth_buf[HEIGHT][WIDTH])
{

	// pre-compute 1 over z
	// ov1.z = 1 / ov1.z;
	// ov2.z = 1 / ov2.z;
	// ov3.z = 1 / ov3.z;

	// We don't interpolate vertex attributes, we're filling only one tri at a time -> all this stuff is constant
	const Vec3 world_normal = normals[i];
	const float amt = world_normal.Dot(light_dir);

	const Material mat = materials[faces[i].matID];
	const float diff_contrib = amt * (1.0 - ambient_contrib);

	const Vec3 amb = mat.diffuse.toVec3() * ambient_contrib; //{mat.diffuse.r * ambient_contrib, mat.diffuse.g * ambient_contrib, mat.diffuse.b * ambient_contrib};
	const Vec3 dif = mat.diffuse.toVec3() * diff_contrib;
	const uint32_t col = Vec3ToColor(amb + dif).toIntColor();

	// std::cerr << "starting tri" << i << std::endl;
	//bresenhamTri(i, ov1, ov2, ov3, col, color_buf, depth_buf);
	
}

void precalculate()
{
	// Precalculate world normals
	for (int i = 0; i < num_faces; i++)
	{
		Tri t = faces[i];
		Vec3 world_normal = TriNormal(points[t.v1_index], points[t.v2_index], points[t.v3_index]).Normalize(); // normals[i]; //
		normals[i] = world_normal;
	}
}

void clear_buffers(uint32_t color[HEIGHT][WIDTH], float depth[HEIGHT][WIDTH])
{
	// Clear all pixels
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			color[y][x] = clear_color.toIntColor();
			depth[y][x] = far + 1;
		}
	}
}

void render(uint32_t color[HEIGHT][WIDTH], float depth[HEIGHT][WIDTH], float time_seconds)
{
	clear_buffers(color, depth);

	// Project all the points to screen space
	Vec3 projected_points[num_points];
	Vec3 cam_projected_points[num_points];
	float aspect = width / height;

	const Mat4 trans = Translate3D({0, 0, -10});
	const Mat4 rotx = RotateX(0 * M_PI / 14.0);
	const Mat4 roty = RotateY(5 * M_PI / 4.0);
	const Mat4 transform = rotx * (trans * roty);

	for (int i = 0; i < num_points; i++)
	{
		Vec4 p = points[i].toVec4(1.0);
		// transform p to camera space

		Vec4 p_cam = transform.Mul4xV4(p); //(p.toVec3().RotateY(5 * M_PI / 4) - Vec3(0, 2.5, 10)).RotateZ(M_PI / 16).toVec4(1.0); // transform from world space to camera space
		cam_projected_points[i] = p_cam.toVec3();

		// divide by depth. things farther away are smaller
		Vec4 p_NDC = {fov * near * p_cam.x / -p_cam.z, fov * near * p_cam.y / -p_cam.z, -p_cam.z};

		// negate Y (+1 is up in graphics, +1 is down in image)
		//  do correction for aspect ratio
		Vec3 p_screen = {p_NDC.x, -p_NDC.y * aspect, p_NDC.z};
		projected_points[i] = p_screen;
	}

	// Assemble triangles and color they pixels
	for (int i = 0; i < num_faces; i++)
	{
		const Tri t = faces[i];
		const Vec3 v1 = projected_points[t.v1_index];
		const Vec3 v2 = projected_points[t.v2_index];
		const Vec3 v3 = projected_points[t.v3_index];

		if (do_backface_culling)
		{
			Vec3 cam_space_normal = TriNormal(cam_projected_points[t.v1_index], cam_projected_points[t.v2_index], cam_projected_points[t.v3_index]);
			if ((view_dir_cam_space.Dot(cam_space_normal) >= 0.1))
			{
				// backface cull this dude
				continue;
			}
		}

		// fill_tri_tiled(i, v1, v2, v3); // 53ms
		fill_tri(i, v1, v2, v3, color, depth);
		//fill_tri_trishaped(i, v1, v2, v3, color, depth); // 40ms 30ms when inlined
		//  fill_tri(i, v1, v2, v3, color, depth); // 40ms 30ms when inlined
	//1679200
	//2133700
	}
}

void output_to_stdout(uint32_t color[HEIGHT][WIDTH])
{
	std::cout << "P3" << std::endl;
	std::cout << WIDTH << " " << HEIGHT << std::endl;
	std::cout << "255" << std::endl;

	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			uint32_t c = color_buffer[y][x];
			int r = (c & 0b111111110000000000000000) >> 16;
			int g = (c & 0b1111111100000000) >> 8;
			int b = (c & 0b11111111);

			std::cout << r << " " << g << " " << b << " ";
		}
		std::cout << "\n";
	}
}

int main()
{
	// v1 = (0.3, 0.3, 0.1), v2 = (0.3, -0.3, 0.1), v3 = (-0.3, -0.3, 0.1)
	// // v1 = (0.3, 0.3, 0.1), v2 = (0.3, -0.3, 0.1), v3 = (-0.3, -0.3, 0.1)
	// clear_buffers(color_buffer, depth_buffer);
	// Vec3 v0 = {0.3, 0.3, 10};
	// Vec3 v1 = {-.3, .3, 10};
	// Vec3 v2 = {-.3, -.3, 10};

	// bresenhamTri(v0, v1, v2, 0xFFFF0000, color_buffer, depth_buffer);
	// //fill_tri(1, v0, v1, v2, color_buffer, depth_buffer);
	// output_to_stdout(color_buffer);

	// return 0;

	const int runs = 100;
	std::cerr << "Rendering" << std::endl;
	precalculate();
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	float time = 0.0;
	for (int i = 0; i < runs; i++)
	{
		render(color_buffer, depth_buffer, time);
	}

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cerr << "Finished Rendering in " << duration.count() << "ms" << std::endl;
	std::cerr << runs << " runs. " << duration.count() / runs << " ms each" << std::endl;

	std::cerr << "Outputting" << std::endl;
	std::cerr << "Checked: " << pixels_checked << ". Filled: " << pixels_filled << std::endl;
	output_to_stdout(color_buffer);
}
