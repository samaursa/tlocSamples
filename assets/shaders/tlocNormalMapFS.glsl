#version 330 core

// We need one out (used to be g_FragColor)
in  vec2          v_texCoord;
in  vec3          v_lightDir;
out vec4          o_color;

uniform sampler2D s_texture;
uniform sampler2D s_normTexture;

void main()
{
	o_color = texture2D(s_texture, vec2(v_texCoord[0], 1.0 - v_texCoord[1]));
  vec3 vertNorm = texture2D(s_normTexture, vec2(v_texCoord[0], 1.0 - v_texCoord[1])).xyz;

  vertNorm = (vertNorm * 2.0) - 1.0;
  vertNorm = normalize(vertNorm);
  vec3 lightDir = normalize(v_lightDir);

  float diffMult = dot(lightDir, vertNorm);

  if (diffMult > 0.0)
  { o_color = diffMult * o_color; }
  else
  { o_color = vec4(0.0, 0.0, 0.0, 1.0); }
  
}
