// https://www.shadertoy.com/view/XsVSzW
// Version by GregRostami
void mainImage(out vec4 o,vec2 u)
{
    u = 8.*u / iResolution.x-.5;   
    float t=iTime, h=.5, a=1., b=a, c=a, d=0.;
    vec2 r;
	for(int i=0;i<7;i++)
		r=vec2(cos(u.y*a-d+t/b),sin(u.x*a-d+t/b))/c,
	u+=r+r.yx*.3,
	a*=1.93,
	b*=1.15,
	c*=1.7,
	d+=.05+.1*t*b;
	o = vec4(sin(u.x-t)*h+h, sin((u.x+u.y+sin(t*h))*h)*h+h, sin(u.y+t)*h+h, 1);
}