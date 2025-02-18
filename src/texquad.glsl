@vs vs
layout(binding=0) uniform vs_params {
	vec2 framesize;
};

in vec2 position;
in vec2 texcoord;

out vec2 uv;

void main() {
	vec2 pos = position * framesize;
	gl_Position = vec4(pos.x - 1.0f, pos.y + 1.0f, 0, 1);
	uv = texcoord;
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