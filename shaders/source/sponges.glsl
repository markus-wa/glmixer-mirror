// Sponges - https://www.shadertoy.com/view/MsjfDK
void mainImage( out vec4 o, vec2 u )
{
    vec3 p = vec3(u/iResolution.x,iTime*.02);
 	for (int i=0 ; i < 99;i++) 
		p.xzy = abs( p/dot(p,p)- vec3(1.0,0.91,0.1) );
	o = vec4(p.xyz, 1.0);    
}