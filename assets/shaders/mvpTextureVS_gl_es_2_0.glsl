#version 100

// Input vertex data, different for all executions of this shader.
attribute lowp vec3 a_vertPos;
attribute lowp vec2 a_vertTexCoord0;
uniform lowp mat4 u_mvp;

varying lowp vec2 texCoord;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vertPos, 1);
  
  texCoord = a_vertTexCoord0;
}

