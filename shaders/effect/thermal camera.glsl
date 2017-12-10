// Greyscale to RGB Thermal Camera - https://www.shadertoy.com/view/MdfczN
const bool darkIsHot = true;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iChannelResolution[0].xy;
	vec3 texColor = texture(iChannel0,uv).rgb;

    float a = texColor.r;
    if(darkIsHot)
       a = 1.0 - a;

    fragColor.r = 1.0 - clamp(step(0.166, a)*a, 0.0, 0.333) - 0.667*step(0.333, a) + step(0.666, a)*a + step(0.833, a)*1.0;
    fragColor.b = clamp(step(0.333, a)*a, 0.0, 0.5) + step(0.5, a)*0.5;
    fragColor.g = clamp(a, 0.0, 0.166) + 0.834*step(0.166, a) - step(0.5, a)*a - step(0.666, a)*1.0;
  	fragColor.a = 1.0;
}
