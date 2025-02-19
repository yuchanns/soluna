@vs vs
layout(binding=0) uniform vs_params {
	vec2 framesize;
};

struct sr_mat {
	mat2 m;
};

layout(binding=0) readonly buffer sr_lut {
	sr_mat sr[];
};

struct sprite {
	vec2 offset;
	vec2 texcoord;
};

layout(binding=1) readonly buffer sprite_buffer {
	sprite spr[];
};

in vec3 position;

out vec2 uv;

void main() {
	int sr_index = int(position.z);
	int index = gl_InstanceIndex * 4 + gl_VertexIndex;
	vec2 pos = (spr[index].offset * sr[sr_index].m + vec2(position.x, position.y)) * framesize;
	gl_Position = vec4(pos.x - 1.0f, pos.y + 1.0f, 0, 1);
	uv = spr[index].texcoord;
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