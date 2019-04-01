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
    vec4 wrapped = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	wrapped.y = cos(2 * kPi * vertex_position.x) * 0.25;
	wrapped.z = sin(2 * kPi * vertex_position.x) * 0.25;
	wrapped.x = vertex_position.y;

    //gl_Position = projection * view * model * wrapped;
    gl_Position = projection * view * model * bone_transform * wrapped;
    //gl_Position = projection * view * model * vertex_position;
}
)zzz"
