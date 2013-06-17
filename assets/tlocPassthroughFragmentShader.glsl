#version 330 core

// We need one out (used to be g_FragColor)
in  vec3 v_color;
out vec3 color;

void main()
{
	color = v_color;
}
