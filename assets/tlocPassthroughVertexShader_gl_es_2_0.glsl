#version 100

// Input vertex data, different for all executions of this shader.
attribute lowp vec3 a_vertPos;

uniform mat4 u_vp;
uniform mat4 u_model;

varying lowp vec3 v_color;

void main()
{ 
  gl_Position = u_vp * u_model * vec4(a_vertPos, 1);
  v_color = vec3(a_vertPos[0], a_vertPos[1], 0.3);
}
