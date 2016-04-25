#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
in  vec3 v_norm;
in  vec3 v_lightDir;
out vec4 o_color;
uniform sampler2D s_texture;
uniform vec3 u_lightColor = vec3(1, 1, 1);

void main()
{
  float multiplier = dot(v_lightDir, v_norm);
  multiplier = clamp(multiplier, 0.1f, 1.0f);
	o_color = texture2D(s_texture, vec2(v_texCoord[0], 1.0f - v_texCoord[1]));
  o_color.rgb = o_color.rgb * multiplier * u_lightColor;
}
