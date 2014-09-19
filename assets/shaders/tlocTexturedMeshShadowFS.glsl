#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
in  vec3 v_norm;
in  vec3 v_lightDir;
in  vec4 v_shadowCoord;
out vec3 o_color;

uniform sampler2D s_texture;
uniform sampler2D s_shadowMap;

void main()
{
  vec3 lightColor = vec3(1, 1, 1);

  float multiplier = dot(v_lightDir, v_norm);
  multiplier = clamp(multiplier, 0.1, 1.0);
	o_color = texture2D(s_texture, vec2(v_texCoord[0], 1.0 - v_texCoord[1])).rgb;
  o_color = o_color * multiplier;

  vec3 positionLtNDC = v_shadowCoord.xyz / v_shadowCoord.w;
  vec3 positionLtScreen = positionLtNDC;

  vec2 shadowTexCoord = positionLtScreen.xy;
  float fragmentDepth = positionLtScreen.z;

  float shadowDepth = texture(s_shadowMap, shadowTexCoord).r;
  
  float vis = 1.0;
  if( (fragmentDepth - shadowDepth) > - 0.00001)
  { vis = 0.2; }

  o_color = o_color * vis;
}
