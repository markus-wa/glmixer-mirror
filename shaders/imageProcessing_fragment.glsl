#version 130

#define KERNEL_SIZE 9

#define KERNEL_BLUR_GAUSSIAN 0.0625, 0.125, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625
#define KERNEL_BLUR_MEAN 0.111111,0.111111,0.111111,0.111111,0.111111,0.111111,0.111111,0.111111,0.111111
#define KERNEL_BLUR_MORE

#define KERNEL_SHARPEN 0.0, -1.0, 0.0, -1.0, 5.0, -1.0, 0.0, -1.0, 0.0
#define KERNEL_SHARPEN_MORE -1.0, -1.0, -1.0, -1.0, 9.0, -1.0, -1.0, -1.0, -1.0

#define KERNEL_EDGE_GAUSSIAN -0.0943852, -0.155615, -0.0943852, -0.155615, 1.0, -0.155615, -0.0943852, -0.155615, -0.0943852
#define KERNEL_EDGE_LAPLACE 0.0, -1.0, 0.0, -1.0, 4.0, -1.0, 0.0, -1.0, 0.0
#define KERNEL_EDGE_LAPLACE_2 -2.0, 1.0, -2.0, 1.0, 4.0, 1.0, -2.0, 1.0, -2.0

#define KERNEL_EMBOSS -2.0, -1.0, 0.0, -1.0, 1.0, 1.0, 0.0, 1.0, 2.0
#define KERNEL_EMBOSS_EDGE 5.0, -3.0, -3.0, 5.0, 0.0, -3.0, 5.0, -3.0, -3.0

in vec4 texc;
in vec4 maskc;
in vec4 baseColor;

uniform sampler2D sourceTexture;
uniform sampler2D maskTexture;
uniform sampler2D utilityTexture;
uniform bool sourceDrawing;
uniform float contrast;
uniform float saturation;
uniform float brightness;
uniform float gamma;
uniform vec4 levels;
uniform float hueshift;
uniform vec3 chromakey;
uniform float threshold;
uniform int nbColors;
uniform int invertMode; // 0=normal, 1=invert RGB, 2=invert V
uniform int filter;

out vec4 gl_FragColor;

vec3 dilation(in int N)
{
    int i = 0, j=0;
    vec3 sample[9];
    vec3 maxValue = vec3(0.0);
    vec3 tmp = vec3(0.0);
    float step_w = 1.0 / float(textureSize(sourceTexture,0).x);
    float step_h = 1.0 / float(textureSize(sourceTexture,0).y);

    for (i = -(N-1)/2; i< (N+1)/2 ; i++)
        for (j = -(N-1)/2; j< (N+1)/2; j++) {
            tmp = texture2D(sourceTexture, vec2 ( texc.s + float(i)*step_w, texc.t + float(j)*step_h ) ).rgb;
            maxValue = max(tmp, maxValue);
        }

    return maxValue;
}

vec3 erosion(in int N)
{
    int i = 0, j=0;
    vec3 sample[9];
    vec3 minValue = vec3(1.0);
    vec3 tmp = vec3(0.0);
    float step_w = 1.0 / float(textureSize(sourceTexture,0).x);
    float step_h = 1.0 / float(textureSize(sourceTexture,0).y);

    for (i = -(N-1)/2; i< (N+1)/2 ; i++)
        for (j = -(N-1)/2; j< (N+1)/2; j++) {
            tmp = texture2D(sourceTexture, vec2 ( texc.s + float(i)*step_w, texc.t + float(j)*step_h ) ).rgb;
            minValue = min(tmp, minValue);
        }

    return minValue;
}

vec3 convolution()
{
    int i = 0, j=0;
    vec3 sum = vec3(0.0);
    vec3 tmp = vec3(0.0);
    float step_w = 1.0 / float(textureSize(sourceTexture,0).x);
    float step_h = 1.0 / float(textureSize(sourceTexture,0).y);

    float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](KERNEL_BLUR_GAUSSIAN);

    if (filter==2)
        kernel = float[KERNEL_SIZE](KERNEL_BLUR_MEAN);
    else if (filter==3)
        kernel = float[KERNEL_SIZE](KERNEL_SHARPEN);
    else if (filter==4)
        kernel = float[KERNEL_SIZE](KERNEL_SHARPEN_MORE);
    else if (filter==5)
        kernel = float[KERNEL_SIZE](KERNEL_EDGE_GAUSSIAN);
    else if (filter==6)
        kernel = float[KERNEL_SIZE](KERNEL_EDGE_LAPLACE);
    else if (filter==7)
        kernel = float[KERNEL_SIZE](KERNEL_EDGE_LAPLACE_2);
    else if (filter==8)
        kernel = float[KERNEL_SIZE](KERNEL_EMBOSS);
    else if (filter==9)
        kernel = float[KERNEL_SIZE](KERNEL_EMBOSS_EDGE);

    for (i = 0; i<3; i++)
        for (j = 0; j<3; j++) {
            tmp = texture2D(texture, vec2 ( texc.s + float(i-1)*step_w, texc.t + float(j-1)*step_h ) ).rgb;
            sum += tmp * kernel[3*i + j];
        }

    return sum;
}


vec3 apply_filter() {

    if (filter == 0)
        return texture2D(sourceTexture, texc.st).rgb;
    else if (filter < 10)
        return convolution();
    else if (filter==10)
        return erosion(3);
    else if (filter==11)
        return erosion(7);
    else if (filter==12)
        return erosion(13);
    else if (filter==13)
        return dilation(3);
    else if (filter==14)
        return dilation(7);
    else if (filter==15)
        return dilation(13);

}

/*
** Hue, saturation, luminance
*/

vec3 RGBToHSL(in vec3 color)
{
    vec3 hsl = vec3(0.0, 0.0, 0.0); // init to 0 to avoid warnings ? (and reverse if + remove first part)

    float fmin = min(min(color.r, color.g), color.b);    //Min. value of RGB
    float fmax = max(max(color.r, color.g), color.b);    //Max. value of RGB
    float delta = fmax - fmin;             //Delta RGB value

    hsl.z = (fmax + fmin) / 2.0; // Luminance

    if (delta == 0.0)       //This is a gray, no chroma...
    {
        hsl.x = 0.0;    // Hue
        hsl.y = 0.0;    // Saturation
    }
    else                    //Chromatic data...
    {
        if (hsl.z < 0.5)
            hsl.y = delta / (fmax + fmin); // Saturation
        else
            hsl.y = delta / (2.0 - fmax - fmin); // Saturation

        float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;
        float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;
        float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;

        if (color.r == fmax )
            hsl.x = deltaB - deltaG; // Hue
        else if (color.g == fmax)
            hsl.x = (1.0 / 3.0) + deltaR - deltaB; // Hue
        else if (color.b == fmax)
            hsl.x = (2.0 / 3.0) + deltaG - deltaR; // Hue

        if (hsl.x < 0.0)
            hsl.x += 1.0; // Hue
        else if (hsl.x > 1.0)
            hsl.x -= 1.0; // Hue
    }

    return hsl;
}

float HueToRGB(in float f1, in float f2, in float hue)
{
    if (hue < 0.0)
        hue += 1.0;
    else if (hue > 1.0)
        hue -= 1.0;
    float res;
    if ((6.0 * hue) < 1.0)
        res = f1 + (f2 - f1) * 6.0 * hue;
    else if ((2.0 * hue) < 1.0)
        res = f2;
    else if ((3.0 * hue) < 2.0)
        res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
    else
        res = f1;
    return res;
}

vec3 HSLToRGB(in vec3 hsl)
{
    vec3 rgb;

    if (hsl.y == 0.0)
        rgb = vec3(hsl.z); // Luminance
    else
    {
        float f1, f2;

        if (hsl.z < 0.5)
            f2 = hsl.z * (1.0 + hsl.y);
        else
            f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);

        f1 = 2.0 * hsl.z - f2;

        rgb.r = HueToRGB(f1, f2, hsl.x + (1.0/3.0));
        rgb.g = HueToRGB(f1, f2, hsl.x);
        rgb.b = HueToRGB(f1, f2, hsl.x - (1.0/3.0));
    }

    return rgb;
}


/*
** Gamma correction
** Details: http://blog.mouaif.org/2009/01/22/photoshop-gamma-correction-shader/
*/

#define GammaCorrection(color, gamma) pow(color, 1.0 / gamma)

/*
** Levels control (input (+gamma), output)
** Details: http://blog.mouaif.org/2009/01/28/levels-control-shader/
*/

#define LevelsControlInputRange(color, minInput, maxInput)  min(max(color - vec3(minInput), vec3(0.0)) / (vec3(maxInput) - vec3(minInput)), vec3(1.0))
#define LevelsControlInput(color, minInput, gamma, maxInput) GammaCorrection(LevelsControlInputRange(color, minInput, maxInput), gamma)
#define LevelsControlOutputRange(color, minOutput, maxOutput)  mix(vec3(minOutput), vec3(maxOutput), color)
#define LevelsControl(color, minInput, gamma, maxInput, minOutput, maxOutput)   LevelsControlOutputRange(LevelsControlInput(color, minInput, gamma, maxInput), minOutput, maxOutput)


void main(void)
{

	if (!sourceDrawing) {
		gl_FragColor = texture2D(utilityTexture, maskc.st) + baseColor;
		return;
	}

    // get filtered value of texel
    vec3 texel = apply_filter();
    // deal with alpha separately
    //float alpha = texture2D(maskTexture, maskc.st).a * texture2D(sourceTexture, texc.st).a  * baseColor.a;
    float alpha = texture2D(maskTexture, maskc.st).a * baseColor.a;

    // operations on RGB ; brightness, contrast and levels
    vec3 transformedRGB = mix(vec3(0.62), texel.rgb, contrast);
    transformedRGB += brightness;
    // also clamp values
    //transformedRGB = LevelsControl(transformedRGB, levels.x, gamma, levels.y, levels.z, levels.w);

    if (invertMode==1)
       transformedRGB = vec3(1.0) - transformedRGB;

    // get HSL to perform operations on Hue Saturation and Luminance
    vec3 transformedHSL = RGBToHSL( transformedRGB );

    // Operations on HSL ; if threshold applied, others are not useful
    if(threshold > 0.0) {
        // level threshold
        transformedHSL = vec3(0.0, 0.0, step(transformedHSL.z, threshold) );
    } else {
        // perform hue shift
        transformedHSL.x = mod( transformedHSL.x + hueshift, 1.0);

        // Saturation
        transformedHSL.y *= saturation;

        // perform reduction of colors
        if (nbColors > 0) {
            transformedHSL *= nbColors;
            transformedHSL = roundEven(transformedHSL);
            transformedHSL /= nbColors;
        }

        if (invertMode == 2)
            transformedHSL.z = 1.0 - transformedHSL.z;

        if ( chromakey.z > 0.0 && all( lessThan(transformedHSL - chromakey, vec3(0.1))) )
            alpha *= 0.0;

    }

    // after operations on HSL, convert back to RGB
    transformedRGB = HSLToRGB(transformedHSL) * baseColor.rgb;

    // bring back the original alpha for final fragment color
    gl_FragColor = vec4(transformedRGB, alpha );

}


