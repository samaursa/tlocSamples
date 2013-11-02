#version 100

// We need one out (used to be g_FragColor)
varying lowp vec2   v_texCoord;

uniform sampler2D   s_texture;
uniform int         u_blur;
uniform int         u_winResX;
uniform int         u_winResY;

void main()
{
  int texCoordX = int(v_texCoord[0] * float(u_winResX));
  int texCoordY = int(v_texCoord[1] * float(u_winResY));

  gl_FragColor = texture2D(s_texture, vec2(v_texCoord[0], v_texCoord[1]));

  int counter = 1;
  for (int x = texCoordX - u_blur; x < texCoordX + u_blur; x++)
  {
     for (int y = texCoordY - u_blur; y < texCoordY + u_blur; y++)
     {
       if (x > 0 && y > 0)
       {
         gl_FragColor = gl_FragColor + texture2D(s_texture, vec2(float(x) / float(u_winResX), float(y) / float(u_winResY) ));
         counter = counter + 1;
       }
       else
       {
         gl_FragColor = gl_FragColor + texture2D(s_texture, vec2(float(u_winResX - x - 1) / float(u_winResX), float(u_winResY - y - 1) / float(u_winResY) ));
         counter = counter + 1;
       }
     }
  }

  gl_FragColor = gl_FragColor / float(counter);
}

