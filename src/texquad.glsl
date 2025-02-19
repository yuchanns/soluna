@vs vs
layout(binding=0) uniform vs_params {
	vec2 framesize;
};

struct sr_mat {
	vec2 line1;
	vec2 line2;
};

layout(binding=0) readonly buffer sr_lut {
	sr_mat sr[];
};

in vec2 position;
in vec2 offset;
in vec4 texcoord;

out vec2 uv;

void main() {
	int index = int(texcoord.z * 65535.0f);
	vec2 t = offset * 32767.0f;
	float dx = dot(t, sr[index].line1);
	float dy = dot(t, sr[index].line2);
	vec2 pos = (vec2(dx, dy) + position) * framesize;
	gl_Position = vec4(pos.x - 1.0f, pos.y + 1.0f, 0, 1);
	uv = vec2(texcoord.x, texcoord.y);
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