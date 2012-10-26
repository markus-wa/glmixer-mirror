
#extension GL_EXT_gpu_shader4 : require

varying vec2 texc;
varying vec2 maskc;
varying vec4 baseColor;

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
uniform float chromadelta;
uniform float threshold;
uniform int nbColors;
uniform int invertMode; 


/*
** Hue, saturation, luminance
*/

vec3 RGBToHSL(vec3 color)
{
    vec3 hsl = vec3(0.0, 0.0, 0.0); // init to 0 to avoid warnings ? (and reverse if + remove first part)

    float fmin = min(min(color.r, color.g), color.b);    //Min. value of RGB
    float fmax = max(max(color.r, color.g), color.b);    //Max. value of RGB
    float delta = fmax - fmin;             //Delta RGB value

    hsl.z = (fmax + fmin) / 2.0; // Luminance

    if (delta == 0.0)       //This is a gray, no chroma...
    {
        hsl.x = -1.0;    // Hue
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

float HueToRGB(float f1, float f2, float hue)
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

vec3 HSLToRGB(vec3 hsl)
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

#define GammaCorrection(color, gamma) pow( color, 1.0 / vec3(gamma))

/*
** Levels control (input (+gamma), output)
** Details: http://blog.mouaif.org/2009/01/28/levels-control-shader/
*/


#define LevelsControlInputRange(color, minInput, maxInput)  min(max(color - vec3(minInput), 0.0) / (vec3(maxInput) - vec3(minInput)), 1.0)
#define LevelsControlInput(color, minInput, gamma, maxInput) GammaCorrection(LevelsControlInputRange(color, minInput, maxInput), gamma)
#define LevelsControlOutputRange(color, minOutput, maxOutput)  mix(vec3(minOutput), vec3(maxOutput), color)
#define LevelsControl(color, minInput, gamma, maxInput, minOutput, maxOutput)   LevelsControlOutputRange(LevelsControlInput(color, minInput, gamma, maxInput), minOutput, maxOutput)


void main(void)
{

	if (!sourceDrawing) {
		gl_FragColor = texture2D(utilityTexture, maskc) + baseColor;
		return;
	}

    // deal with alpha separately
    float alpha = texture2D(maskTexture, maskc).a * texture2D(sourceTexture, texc).a  * baseColor.a;
    vec3 transformedRGB;
    
    transformedRGB = mix(vec3(0.62), texture2D(sourceTexture, texc).rgb, contrast);
    transformedRGB += brightness;
    transformedRGB = LevelsControl(transformedRGB, levels.x, gamma, levels.y, levels.z, levels.w);

    if (invertMode==1)
       transformedRGB = vec3(1.0) - transformedRGB;

	if ( abs(saturation -1.0) > 0.01 || threshold > 0.0 || hueshift > 0.0 || nbColors > 0  || chromakey.z > 0.0  || invertMode == 2 ) {

	    vec3 transformedHSL = RGBToHSL( transformedRGB );
	
        if (invertMode == 2)
            transformedHSL.z = 1.0 - transformedHSL.z;
	            
        // perform hue shift
        transformedHSL.x = transformedHSL.x + hueshift; 

        // Saturation
        transformedHSL.y *= saturation;

        // perform reduction of colors
        if (nbColors > 0) {
            transformedHSL *= vec3(nbColors);
            transformedHSL = floor(transformedHSL);
            transformedHSL /= vec3(nbColors);
        }
	        
	    if(threshold > 0.0) {
	        // level threshold
	        if (transformedHSL.z < threshold)
	        	transformedHSL = vec3(0.0, 0.0, 1.0);
	        else
	        	transformedHSL = vec3(0.0, 0.0, 0.0);
	    } 
	    
        if ( chromakey.w > 0.0 ) {
        	if ( all( lessThan( abs(transformedHSL - chromakey.xyz), vec3(chromadelta))) )
           		discard;
   		}
	
	    // after operations on HSL, convert back to RGB
	    transformedRGB = HSLToRGB(transformedHSL);

    } 
    
    // apply base color
    transformedRGB *= baseColor.rgb;

    // bring back the original alpha for final fragment color
    gl_FragColor = vec4(transformedRGB, alpha );

}


