#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vertPos;
in vec2 a_vertTexCoord0;
in mat3 a_tbn;

uniform mat4 u_vp;
uniform mat4 u_view;
uniform mat4 u_model;
uniform vec3 u_lightPos;

out vec2 v_texCoord;
out vec3 v_lightDir; // in tangent space
out vec3 v_viewDir; // in tangent space

void main()
{ 
  vec4 vertPosWorld = u_model * vec4(a_vertPos, 1);

  gl_Position = u_vp * vertPosWorld;
  v_texCoord = a_vertTexCoord0;

  mat3 worldToTan = inverse(a_tbn) * inverse(mat3(u_model));

  // put the light in tangent space from world space
  v_lightDir = u_lightPos - vertPosWorld.xyz;
  v_lightDir = worldToTan * v_lightDir;

  mat4 viewInv = inverse(u_view);
  v_viewDir  = worldToTan * ((viewInv[3]).xyz - vertPosWorld.xyz);
}
