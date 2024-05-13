#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform vec3 navmap_tile_color;

out vec4 frag_color;

void main()
{
    frag_color = vec4(navmap_tile_color, 1.0f);
}
