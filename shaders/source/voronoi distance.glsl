// https://www.shadertoy.com/view/ldl3W8
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm

#define ANIMATE

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

vec2 hash( vec2 p )
{
    p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return fract(sin(p)*43758.5453);
}

vec3 voronoi( in vec2 x )
{
    vec2 n = floor(x);
    vec2 f = fract(x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
	vec2 mg = vec2(0,0);
	vec2 mr = vec2(0,0);

    float md = 8.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        vec2 g = vec2(float(i),float(j));
		vec2 o = hash( n + g );
		#ifdef ANIMATE
        o = 0.5 + 0.5*sin( iTime + 6.2831*o );
        #endif	
        vec2 r = g + o - f;
        float d = dot(r,r);

        if( d<md )
        {
            md = d;
            mr = r;
            mg = g;
        }
    }

    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = 8.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = mg + vec2(float(i),float(j));
		vec2 o = hash( n + g );
		#ifdef ANIMATE
        o = 0.5 + 0.5*sin( iTime + 6.2831*o );
        #endif	
        vec2 r = g + o - f;

		
        if( dot(mr-r,mr-r)>0.000001 )
		{
        // distance to line		
        float d = dot( 0.5*(mr+r), normalize(r-mr) );

        md = min( md, d );
		}
    }

    return vec3( md, mr );
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = fragCoord.xy/iResolution.xx;

    vec3 c = voronoi( 8.0*p );

	// isolines
    vec3 col = c.x*(0.5 + 0.5*sin(64.0*c.x))*vec3(1.0);
    // borders	
    col = mix( vec3(1.0,0.6,0.0), col, smoothstep( 0.04, 0.07, c.x ) );
    // feature points
	float dd = length( c.yz );
	col = mix( vec3(1.0,0.6,0.1), col, smoothstep( 0.0, 0.12, dd) );
	col += vec3(1.0,0.6,0.1)*(1.0-smoothstep( 0.0, 0.04, dd));

	fragColor = vec4(col,1.0);
}
