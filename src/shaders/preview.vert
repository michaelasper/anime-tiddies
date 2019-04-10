R"zzz(#version 330 core
in vec4 vertex_position;
in vec2 tex_coord_in;
uniform mat4 orthomat;
uniform float frame_shift;
out vec2 tex_coord;
void main()
{
	tex_coord = tex_coord_in;
	vec4 pos = vertex_position;
	pos.y += frame_shift;
	gl_Position = orthomat * pos;
}
)zzz"
