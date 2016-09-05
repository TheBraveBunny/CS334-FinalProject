
layout(location = 0) in vec3 vp; // positions from mesh
layout(location = 1) in vec2 vt;
layout(location = 2) in vec3 vn; // normals from mesh

uniform mat4 P, V, M; // proj, view, model matrices

out vec3 point;
out vec3 eye;
out vec2 coord;
out vec3 normal;

void main () {
	point = vp;
	coord = vt;
	eye = vec3 (V * M * vec4 (vp, 1.0));
	normal = vec3 (V * M * vec4 (vn, 0.0));
	gl_Position = P * V * M * vec4 (vp, 1.0);
}