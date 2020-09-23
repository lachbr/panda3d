#version 330

uniform sampler2D scene_tex;
in vec2 l_uv;

out vec4 o_color;

void main()
{
	o_color = texture(scene_tex, l_uv);
}
