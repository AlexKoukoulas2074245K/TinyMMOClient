#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in vec2 uv_frag;
in vec3 frag_unprojected_pos;
in vec3 frag_pos;
in vec3 normal_interp;

uniform sampler2D tex;
uniform float custom_alpha;
uniform float shine_ray_x;
uniform bool grayscale;
out vec4 frag_color;

const float RAY_THICKNESS = 0.2f;
const float RAY_COLOR_STRENGTH = 10.0f;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    frag_color = texture(tex, vec2(final_uv_x, final_uv_y));

    if (frag_color.a < 0.1) discard;
    if (grayscale)
    {
        frag_color.rgb = vec3((frag_color.r + frag_color.g + frag_color.b)/6.0f);
    }
    else
    {
        float distance_to_ray = abs(frag_unprojected_pos.x - shine_ray_x);
        if (distance_to_ray < RAY_THICKNESS)
        {
            float shine_color = (RAY_THICKNESS - distance_to_ray) * RAY_COLOR_STRENGTH;
            frag_color.rgb += vec3(shine_color);
            frag_color.r = min(1.0f, frag_color.r);
            frag_color.g = min(1.0f, frag_color.g);
            frag_color.b = min(1.0f, frag_color.b);
        }
    }
    frag_color.a *= custom_alpha;
}
