#version 100

// We need one out (used to be g_FragColor)
varying lowp vec2 v_texCoord;
varying lowp vec3 v_norm;
varying lowp vec3 v_lightDir;
varying mediump vec4 v_shadowCoord;

uniform sampler2D s_texture;
uniform sampler2D s_shadowMap;

void main()
{
  lowp vec3 lightColor = vec3(1, 1, 1);

  mediump float multiplier = dot(v_lightDir, v_norm);
  multiplier = clamp(multiplier, 0.1, 1.0);
  gl_FragColor.rgb = texture2D(s_texture, vec2(v_texCoord[0], 1.0 - v_texCoord[1])).rgb;
  gl_FragColor = gl_FragColor * multiplier;

  mediump vec3 positionLtNDC = v_shadowCoord.xyz / v_shadowCoord.w;
  mediump vec3 positionLtScreen = positionLtNDC;

  mediump vec2 shadowTexCoord = positionLtScreen.xy;
  mediump float fragmentDepth = positionLtScreen.z;

  mediump float shadowDepth = texture2D(s_shadowMap, shadowTexCoord).r;
  
  mediump float vis = 1.0;
  if( (fragmentDepth - shadowDepth) > - 0.0005)
  { vis = 0.2; }

  gl_FragColor.rgb = gl_FragColor.rgb * vis;

}
