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

  mediump float diffMult = dot(v_lightDir, v_norm);
  diffMult = clamp(diffMult, 0.1, 1.0);

  lowp vec3 pixelColor = texture2D(s_texture, vec2(v_texCoord[0], 1.0 - v_texCoord[1])).rgb;

  mediump vec3 positionLtNDC = v_shadowCoord.xyz / v_shadowCoord.w;
  mediump vec3 positionLtScreen = positionLtNDC;

  mediump vec2 shadowTexCoord = positionLtScreen.xy;
  mediump float fragmentDepth = positionLtScreen.z;

  mediump float shadowDepth = texture2D(s_shadowMap, shadowTexCoord).r;
  
  mediump float shadowMult = 1.0;
  if( (fragmentDepth - shadowDepth) > - 0.0005)
  { shadowMult = 0.2; }

  lowp float ambient = 0.1;

  gl_FragColor.rgb = pixelColor * (ambient + shadowMult + diffMult);
}
