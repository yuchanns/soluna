@vs vs
layout(binding=0) uniform vs_params {
	float texsize;
	int baseinst;
	vec2 framesize;
};

struct sr_mat {
	mat2 m;
};

layout(binding=0) readonly buffer sr_lut {
	sr_mat sr[];
};

struct sprite {
	uint offset;
	uint u;
	uint v;
};

layout(binding=1) readonly buffer sprite_buffer {
	sprite spr[];
};

in vec3 position;

out vec2 uv;

void main() {
	sprite s = spr[gl_InstanceIndex+baseinst];
	ivec2 uv_base = ivec2(s.u >> 16, s.v >> 16);
	ivec2 u2 = ivec2(0 , s.u & 0xffff);
	ivec2 v2 = ivec2(0 , s.v & 0xffff);
	ivec2 off = ivec2(s.offset >> 16 , s.offset & 0xffff) - 0x8000;
	vec2 uv_offset = vec2(u2[gl_VertexIndex & 1] , v2[gl_VertexIndex >> 1]);
	vec2 pos = ((uv_offset - off) * sr[int(position.z)].m + position.xy) * framesize;
	gl_Position = vec4(pos.x - 1.0f, pos.y + 1.0f, 0, 1);
	uv = (uv_base + uv_offset) * texsize;
}

@end

@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;
out vec4 frag_color;

void main() {
	frag_color = texture(sampler2D(tex,smp), uv);
}
@end

@program texquad vs fs