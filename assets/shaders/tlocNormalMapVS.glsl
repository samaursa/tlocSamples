#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vertPos;
in vec2 a_vertTexCoord0;

uniform mat4 u_vp;
uniform mat4 u_model;
uniform vec3 u_lightPos;

out vec2 v_texCoord;
out vec3 v_lightDir; // in tangent space

void main()
{ 
  // quad's TBN matrix is always the same
  mat3 tbn = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);

  vec4 vertPosWorld = u_model * vec4(a_vertPos, 1);

  gl_Position = u_vp * vertPosWorld;
  v_texCoord = a_vertTexCoord0;

  v_lightDir = u_lightPos - vertPosWorld.xyz;
  v_lightDir = inverse(tbn) * v_lightDir; // no effect because tbn is identity

  v_lightDir = normalize(v_lightDir);
}
