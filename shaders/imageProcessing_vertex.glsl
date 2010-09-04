
attribute vec2 texCoord;
varying vec2 texc;
attribute vec2 maskCoord;
varying vec2 maskc;
varying vec4 baseColor;
   
void main(void)
{
    texc = texCoord;
    maskc = maskCoord;
    baseColor = gl_Color;

    gl_Position = ftransform();
}

