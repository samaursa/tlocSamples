#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vPos;
in vec3 a_vNorm;
in vec2 a_tCoord;
uniform mat4 u_mvp;
uniform mat4 u_viewMat;
uniform mat3 u_normalMat;
uniform vec3 u_lightDir;

out vec2 v_texCoord;
out vec3 v_norm;
out vec3 v_lightDir;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);

  v_texCoord = a_tCoord;

  // We need the light to be stationary. We can achieve
  // this by either not transforming the normals or 
  // transforming the light as well as the normals. We
  // will transform both just to be 'correct'
  v_norm    = a_vNorm;
  v_norm    = u_normalMat * v_norm;

  v_lightDir = normalize(u_lightDir);
  v_lightDir = mat3(u_viewMat) * v_lightDir;
}
