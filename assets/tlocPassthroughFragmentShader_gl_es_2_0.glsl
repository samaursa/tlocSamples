#version 100

// We need one out (used to be g_FragColor)
varying lowp vec2 texCoord;

void main()
{
	gl_FragColor = vec4(texCoord.s, texCoord.t, 0, 0);
}
