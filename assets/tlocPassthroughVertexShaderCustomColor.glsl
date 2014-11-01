#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vPos;
in vec4 a_color;
uniform mat4 u_mvp;

out vec3 v_color;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  v_color = a_color.rgb;
}
