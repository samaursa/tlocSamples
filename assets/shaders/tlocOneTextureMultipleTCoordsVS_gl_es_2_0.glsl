#version 100

// Input vertex data, different for all executions of this shader.
attribute lowp vec3 a_vPos;
attribute lowp vec2 a_tCoord;
attribute lowp vec2 a_tCoord2;
uniform mat4 u_mvp;

varying vec2 v_texCoord;
varying vec2 v_texCoord2;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  v_texCoord = a_tCoord;
  v_texCoord2 = a_tCoord2;
}
