#version 100

// We need one out (used to be g_FragColor)
varying lowp vec2 v_texCoord;
varying lowp vec3 v_norm;
varying lowp vec3 v_lightDir;

uniform sampler2D s_texture;

void main()
{
  lowp float multiplier = dot(v_lightDir, v_norm);
  multiplier = clamp(multiplier, 0.1, 1.0);
  gl_FragColor.rgb = texture2D(s_texture, vec2(v_texCoord[0], 1.0 - v_texCoord[1])).rgb;
  gl_FragColor.rgb = gl_FragColor.rgb * multiplier;
}
