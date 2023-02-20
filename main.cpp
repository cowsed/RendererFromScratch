#include <iostream>
#include <chrono>

#include "gfx_math.h"
#include "gfx.h"
#include "model.h"

#define WIDTH (1 * 480)
#define HEIGHT (1 * 240)

// transforms from [-1, 1] to [0, Width]
Vec2 NDCtoPixels(Vec2 v)
{
	float newx = ((v.x / 2.0) + 0.5) * WIDTH;
	float newy = ((v.y / 2.0) + 0.5) * HEIGHT;
	return {newx, newy};
}

Color color_buffer[HEIGHT][WIDTH];
float depth_buffer[HEIGHT][WIDTH];
Color clear_color = {0.0, 0.0, 0.0};

void render(Camera &cam)
{
	float width = (float)(WIDTH);
	float height = (float)(HEIGHT);

	float fov = 2.0; // in no units in particular. bigger is 'more zoomed in'
	float near = 1.0;
	float far = 100.0;

	// Clear all pixels
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			color_buffer[y][x] = clear_color;
			depth_buffer[y][x] = far + 1;
		}
	}

	// Project all the points to screen space
	Vec3 projected_points[num_points];
	float aspect = width / height;
	for (int i = 0; i < num_points; i++)
	{
		Vec3 p = points[i];
		Vec3 p_cam = p.Sub({0, 0, 5}); // transform from world space to camera space
		// divide by depth. things farther away are smaller
		Vec3 p_NDC = {fov * near * p_cam.x / -p_cam.z, fov * near * p_cam.y / -p_cam.z, -p_cam.z};

		// negate Y (+1 is up in graphics, +1 is down in image)
		//  do correction for aspect ratio
		Vec3 p_screen = {p_NDC.x, -p_NDC.y * aspect, p_NDC.z};
		projected_points[i] = p_screen;
	}

	// Assemble triangles and color they pixels
	for (int i = 0; i < num_faces; i++)
	{
		Tri t = faces[i];
		Vec3 v1 = projected_points[t.v1_index];
		Vec3 v2 = projected_points[t.v2_index];
		Vec3 v3 = projected_points[t.v3_index];

		Rect bb = bounding_box2d(v1.toVec2(), v2.toVec2(), v3.toVec2());
		Rect bb_pixel = {.min = NDCtoPixels(bb.min), .max = NDCtoPixels(bb.max)};

		// indices into buffer that determine the bounding box
		int minx = (int)max(0, bb_pixel.min.x - 1);
		int miny = (int)max(0, bb_pixel.min.y - 1);
		int maxx = (int)min(bb_pixel.max.x + 1, WIDTH);
		int maxy = (int)min(bb_pixel.max.y + 1, HEIGHT);

		// Iterate through pixels
		for (int y = miny; y < maxy; y++)
		{
			for (int x = minx; x < maxx; x++)
			{
				float width = (float)(WIDTH);
				float height = (float)(HEIGHT);
				float xf = (float)x;
				float yf = (float)y;

				Vec3 pixel_screen_space = {.x = 2 * x / width - 1.0f, .y = 2 * y / height - 1.0f, .z = 0.0};

				bool inside = insideTri(v1, v2, v3, pixel_screen_space);
				Vec3 world_normal = normals[i];

				if (inside)
				{
					float depth = v1.z; // todo interpolate this
					if (depth > near && depth < far && depth < depth_buffer[y][x])
					{
						color_buffer[y][x] = Vec3ToColor(world_normal.Normalize());
						depth_buffer[y][x] = depth;

						// uncomment to render the depth
						//float depth_col = depth / (10);
						//color_buffer[y][x] = Vec3ToColor({depth_col, depth_col, depth_col});
					}
				}
			}
		}
	}
}

void output_to_stdout()
{
	std::cout << "P3" << std::endl;
	std::cout << WIDTH << " " << HEIGHT << std::endl;
	std::cout << "255" << std::endl;

	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			Color c = color_buffer[y][x];
			if (c.r > 1.0)
			{
				c.r = 1.0;
			}
			if (c.g > 1.0)
			{
				c.g = 1.0;
			}
			if (c.b > 1.0)
			{
				c.b = 1.0;
			}
			std::cout << (int)(c.r * 255.99) << " " << (int)(c.g * 255.99) << " " << (int)(c.b * 255.99) << " ";
		}
		std::cout << "\n";
	}
}

int main()
{

	Camera cam = {
		.pos = {0.0, 0.0, 6.0},
		.target = {0.0, 0.0, 0.0},

	};
	std::cerr << "Rendering" << std::endl;
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	render(cam);
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(end - start);
	std::cerr << "Finished Rendering in " << duration.count() << "μs" <<std::endl;
	std::cerr << "Outputting" << std::endl;
	output_to_stdout();
}
