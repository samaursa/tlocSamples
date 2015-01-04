#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vertPos;
in vec2 a_vertTexCoord0;
in mat4 a_model;

uniform mat4 u_vp;

out vec2 texCoord;

void main()
{ 
  gl_Position = u_vp * a_model * vec4(a_vertPos, 1);
  
  texCoord = a_vertTexCoord0;
}

