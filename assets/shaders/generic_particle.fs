#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in float frag_lifetime;
in vec2 uv_frag;
in vec3 frag_unprojected_pos;
out vec4 frag_color;

uniform float custom_alpha;
uniform sampler2D tex;

void main()
{
    // Calculate final uvs
    float finalUvX = uv_frag.x;
    float finalUvY = 1.00 - uv_frag.y;

    // Get texture color
    frag_color = texture(tex, vec2(finalUvX, finalUvY));
    frag_color.r = frag_color.r * min(1.0f, frag_lifetime);
    frag_color.g = frag_color.g * min(1.0f, frag_lifetime);
    frag_color.b = frag_color.b * min(1.0f, frag_lifetime);
    frag_color.a = max(frag_color.r, max(frag_color.g, frag_color.b)) * min(1.0f, frag_lifetime);
    if (frag_color.a < 0.1) discard;
    
    frag_color.a *= custom_alpha;
}
