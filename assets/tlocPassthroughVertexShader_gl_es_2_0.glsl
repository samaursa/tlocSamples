#version 100

// Input vertex data, different for all executions of this shader.
attribute vec3 a_vPos;
uniform mat4 u_mvp;

varying lowp vec3 v_color;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  v_color = vec3(a_vPos[0], a_vPos[1], 0.3f);
}
