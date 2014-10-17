#version 100

// Input vertex data, different for all executions of this shader.
attribute mediump vec3 a_vPos;
attribute lowp vec3 a_vNorm;
attribute lowp vec2 a_tCoord;
uniform mat4 u_mvp;
uniform mat4 u_modelMat;
uniform mat4 u_lightMVP;
uniform vec3 u_lightDir;

varying lowp vec2 v_texCoord;
varying lowp vec3 v_norm;
varying lowp vec3 v_lightDir;
varying mediump vec4 v_shadowCoord;

void main()
{
  gl_Position.xyz = a_vPos;
  gl_Position.w = 1.0;
  gl_Position = u_mvp * gl_Position;

  v_shadowCoord = u_lightMVP * u_modelMat * vec4(a_vPos, 1);

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

  v_norm.xyz = mvpRot * v_norm;
  v_norm = normalize(v_norm);

  v_lightDir = u_lightDir;
  v_lightDir = normalize(v_lightDir);
}
