#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in vec2 uv_frag;
in vec3 frag_unprojected_pos;
in vec3 frag_pos;
in vec3 normal_interp;

uniform sampler2D bar_frame;
uniform sampler2D bar_progress;
uniform vec4 color_factor;
uniform float custom_alpha;
uniform float fill_progress;

out vec4 frag_color;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0f - uv_frag.y;
    frag_color = texture(bar_frame, vec2(final_uv_x, final_uv_y));
    
    if (frag_color.a < 0.1f) discard;
    
    vec4 progress_color = texture(bar_progress, vec2(final_uv_x, final_uv_y));
    if (final_uv_x <= fill_progress && progress_color.a > 0.0f)
    {
        progress_color = mix(progress_color, color_factor, progress_color.r);
        frag_color = mix(frag_color, progress_color, progress_color.a);
    }
    
    float glow_factor = abs(final_uv_x - fill_progress);
    if (glow_factor < 0.02f && progress_color.a > 0.0f)
    {
        vec4 glow_color = vec4(1.0f, 1.0f, 1.0f, mix(0.0f, 1.0f, 1.0f - (glow_factor/0.02f)));
        frag_color = mix(frag_color, glow_color, glow_color.a);
    }
    
    frag_color.a *= custom_alpha;
}
