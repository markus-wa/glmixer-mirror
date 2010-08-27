#version 130

in highp vec2 texCoord;
out highp vec2 texc;
in vec2 maskCoord;
out vec2 maskc;
in vec4 gl_Color;
out vec4 baseColor;
   
void main(void)
{
    texc = texCoord;
    maskc = maskCoord;
    baseColor = gl_Color;

    gl_Position = ftransform();
};