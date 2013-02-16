#version 100

// Input vertex data, different for all executions of this shader.
attribute vec3 a_vPos;
attribute vec2 a_tCoord;
uniform mat4 u_mvp;

varying lowp vec2 texCoord;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  texCoord = a_tCoord;
}
