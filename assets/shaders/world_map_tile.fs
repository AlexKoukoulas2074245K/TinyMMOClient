#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in vec2 uv_frag;
in vec3 frag_unprojected_pos;
in vec3 frag_pos;
in vec3 normal_interp;

uniform sampler2D icon_tex;
uniform sampler2D tile_tex_a;
uniform sampler2D tile_tex_b;
uniform vec3 point_light_position;
uniform float point_light_power;
uniform float custom_alpha;
uniform bool affected_by_light;
uniform bool highlighted;
uniform int connector_type; // 0=none  1=|  2=-  3=topleft  4=topright 5=botright 6=botleft  7=invalid
out vec4 frag_color;

const vec4 INVALID_COLOR = vec4(1.0f, 0.0f, 0.0f, 1.0f);
const vec4 HIGHLIGHTED_EDGES_COLOR = vec4(0.5f, 0.5f, 1.0f, 1.0f);
const float HIGHLIGHTED_COLOR_INCREASE = 0.2f;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    
    frag_color = texture(icon_tex, vec2(final_uv_x, final_uv_y));
    vec4 frag_color_a = texture(tile_tex_a, vec2(final_uv_x, final_uv_y));
    vec4 frag_color_b = texture(tile_tex_b, vec2(final_uv_x, final_uv_y));
    
    switch (connector_type)
    {
        case 0: break;
        case 1: frag_color = mix(frag_color_a, frag_color_b, final_uv_y); break;
        case 2: frag_color = mix(frag_color_a, frag_color_b, final_uv_x); break;
        case 3: frag_color = mix(frag_color_a, frag_color_b, min(final_uv_x, final_uv_y)); break;
        case 4: frag_color = mix(frag_color_a, frag_color_b, max(final_uv_x, 1.0f - final_uv_y)); break;
        case 5: frag_color = mix(frag_color_a, frag_color_b, max(final_uv_x, final_uv_y)); break;
        case 6: frag_color = mix(frag_color_a, frag_color_b, min(final_uv_x, 1.0f - final_uv_y)); break;
        case 7: frag_color *= INVALID_COLOR; break;
        default: break;
    }
    
    if (highlighted)
    {
        frag_color += HIGHLIGHTED_COLOR_INCREASE;
        
        if (final_uv_x < 0.05f || final_uv_x > 0.95f || final_uv_y < 0.05f || final_uv_y > 0.95f)
        {
            frag_color = HIGHLIGHTED_EDGES_COLOR;
        }
    }
}
