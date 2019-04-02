R"zzz(
#version 330 core
uniform vec4 light_position;
uniform vec3 camera_position;

uniform vec3 joint_trans[128];
uniform vec4 joint_rot[128];

in int jid0;
in int jid1;
in float w0;
in vec3 vector_from_joint0;
in vec3 vector_from_joint1;
in vec4 normal;
in vec2 uv;
in vec4 vert;

out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_camera_direction;

vec3 qtransform(vec4 q, vec3 v) {
	return v + 2.0 * cross(cross(v, q.xyz) - q.w*v, q.xyz);
}

vec4 construct(vec4 q, vec3 v) {
	vec4 dual = vec4(1.0);
	dual[0] = -0.5 * (v[0]*q.x  + v[1]*q.y + v[2]*q.z);
	dual[1] =  0.5 * (v[0]*q.w  + v[1]*q.z - v[2]*q.y);
	dual[2] =  0.5 * (-v[0]*q.z + v[1]*q.w + v[2]*q.x);
	dual[3] =  0.5 * (v[0]*q.y  - v[1]*q.x + v[2]*q.w);
	return dual;
}

vec3 trans(vec4 r, vec4 i){
	vec3 trans = vec3(1.0);
	trans[0] = 2.0*(-i[0]*r.x + i[1]*r.w - i[2]*r.z + i[3]*r.y);
	trans[1] = 2.0*(-i[0]*r.y + i[1]*r.z + i[2]*r.w - i[3]*r.x);
	trans[2] = 2.0*(-i[0]*r.z - i[1]*r.y + i[2]*r.x + i[3]*r.w);
	return trans;
}


void main() {
	// FIXME: Implement linear skinning here
	
	vec4 r_0 = joint_rot[jid0];
	vec4 r_1 = joint_rot[jid1];


	vec3 trans_joint_0 =  joint_trans[jid0] - qtransform(r_0, vert.xyz - vector_from_joint0);
	vec3 trans_joint_1 =  joint_trans[jid1] - qtransform(r_1, vert.xyz - vector_from_joint1);

	vec4 i_0 = construct(r_0, trans_joint_0);
	vec4 i_1 = construct(r_1, trans_joint_1);

	vec4 r = w0 * r_0 + (1-w0) * r_1;
	vec4 i = w0 * i_0 + (1-w0) * i_1;

	float length = length(r);
	r /= length;
	i /= length;

	vec3 new_trans = trans(r, i);



	gl_Position = vec4(qtransform(r, vert.xyz) + new_trans, 1.0);

	
	vs_normal = normal;
	vs_light_direction = light_position - gl_Position;
	vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_uv = uv;
}
)zzz"
