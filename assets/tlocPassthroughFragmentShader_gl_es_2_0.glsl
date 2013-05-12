#version 100

// We need one out (used to be g_FragColor)
varying lowp vec3 v_color;

void main()
{
	gl_FragColor = vec4(v_color, 0);
}
