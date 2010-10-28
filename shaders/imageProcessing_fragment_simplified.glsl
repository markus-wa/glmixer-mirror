
varying vec2 texc;
varying vec2 maskc;
varying vec4 baseColor;

uniform sampler2D sourceTexture;
uniform sampler2D maskTexture;
uniform sampler2D utilityTexture;

uniform bool sourceDrawing;
uniform float gamma;
uniform vec4 levels;



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

    float alpha = texture2D(maskTexture, maskc).a * texture2D(sourceTexture, texc).a  * baseColor.a;

    vec3 transformedRGB = LevelsControl(texture2D(sourceTexture, texc).rgb, levels.x, gamma, levels.y, levels.z, levels.w);

    gl_FragColor = vec4(transformedRGB * baseColor.rgb, alpha );

}


