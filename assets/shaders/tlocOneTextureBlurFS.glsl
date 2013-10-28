#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
out vec4 o_color;
uniform sampler2D s_texture;
uniform float     u_blur;

void main()
{
  float texCoordX = v_texCoord[0] * 500;
  float texCoordY = v_texCoord[1] * 500;

  o_color = texture2D(s_texture, vec2(v_texCoord[0], v_texCoord[1]));

  float counter = 1;
  for (float x = texCoordX - u_blur; x < texCoordX + u_blur; x++)
  {
     for (float y = texCoordY - u_blur; y < texCoordY + u_blur; y++)
     {
       o_color = o_color + texture2D(s_texture, vec2(x / 500, y / 500));
       counter = counter + 1;
     }
  }

  o_color = o_color / counter;
}
