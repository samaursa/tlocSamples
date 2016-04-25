#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 v_texCoord;
in  vec3 v_lightPosOnScreen;

out vec4 o_color;

uniform sampler2D s_texture;
uniform sampler2D s_stencil;

const int NUM_SAMPLES = 100;
const float density = 0.1;
const float decay = 0.5;
const float weight = 0.9;

void main()
{
  vec2 deltaTexCoord = vec2(v_texCoord.st - v_lightPosOnScreen.xy);
  vec2 texCoo = v_texCoord.st;
  deltaTexCoord *= 1.0 / float(NUM_SAMPLES) * density;
  float illuminationDecay = 1.0;

	vec4 rays = vec4(0, 0, 0, 1);

  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    texCoo -= deltaTexCoord;
    vec4 sample = texture2D(s_stencil, texCoo);
    sample *= illuminationDecay * weight;
    rays += sample;
    illuminationDecay *= decay;
  }

  //o_color = texture2D(s_texture, v_texCoord) + s_stencil;
  o_color = texture2D(s_texture, v_texCoord) + rays;
}
