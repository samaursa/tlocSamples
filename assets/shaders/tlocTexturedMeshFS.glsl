#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
in  vec3 v_norm;
out vec3 o_color;
uniform sampler2D s_texture;

void main()
{
  vec3 lightDir = vec3(1.0f, 1.0f, 0.0f);
  lightDir = normalize(lightDir);
  float multiplier = dot(lightDir, v_norm);
  multiplier = clamp(multiplier, 0.5f, 1.0f);
	o_color = texture2D(s_texture, v_texCoord).rgb * multiplier;
}
