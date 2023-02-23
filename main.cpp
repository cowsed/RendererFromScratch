#include <iostream>
#include <chrono>

#include "gfx_math.h"
#include "gfx.h"
#include "model.h"

#define WIDTH (2 * 240)
#define HEIGHT (1 * 240)

// transforms from [-1, 1] to [0, Width]
Vec2 NDCtoPixels(Vec2 v)
{
	float newx = ((v.x / 2.0) + 0.5) * WIDTH;
	float newy = ((v.y / 2.0) + 0.5) * HEIGHT;
	return {newx, newy};
}

IntColor color_buffer[HEIGHT][WIDTH];
float depth_buffer[HEIGHT][WIDTH];
Color clear_color = {1.0, 1.0, 1.0};

inline float edgeFunction(const Vec3 &a, const Vec3 &b, const Vec3 &c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

const float ambient_contrib = 0.6;
const Vec3 light_dir = Vec3({1, 1, -1}).Normalize();
const float fov = 2.0; // in no units in particular. higher is 'more zoomed in'
const float near = 1.0;
const float far = 100.0;
const float width = (float)(WIDTH);
const float height = (float)(HEIGHT);
const bool do_backface_culling = true;
const Vec3 view_dir_cam_space = Vec3(0, 0, -1.0);

void fill_tri(int i, Vec3 v1, Vec3 v2, Vec3 v3)
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
	

	// Iterate through pixels
	for (int y = miny; y < maxy; y++)
	{
		for (int x = minx; x < maxx; x++)
		{
			float xf = (float)x + .5; // middle of pixel
			float yf = (float)y + .5; // middle of pixel

			Vec3 pixel_NDC = Vec3(2 * xf / width - 1.0f, 2 * yf / height - 1.0f, 0.0);

			float area = edgeFunction(v1, v2, v3);
			float w1 = edgeFunction(v2, v3, pixel_NDC) / area;
			float w2 = edgeFunction(v3, v1, pixel_NDC) / area;
			float w3 = edgeFunction(v1, v2, pixel_NDC) / area;
			bool inside = (w1 >= 0) && (w2 >= 0) && (w3 >= 0);
			if (inside)
			{

				float depth = 1 / (w1 * v1.z + w2 * v2.z + w3 * v3.z);
				if (depth > near && depth < far && depth < depth_buffer[y][x])
				{
					color_buffer[y][x] = Vec3ToColor(amb + dif).toIntColor(); // Vec3ToColor({fabs(world_normal.x), fabs(world_normal.y), fabs(world_normal.z)}); //
					depth_buffer[y][x] = depth;

					// uncomment to render the depth
					// float depth_col = depth / (10);
					// color_buffer[y][x] = Vec3ToColor({depth_col, depth_col, depth_col});
				}
			}
		}
	}
}

void render()
{

	// Clear all pixels
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			color_buffer[y][x] = clear_color.toIntColor();
			depth_buffer[y][x] = far + 1;
		}
	}

	// Precalculate world normals
	for (int i = 0; i < num_faces; i++)
	{
		Tri t = faces[i];
		Vec3 world_normal = TriNormal(points[t.v1_index], points[t.v2_index], points[t.v3_index]).Normalize(); // normals[i]; //
		normals[i] = world_normal;
	}

	// Project all the points to screen space
	Vec3 projected_points[num_points];
	Vec3 cam_projected_points[num_points];
	float aspect = width / height;

	Mat4 trans = Translate3D({0, -2.5, -10});
	Mat4 rotx = RotateX(1 * M_PI / 14.0);
	Mat4 roty = RotateY(5 * M_PI / 4.0);
	Mat4 transform = rotx * (trans * roty);

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
		Tri t = faces[i];
		Vec3 v1 = projected_points[t.v1_index];
		Vec3 v2 = projected_points[t.v2_index];
		Vec3 v3 = projected_points[t.v3_index];

		if (do_backface_culling)
		{
			Vec3 cam_space_normal = TriNormal(cam_projected_points[t.v1_index], cam_projected_points[t.v2_index], cam_projected_points[t.v3_index]);
			if ((view_dir_cam_space.Dot(cam_space_normal) >= 0.1))
			{
				// backface cull this dude
				continue;
			}
		}

		fill_tri(i, v1, v2, v3);
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
			IntColor c = color_buffer[y][x];

			std::cout << (int)c.r << " " << (int)c.g << " " << (int)c.b << " ";
		}
		std::cout << "\n";
	}
}

int main()
{

	std::cerr << "Rendering" << std::endl;
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	render();
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cerr << "Finished Rendering in " << duration.count() << "ms" << std::endl;
	std::cerr << "Outputting" << color_buffer[0][1].r << std::endl;
	// output_to_stdout();
}
