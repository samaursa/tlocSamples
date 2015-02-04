#version 100

// Input vertex data, different for all executions of this shader.
attribute mediump vec3  a_vertPos;
attribute lowp float    a_index;
attribute lowp mat4     a_color;

uniform mat4 u_vp;
uniform mat4 u_model;

varying lowp vec3 v_color;

void main()
{ 
  gl_Position = u_vp * u_model * vec4(a_vertPos, 1);
  v_color = a_color[int(a_index)].rgb;
}
