package main

import (
	"fmt"
	"os"

	"github.com/g3n/engine/loader/obj"
	"github.com/g3n/engine/math32"
)

type vec3 struct {
	x, y, z float64
}

func (v vec3) Sub(o vec3) vec3 {
	return vec3{
		v.x - o.x,
		v.y - o.y,
		v.z - o.z,
	}
}

func (v vec3) String() string {
	return fmt.Sprintf("{%f, %f, %f}", v.x, v.y, v.z)
}

type face struct {
	v1, v2, v3 int
	mat_index  int
}

func (f face) String() string {
	return fmt.Sprintf("{%d, %d, %d, %d}", f.v1, f.v2, f.v3, f.mat_index)
}

type material struct {
	ambientCol, diffuseCol, specularCol vec3
	Ns                                  float32
}

func (m material) String() string {
	return fmt.Sprintf("{.ambient = %v, .diffuse = %v, .specular = %v, .Ns = %f}", m.ambientCol, m.diffuseCol, m.specularCol, m.Ns)
}

var verts []vec3
var faces []face
var materials []material

var material_map map[string]int = map[string]int{}
var material_count = 0

func get_mat_index(name string) int {
	return material_map[name]
}

func add_material(name string) {
	material_map[name] = material_count
	material_count++
}
func ColorToV3(c math32.Color) vec3 {
	return vec3{
		x: float64(c.R),
		y: float64(c.G),
		z: float64(c.B),
	}
}

func TriNormal(v1, v2, v3 vec3) vec3 {
	var A vec3 = v2.Sub(v1)
	var B vec3 = v3.Sub(v1)
	var Nx float64 = A.y*B.z - A.z*B.y
	var Ny float64 = A.z*B.x - A.x*B.z
	var Nz float64 = A.x*B.y - A.y*B.x
	return vec3{Nx, Ny, Nz}
}

var defualt_material = material{
	ambientCol:  vec3{1.0, 1.0, 1.0},
	diffuseCol:  vec3{1.0, 1.0, 1.0},
	specularCol: vec3{1.0, 1.0, 1.0},
	Ns:          1,
}

func main() {

	mod, err := obj.Decode("Models/cylinder.obj", "Models/cylinder.mtl")
	if err != nil {
		panic(err)
	}
	materials = make([]material, len(mod.Materials))
	// Decode materials
	for name, mat := range mod.Materials {
		add_material(name)
		mat_index := get_mat_index(name)
		materials[mat_index].ambientCol = ColorToV3(mat.Ambient)
		materials[mat_index].diffuseCol = ColorToV3(mat.Diffuse)
		materials[mat_index].specularCol = ColorToV3(mat.Specular)
		materials[mat_index].Ns = float32(mat.Illum)

	}
	if len(materials) == 0 {
		materials = []material{defualt_material}
	}

	mod_vs := mod.Vertices.ToFloat32()
	verts = make([]vec3, len(mod_vs)/3)
	// Decode vertices
	for i := 0; i < len(mod.Vertices); i += 3 {
		verts[i/3] = vec3{
			x: float64(mod_vs[i]),
			y: float64(mod_vs[i+1]),
			z: float64(mod_vs[i+2]),
		}
	}

	faces = make([]face, len(mod.Objects[0].Faces))
	fmt.Printf("Taking %d of %d objects\n", 1, len(mod.Objects))
	fmt.Printf("%d normals, %d faces %d verts\n", mod.Normals.Size(), len(mod.Objects[0].Faces), mod.Vertices.Len())
	// Decode Faces
	for i := 0; i < len(mod.Objects[0].Faces); i++ {
		face := mod.Objects[0].Faces[i]
		if len(face.Vertices) != 3 {
			fmt.Println("TRIANGULATE YOUR MODEL FIRST - IF YOU DID THAT, YOU WOULDNT GET THIS ERROR")
			return
		}
		faces[i].v1 = face.Vertices[0]
		faces[i].v2 = face.Vertices[1]
		faces[i].v3 = face.Vertices[2]
		faces[i].mat_index = get_mat_index(face.Material)
	}

	// calculate world normals
	normals := make([]vec3, len(faces))
	for i := 0; i < len(normals); i++ {
		face := faces[i]
		v1 := verts[face.v1]
		v2 := verts[face.v2]
		v3 := verts[face.v3]
		norm := TriNormal(v1, v2, v3)
		normals[i] = norm
	}

	// Write output
	f, err := os.Create("out.h")
	if err != nil {
		panic(err)
	}
	header := "#pragma once\n#include \"gfx_math.h\"\n#include \"gfx.h\"\n// Model\n"
	f.Write([]byte(header))
	f.WriteString(fmt.Sprintf("const int num_points = %d;\n", len(verts)))
	f.WriteString(fmt.Sprintf("const int num_faces = %d;\n", len(faces)))
	f.WriteString(fmt.Sprintf("const int num_materials = %d;\n", len(materials)))

	// vert array
	f.WriteString("Vec3 points[num_points] = {")
	for i, v := range verts {
		f.WriteString(v.String())
		if i < len(verts)-1 {
			f.WriteString(", ")
		}
	}

	f.WriteString("};\n")
	// face array
	f.WriteString("Tri faces[num_faces] = {")
	for i, face := range faces {
		f.WriteString(face.String())
		if i < len(faces)-1 {
			f.WriteString(", ")
		}
	}
	f.WriteString("};\n")

	// material array
	f.WriteString("Material materials[num_materials] = {")
	for i, mat := range materials {
		f.WriteString(mat.String())
		if i < len(faces)-1 {
			f.WriteString(", ")
		}
	}
	f.WriteString("};\n")

	f.WriteString("Vec3 normals[num_faces] = {")
	for i, n := range normals {
		f.WriteString(n.String())
		if i < len(faces)-1 {
			f.WriteString(", ")
		}
	}
	f.WriteString("};\n")

	f.Close()
}
