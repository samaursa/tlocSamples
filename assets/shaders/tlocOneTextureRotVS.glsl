#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vertPos;
in vec2 a_vertTexCoord0;

uniform mat4  u_vp;
uniform mat4  u_model;
uniform mat3  u_rot;
uniform float u_slice;

out vec3 v_texCoord;

void main()
{ 
  gl_Position = u_vp * u_model * vec4(a_vertPos, 1);
  
  vec3 texCoord3 = vec3(a_vertTexCoord0, u_slice);
  texCoord3 = (texCoord3 - 0.5f) * 2;
  texCoord3[2] = texCoord3[2] * 0.1f;

  vec3 tRot = u_rot * texCoord3;
  //tRot[0] = (1.0/tRot[0]) * texCoord3[0];

  tRot = (tRot / 2) + 0.5f;

  v_texCoord = tRot;
}
