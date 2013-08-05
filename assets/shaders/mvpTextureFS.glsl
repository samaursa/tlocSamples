#version 330 core

// Ouput data
in  vec2 texCoord;
out vec3 color;
uniform mat4 u_mvp;
uniform sampler2D shaderTexture;

void main()
{
	color = texture2D(shaderTexture, vec2(texCoord[0], 1.0 - texCoord[1])).rgb;
}
