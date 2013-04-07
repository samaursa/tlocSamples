#version 100

// Ouput data
varying lowp vec2 texCoord;
uniform lowp mat4 u_mvp;
uniform sampler2D shaderTexture;

void main()
{
	gl_FragColor.rgb = texture2D(shaderTexture, texCoord).rgb;
}
