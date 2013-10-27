#version 330 core

// We need one out (used to be g_FragColor)
in  vec2            v_texCoord;
in  vec2            v_texCoord2;
out vec4            o_color;
uniform sampler2D   s_texture;
uniform vec4        u_blockColor;
uniform float       u_whiteMultiplier;
uniform float       u_alpha;

void main()
{
  o_color = texture2D(s_texture, vec2(v_texCoord[0], 1.0 - v_texCoord[1]));

  vec4 tex2Col = texture2D(s_texture, vec2(v_texCoord2[0], 1.0 - v_texCoord2[1]));
 
  if (tex2Col.r > 0.0 && 
      tex2Col.g > 0.0 &&
      tex2Col.b > 0.0) 
  {
    vec4 currCol = o_color;

    if (currCol[0] > 0.5)
    { currCol[0] = 1.0 - 2.0 * (1.0 - currCol[0]) * (1.0 - u_blockColor[0]); }
    else
    { currCol[0] = 2.0 * currCol[0] * u_blockColor[0]; }

    if (currCol[1] > 0.5)
    { currCol[1] = 1.0 - 2.0 * (1.0 - currCol[1]) * (1.0 - u_blockColor[1]); }
    else
    { currCol[1] = 2.0 * currCol[1] * u_blockColor[1]; }

    if (currCol[2] > 0.5)
    { currCol[2] = 1.0 - 2.0 * (1.0 - currCol[2]) * (1.0 - u_blockColor[2]); }
    else
    { currCol[2] = 2.0 * currCol[2] * u_blockColor[2]; }

    currCol = clamp(currCol, 0.0, 1.0);

    o_color = (o_color * (vec4(1.0, 1.0, 1.0, 1.0) - tex2Col)) + (currCol * tex2Col);
  }
}

