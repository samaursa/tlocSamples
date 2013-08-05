#version 100

// Ouput data
varying lowp vec2 texCoord;
uniform lowp mat4 u_mvp;
uniform sampler2D shaderTexture;

void main()
{
	gl_FragColor.rgb = texture2D(shaderTexture, vec2(texCoord[0], 1.0 - texCoord[1])).rgb;
}
