#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in vec2 uv_frag;
in vec3 frag_unprojected_pos;
in float frag_alpha;
out vec4 frag_color;

uniform sampler2D tex;

void main()
{
    // Calculate final uvs
    float finalUvX = uv_frag.x;
    float finalUvY = 1.00 - uv_frag.y;

    // Get texture color
    frag_color = texture(tex, vec2(finalUvX, finalUvY));
    frag_color.a *= frag_alpha;
}
