#version 330 core

// Input vertex data, different for all executions of this shader.
in vec2 a_vertPos;
in vec2 a_vertTexCoord0;

uniform mat4 u_mvp;

out vec2 texCoord;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vertPos, 0, 1);
  
  texCoord = a_vertTexCoord0;
}

