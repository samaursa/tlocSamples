#version 100

// Input vertex data, different for all executions of this shader.
attribute lowp vec3 a_vPos;
attribute lowp vec2 a_tCoord;
uniform lowp mat4 u_mvp;

varying lowp vec2 texCoord;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  
  texCoord = a_tCoord;
}

