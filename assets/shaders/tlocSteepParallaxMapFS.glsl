#version 330 core

// We need one out (used to be g_FragColor)
in  vec2          v_texCoord;
in  vec3          v_lightDir;
in  vec3          v_viewDir;
out vec4          o_color;

uniform sampler2D s_texture;
uniform sampler2D s_normTexture;
uniform sampler2D s_dispTexture;
uniform vec3      u_pScaleBias;
uniform float     u_numSamples;

vec3 GetNormalFromTexture(in sampler2D a_normTexture, vec2 a_texCoord)
{
  vec3 vertNorm = texture2D(a_normTexture, a_texCoord).xyz;
  vertNorm = (vertNorm * 2.0) - 1.0;
  vertNorm = normalize(vertNorm);

  return vertNorm;
}

void main()
{
  vec2 texCoord = vec2(v_texCoord[0], v_texCoord[1]) * 2.0;
  vec2 newTexCoord = texCoord;

  if (u_pScaleBias[2] > 0.5)
  {
    vec3 viewDir = normalize(v_viewDir);
    float angleFactor = dot(viewDir, vec3(0, 0, 1));

    float n = u_numSamples * (1.0 / angleFactor);

    float step;
    // prevent divide by 0
    if (n >= 1.0) { step = 1.0 / n; }
    else { step = 1.0; }

    float bumpScale = u_pScaleBias;
  
    vec2 dt = viewDir.xy * bumpScale / (n * viewDir.z);

    float height = 1.0;
    vec2 t = texCoord;
    float disp = texture2D(s_dispTexture, t);
    disp = (1.0 - disp) + u_pScaleBias[1];

    while (disp < height)
    {
      height -= step; t += dt;
      disp = texture2D(s_dispTexture, t);
      disp = (1.0 - disp) + u_pScaleBias[1];
    }

    //disp = (disp * u_pScaleBias[0]) + u_pScaleBias[1];
    //newTexCoord = t + (disp * viewDir.xy);
    newTexCoord = t;
  }

	o_color = texture2D(s_texture, newTexCoord);

  vec3 vertNorm   = GetNormalFromTexture(s_normTexture, newTexCoord);
  vec3 lightDir   = normalize(v_lightDir);
  float diffMult  = dot(lightDir, vertNorm);

  if (diffMult > 0.0)
  { o_color = diffMult * o_color; }
  else
  { o_color = vec4(0.0, 0.0, 0.0, 1.0); }
}
