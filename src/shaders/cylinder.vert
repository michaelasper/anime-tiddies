R"zzz(#version 330 core
uniform mat4 bone_transform; // transform the cylinder to the correct configuration
const float kPi = 3.1415926535897932384626433832795;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
in vec4 vertex_position;

// FIXME: Implement your vertex shader for cylinders
// Note: you need call sin/cos to transform the input mesh to a cylinder
void main() {
    vec4 cylinder = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cylinder.y = cos(2 * kPi * vertex_position.x);
	cylinder.z = sin(2 * kPi * vertex_position.x);
	cylinder.x = vertex_position.y;

    gl_Position = projection * view * model * bone_transform * cylinder;
}
)zzz"
