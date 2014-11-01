#version 100

// Input vertex data, different for all executions of this shader.
attribute lowp vec3 a_vPos;
attribute lowp vec4 a_color;
uniform mat4 u_mvp;

varying lowp vec3 v_color;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  v_color = a_color.rgb;
}
