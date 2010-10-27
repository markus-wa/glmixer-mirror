
attribute highp vec2 texCoord;
attribute lowp vec2 maskCoord;

varying vec2 texc;
varying vec2 maskc;
varying vec4 baseColor;
   
void main(void)
{
    texc = texCoord;
    maskc = maskCoord;
    baseColor = gl_Color;

    gl_Position = ftransform();
}

