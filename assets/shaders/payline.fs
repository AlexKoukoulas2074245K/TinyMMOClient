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
uniform float hor_reveal_threshold;
uniform float hor_reveal_threshold_stop_value;
out vec4 frag_color;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    frag_color = texture(tex, vec2(final_uv_x, final_uv_y));

    if (frag_color.a < 0.1) discard;
    if (hor_reveal_threshold < final_uv_x) discard;
    if (hor_reveal_threshold_stop_value < 0.99f)
    {
        vec3 grayscale = vec3((frag_color.r + frag_color.g + frag_color.b)/3.0f);
        float critical_distance = 0.08f;
        float current_distance = final_uv_x - hor_reveal_threshold_stop_value;
        if (current_distance > critical_distance)
        {
            frag_color.rgb = grayscale;
        }
        else if (current_distance <= critical_distance)
        {
            frag_color.rgb = mix(frag_color.rgb, grayscale, current_distance/critical_distance);
        }
    }
    frag_color.a *= custom_alpha;
}
