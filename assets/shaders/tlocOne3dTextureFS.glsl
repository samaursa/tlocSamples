#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
out vec4 o_color;
uniform sampler3D s_texture;
uniform float     u_slice;

void main()
{
	o_color = texture(s_texture, vec3(v_texCoord[0], 1.0 - v_texCoord[1], u_slice));
}
