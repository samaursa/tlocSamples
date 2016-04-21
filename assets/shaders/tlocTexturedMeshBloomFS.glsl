#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
in  vec3 v_norm;
in  vec3 v_lightDir;
uniform sampler2D s_texture;
uniform vec3 u_lightColor = vec3(5, 5, 5);

layout (location = 0) out vec4 o_color;
layout (location = 1) out vec4 o_bright;

void main()
{
  float multiplier = dot(v_lightDir, v_norm);
  multiplier = clamp(multiplier, 0.1f, 1.0f);
	o_color = texture2D(s_texture, vec2(v_texCoord[0], 1.0f - v_texCoord[1]));
  o_color = o_color * multiplier * vec4(u_lightColor, 1.0);

  // Bright pass
  // Where does the vec3() come from? That ensures that we get brightness
  // wrt to human eye sensitivity to each color
  float brightness = dot(o_color.rgb, vec3(0.2126, 0.7152, 0.0722));
  if (brightness > 1.0) { o_bright = o_color; }
  else { o_bright = vec4(0, 0, 0, 1); }
}
