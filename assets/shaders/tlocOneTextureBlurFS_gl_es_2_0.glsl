#version 100

// We need one out (used to be g_FragColor)
varying lowp vec2   v_texCoord;

uniform sampler2D   s_texture;
uniform lowp float  u_blur;

void main()
{
  lowp float texCoordX = v_texCoord[0] * 500;
  lowp float texCoordY = v_texCoord[1] * 500;

  o_color = texture2D(s_texture, vec2(v_texCoord[0], v_texCoord[1]));

  lowp float counter = 1;
  for (lowp float x = texCoordX - u_blur; x < texCoordX + u_blur; x++)
  {
     for (float y = texCoordY - u_blur; y < texCoordY + u_blur; y++)
     {
       o_color = o_color + texture2D(s_texture, vec2(x / 500, y / 500));
       counter = counter + 1;
     }
  }

  o_color = o_color / counter;
}

