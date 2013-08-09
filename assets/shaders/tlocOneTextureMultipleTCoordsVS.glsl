#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vPos;
in vec2 a_tCoord;
in vec2 a_tCoord2;
uniform mat4 u_mvp;

out vec2 v_texCoord;
out vec2 v_texCoord2;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);
  v_texCoord = a_tCoord;
  v_texCoord2 = a_tCoord2;
}
