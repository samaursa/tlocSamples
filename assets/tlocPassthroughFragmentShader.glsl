#version 330 core

// We need one out (used to be g_FragColor)
in  vec2 texCoord;
out vec3 color;

void main()
{
	color = vec3(texCoord.s, texCoord.t, 0);
}
