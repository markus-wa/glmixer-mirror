// simplest chroma key possible on green
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
   vec2 uv = fragCoord.xy / iResolution.xy;
   vec4 tex = texture(iChannel0, uv);
   float a = (1.0-tex.g)+tex.r*0.5+tex.b*0.5;

   fragColor = vec4(tex.rgb, a);
}


