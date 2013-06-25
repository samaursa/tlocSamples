#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 a_vPos;
in vec3 a_vNorm;
in vec2 a_tCoord;
uniform mat4 u_mvp;

out vec2 v_texCoord;
out vec3 v_norm;
out vec3 v_lightDir;

void main()
{ 
  gl_Position = u_mvp * vec4(a_vPos, 1);

  v_texCoord = a_tCoord;

  // need this matrix for later calculations
  mat3 mvpRot;
  mvpRot[0] = u_mvp[0].xyz;
  mvpRot[1] = u_mvp[1].xyz;
  mvpRot[2] = u_mvp[2].xyz;

  // We need the light to be stationary. We can achieve
  // this by either not transforming the normals or 
  // transforming the light as well as the normals. We
  // will transform both just to be 'correct'
  v_norm = a_vNorm;

  mat3 normalMat = mvpRot;
  normalMat = inverse(normalMat);
  normalMat = transpose(normalMat);
  v_norm.xyz = normalMat * v_norm;
  v_norm = normalize(v_norm);

  v_lightDir = vec3(0.2f, 0.5f, 1.0f);
  v_lightDir = normalize(v_lightDir);

  v_lightDir = mvpRot * v_lightDir;
}
