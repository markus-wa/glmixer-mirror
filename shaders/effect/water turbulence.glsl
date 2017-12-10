// Water turbulence effect by joltz0r 2013-07-04, improved 2013-07-07
// https://www.shadertoy.com/view/ltSczG

#define TAU 6.28318530718
#define MAX_ITER 5

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float time = iTime * .5 + 23.0;
    // uv should be the 0-1 uv of texture...
	vec2 uv = fragCoord.xy / iResolution.xy;

    vec2 p = mod(uv*TAU, TAU)-250.0;
	vec2 i = vec2(p);
	float c = 1.0;
	float inten = .005;

	for (int n = 0; n < MAX_ITER; n++) {
		float t = time * (1.0 - (3.5 / float(n+1)));
		i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
	}
	c /= float(MAX_ITER);
	c = 1.17-pow(c, 1.4);
	vec3 colour = vec3(pow(abs(c), 8.0));
    colour = clamp((colour + vec3(0.05)), 0.0, 0.5);

    // added distortion of background image
	vec2 coord = fragCoord.xy / iResolution.xy;

    // perterb uv based on value of c from caustic calc above
    vec2 tc = vec2(cos(c)-0.75,sin(c)-0.75)*0.04;
    coord = clamp(coord + tc,0.0,1.0);

    fragColor = texture(iChannel0, coord);
    // give transparent pixels a color
    if (fragColor.a == 0.0 )
        fragColor=vec4(1.0,1.0,1.0,1.0);    
    fragColor += vec4(colour, 1.0);
}
