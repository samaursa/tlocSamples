#version 100

// Input vertex data, different for all executions of this shader.
attribute lowp vec3 a_vertPos;
attribute lowp vec2 a_tCoord;
uniform mat4 u_mvp;

varying vec2 v_texCoord;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vertPos, 1);
  v_texCoord = a_tCoord;
}
