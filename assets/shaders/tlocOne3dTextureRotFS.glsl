#version 330 core

// We need one out (used to be g_FragColor)
in  vec3 v_texCoord;
out vec4 o_color;
uniform sampler3D s_texture;

void main()
{
	o_color = texture(s_texture, v_texCoord);
  if (o_color.a < 0.5) { discard; }
}

