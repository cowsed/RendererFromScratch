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

uint32_t color_buffer1[HEIGHT][WIDTH];
float depth_buffer1[HEIGHT][WIDTH];

const Color clear_color = {1.0, 1.0, 1.0};
int pixels_checked = 0;
int pixels_filled = 0;

/*
void fill_tri_tiled(int i, Vec3 v1, Vec3 v2, Vec3 v3)
{

	// pre-compute 1 over z
	v1.z = 1 / v1.z;
	v2.z = 1 / v2.z;
	v3.z = 1 / v3.z;

	Rect bb = bounding_box2d(v1.toVec2(), v2.toVec2(), v3.toVec2());
	Rect bb_pixel = {.min = NDCtoPixels(bb.min), .max = NDCtoPixels(bb.max)};

	// indices into buffer that determine the bounding box
	int minx = (int)max(0, bb_pixel.min.x - 1);
	int miny = (int)max(0, bb_pixel.min.y - 1);
	int maxx = (int)min(bb_pixel.max.x + 1, WIDTH);
	int maxy = (int)min(bb_pixel.max.y + 1, HEIGHT);

	// We don't interpolate vertex attributes, we're filling only one tri at a time -> all this stuff is constant
	Vec3 world_normal = normals[i];
	float amt = world_normal.Dot(light_dir);

	Material mat = materials[faces[i].matID];
	float diff_contrib = amt * (1.0 - ambient_contrib);

	Vec3 amb = {mat.diffuse.r * ambient_contrib, mat.diffuse.g * ambient_contrib, mat.diffuse.b * ambient_contrib};
	Vec3 dif = {mat.diffuse.r * diff_contrib, mat.diffuse.g * diff_contrib, mat.diffuse.b * diff_contrib};
#define BLOCK_SIZE 8
	for (int y = miny; y < maxy; y += BLOCK_SIZE)
	{
		for (int x = minx; x < maxx; x += BLOCK_SIZE)
		{

			if (!(maxy - y < BLOCK_SIZE || maxx - x < BLOCK_SIZE))
			{
				// we have a full tile to work with
				tri_info TL_info = insideTri(v1, v2, v3, PixelToNDC(x, y));
				tri_info TR_info = insideTri(v1, v2, v3, PixelToNDC(x + BLOCK_SIZE - 1, y));
				tri_info BL_info = insideTri(v1, v2, v3, PixelToNDC(x, y + BLOCK_SIZE - 1));
				tri_info BR_info = insideTri(v1, v2, v3, PixelToNDC(x + BLOCK_SIZE - 1, y + BLOCK_SIZE - 1));
				float TL_depth = 1 / (TL_info.w1 * v1.z + TL_info.w2 * v2.z + TL_info.w3 * v3.z);
				float TR_depth = 1 / (TR_info.w1 * v1.z + TR_info.w2 * v2.z + TR_info.w3 * v3.z);
				float BL_depth = 1 / (BL_info.w1 * v1.z + BL_info.w2 * v2.z + BL_info.w3 * v3.z);
				float BR_depth = 1 / (BR_info.w1 * v1.z + BR_info.w2 * v2.z + BR_info.w3 * v3.z);

				if (TL_info.inside && TR_info.inside && BL_info.inside && BR_info.inside)
				{
					// Best Outcome - everything in the tri
					for (int dy = 0; dy < BLOCK_SIZE; dy++)
					{
						for (int dx = 0; dx < BLOCK_SIZE; dx++)
						{
							float dxf = (float)dx / (float)BLOCK_SIZE;
							float dyf = (float)dy / (float)BLOCK_SIZE;
							float top_depth = TL_depth * dxf + TR_depth * (1 - dxf);
							float bot_depth = BL_depth * dxf + BR_depth * (1 - dxf);
							float depth = top_depth * dyf + bot_depth * (1 - dyf);
							if (depth < depth_buffer[y + dy][x + dx])
							{
								color_buffer[y + dy][x + dx] = Vec3ToColor(amb + dif).toIntColor();
								// color_buffer[y + dy][x + dx] = {0, 255, 0, 255};
								depth_buffer[y + dy][x + dx] = depth;
							}
						}
					}
				}
			}

			// Either too small a block or not all were in - gotta go through the old fashioned way
			for (int dy = 0; dy < BLOCK_SIZE; dy++)
			{
				for (int dx = 0; dx < BLOCK_SIZE; dx++)
				{
					tri_info ti = insideTri(v1, v2, v3, PixelToNDC(x + dx, y + dy));
					if (ti.inside)
					{
						float depth = 1 / (ti.w1 * v1.z + ti.w2 * v2.z + ti.w3 * v3.z);
						if (depth > near && depth < far && depth < depth_buffer[y + dy][x + dx])
						{
							color_buffer[y + dy][x + dx] = Vec3ToColor(amb + dif).toIntColor(); // Vec3ToColor({fabs(world_normal.x), fabs(world_normal.y), fabs(world_normal.z)}); //
							depth_buffer[y + dy][x + dx] = depth;
						}
					}
				}
			}
		}
	}
}
*/
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
				pixels_filled++;

				float depth = 1 / (ti.w1 * v1.z + ti.w2 * v2.z + ti.w3 * v3.z);
				if (depth > near && depth < far && depth < depth_buf[y][x])
				{

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
void draw_tri_better(int i, Vec3 ov1, Vec3 ov2, Vec3 ov3, uint32_t color, uint32_t color_buf[HEIGHT][WIDTH], float depth_buf[HEIGHT][WIDTH])
{
	// init
	// top is higher on the screen
	Vec3 top;
	Vec3 mid;
	Vec3 low;

	// original vector less than other vector
	bool ov1_lt_ov2 = ov1.y <= ov2.y;
	bool ov1_lt_ov3 = ov1.y <= ov3.y;
	bool ov2_lt_ov1 = ov2.y <= ov1.y;
	bool ov2_lt_ov3 = ov2.y <= ov3.y;
	bool ov3_lt_ov1 = ov3.y <= ov1.y;
	bool ov3_lt_ov2 = ov3.y <= ov2.y;

	// cursed if tree
	if (ov1_lt_ov2 && ov1_lt_ov3)
	{
		top = ov1;
		if (ov2_lt_ov3)
		{
			mid = ov2;
			low = ov3;
		}
		else
		{
			mid = ov3;
			low = ov2;
		}
	}
	else if (ov2_lt_ov1 && ov2_lt_ov3)
	{

		top = ov2;
		if (ov1_lt_ov3)
		{
			mid = ov1;
			low = ov3;
		}
		else
		{
			mid = ov3;
			low = ov1;
		}
	}
	else
	{
		top = ov3;
		if (ov1_lt_ov2)
		{
			mid = ov1;
			low = ov2;
		}
		else
		{
			mid = ov2;
			low = ov1;
		}
	}

	int miny = (int)(top.y + .5);
	int midy = (int)(mid.y + .5);
	int maxy = (int)(low.y + .5);

	Vec3 top2mid = (mid - top);
	Vec3 top2low = (low - top);
	Vec3 mid2low = (mid - low);

	float short_side_dx = top2mid.x / top2mid.y;
	float long_side_dx = top2low.x / top2low.y;

	Vec3 short_side_pos = top;
	Vec3 long_side_pos = top;

	for (int y = max(0, miny - 1); y <= min(HEIGHT, maxy); y++)
	{
		// switch direction when we pass midy
		if (y == midy)
		{
			short_side_dx = mid2low.x / mid2low.y;
			short_side_pos = mid;
		}
		int short_side_index = (int)(short_side_pos.x + .5);
		int long_side_index = (int)(long_side_pos.x + .5);
		int direction = sign(long_side_index - short_side_index);
		short_side_index -= direction; // extra padding to avoid off by one errors
		long_side_index += direction;  // extra padding to avoid off by one errors
		short_side_index = clamp(short_side_index, 0, WIDTH);
		long_side_index = clamp(long_side_index, 0, WIDTH);

		direction = sign(long_side_index - short_side_index); // necessary sometimes cuz direction can get mess up ? todo figure out why this happens

		int x = short_side_index;
		while (x != long_side_index + direction && x < WIDTH && x > 0) // + direction to do <= typa thing
		{
			Vec3 fov1 = PixelToNDC3(ov1.x, ov1.y, ov1.z);
			Vec3 fov2 = PixelToNDC3(ov2.x, ov2.y, ov2.z);
			Vec3 fov3 = PixelToNDC3(ov3.x, ov3.y, ov3.z);
			const tri_info ti = insideTri(fov1, fov2, fov3, PixelToNDC(x, y));
			float depth = 1 / (ti.w1 * ov1.z + ti.w2 * ov2.z + ti.w3 * ov3.z);
			if (ti.inside)
			{
				pixels_filled++;
				if ((depth > near) && (depth < far) && (depth < depth_buf[y][x]))
				{
					color_buf[y][x] = color; // color;
					depth_buf[y][x] = depth;
				}
			}
			x += direction;
		}

		short_side_pos.x += short_side_dx;
		short_side_pos.y += 1;
		long_side_pos.x += long_side_dx;
		long_side_pos.y += 1;
	}
	// std::cerr << "ssp: " << short_side_pos << " lsp: " << long_side_pos << std::endl;
}

inline void fill_tri_trishaped(int i, Vec3 ov1, Vec3 ov2, Vec3 ov3, uint32_t color_buf[HEIGHT][WIDTH], float depth_buf[HEIGHT][WIDTH])
{

	// pre-compute 1 over z
	ov1.z = 1 / ov1.z;
	ov2.z = 1 / ov2.z;
	ov3.z = 1 / ov3.z;

	// We don't interpolate vertex attributes, we're filling only one tri at a time -> all this stuff is constant
	const Vec3 world_normal = normals[i];
	const float amt = world_normal.Dot(light_dir);

	const Material mat = materials[faces[i].matID];
	const float diff_contrib = amt * (1.0 - ambient_contrib);

	const Vec3 amb = mat.diffuse.toVec3() * ambient_contrib; //{mat.diffuse.r * ambient_contrib, mat.diffuse.g * ambient_contrib, mat.diffuse.b * ambient_contrib};
	const Vec3 dif = mat.diffuse.toVec3() * diff_contrib;
	const uint32_t col = Vec3ToColor(amb + dif).toIntColor();

	ov1 = NDCtoPixels3(ov1);
	ov2 = NDCtoPixels3(ov2);
	ov3 = NDCtoPixels3(ov3);

	draw_tri_better(i, ov1, ov2, ov3, col, color_buf, depth_buf);
}

/*
const tri_info ti = insideTri(v1, v2, v3, PixelToNDC(x, y));
if (ti.inside)
{
pixels_filled++;

float depth = 1 / (ti.w1 * v1.z + ti.w2 * v2.z + ti.w3 * v3.z);
if (depth > near && depth < far && depth < depth_buf[y][x])
{

color_buf[y][x] = Vec3ToColor(amb + dif).toIntColor(); // Vec3ToColor({fabs(world_normal.x), fabs(world_normal.y), fabs(world_normal.z)}); //
depth_buf[y][x] = depth;
}
}*/

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

void render(uint32_t color[HEIGHT][WIDTH], float depth[HEIGHT][WIDTH], double time_seconds)
{
	clear_buffers(color, depth);

	// Project all the points to screen space
	Vec3 projected_points[num_points];
	Vec3 cam_projected_points[num_points];
	float aspect = width / height;

	const Mat4 trans = Translate3D({0, 0, -10});
	const Mat4 rotx = RotateX(0 * M_PI / 14.0);
	const Mat4 roty = RotateX(2 * M_PI / 4.0);
	const Mat4 transform = rotx * (trans * roty);

	// std::cerr << "transform:\n"
	// 		  << transform << std::endl;

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
		//   fill_tri(i, v1, v2, v3, color, depth); // 40ms 30ms when inlined
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
			uint32_t c = color_buffer1[y][x];
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

	clear_buffers(color_buffer1, depth_buffer1);

	// // v1 = (161.93, 120, 0.116472), v2 = (78.0702, 120, 0.116472), v3 = (84, 170.912, 0.1)
	// Vec3 v1 = {161.93, 120.0, 0.116472}; // mid
	// Vec3 v2 = {78.0702, 120, 0.116472};	 // low
	// Vec3 v3 = {84, 170.912, 0.1};		 // top

	// std::cerr << "v1 = " << v1 << ", v2 = " << v2 << ", v3 = " << v3 << std::endl;

	// draw_tri_better(0, v1, v2, v3, 0xFFFF0000, color_buffer1, depth_buffer1);
	// output_to_stdout(color_buffer1);

	// return 0;
	// draw_tri_fast(v1, v2,v3, 0xFF0000FF, color_buffer1, depth_buffer1);

	const int runs = 1;
	std::cerr << "Rendering" << std::endl;
	precalculate();
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	float time = 0.0;
	for (int i = 0; i < runs; i++)
	{
		render(color_buffer1, depth_buffer1, time);
	}

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cerr << "Finished Rendering in " << duration.count() << "ms" << std::endl;
	std::cerr << runs << " runs. " << duration.count() / runs << " ms each" << std::endl;

	std::cerr << "Outputting" << std::endl;
	std::cerr << "Checked: " << pixels_checked << ". Filled: " << pixels_filled << std::endl;
	output_to_stdout(color_buffer1);
}
/*
	bool buffer1 = true;
	for (int i = 0; i < runs; i++)
	{
		if (buffer1)
		{
			render(color_buffer1, depth_buffer1);
		} else {
			render(color_buffer2, depth_buffer2);
		}
		buffer1 = !buffer1;
	}
*/