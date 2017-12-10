// https://www.shadertoy.com/view/llBcWR
// The MIT License https://opensource.org/licenses/MIT
// Copyright © 2017 Przemyslaw Zaworski

void mainImage( out vec4 c, in vec2 f )
{
  for (int i = 0; i++ < 10;)  {	
    c.w += .60;	
	vec3 p = c.w * vec3(f/iResolution.xy,2);
	p.x += .015 *iTime;
	p.y += .01 *iTime;
	for (int j = 0; j++ < 50;) 
	    p = vec3(1.25, 1.07, 1.29) * abs(p/dot(p,p) -vec3(.95,.91,.67));
	c.xyz += p * 0.08;	
  }
}