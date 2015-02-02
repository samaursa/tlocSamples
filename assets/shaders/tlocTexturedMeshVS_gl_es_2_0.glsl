#version 100

// Input vertex data, different for all executions of this shader.
attribute mediump vec3 a_vertPos;
attribute lowp vec3 a_vertNorm;
attribute lowp vec2 a_vertTexCoord0;
uniform lowp mat4 u_model;
uniform lowp mat3 u_normal;

uniform lowp mat4 u_vp;
uniform lowp mat4 u_view;
uniform lowp vec3 u_lightDir;

varying lowp vec2 v_texCoord;
varying lowp vec3 v_norm;
varying lowp vec3 v_lightDir;

void main()
{ 
  gl_Position = u_vp * u_model * vec4(a_vertPos, 1);

  v_texCoord = a_vertTexCoord0;

  // We need the light to be stationary. We can achieve
  // this by either not transforming the normals or 
  // transforming the light as well as the normals. We
  // will transform both just to be 'correct'
  v_norm = a_vertNorm;
  v_norm = u_normal * v_norm;
 
  v_lightDir = normalize(u_lightDir);
  v_lightDir = mat3(u_view) * v_lightDir;
}
