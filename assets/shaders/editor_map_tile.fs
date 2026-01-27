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
uniform int navmap_tile_type;
uniform bool is_navmap_tile;
uniform float navmap_tile_color_r;
uniform float navmap_tile_color_g;
uniform float navmap_tile_color_b;
uniform float navmap_tile_color_a;
out vec4 frag_color;
const float HIGHLIGHTED_COLOR_INCREASE = 0.2f;

void main()
{
    if (is_navmap_tile)
    {
        frag_color = vec4(navmap_tile_color_r, navmap_tile_color_g, navmap_tile_color_b, navmap_tile_color_a);
    }
    else
    {
        float final_uv_x = uv_frag.x * 0.999f;
        float final_uv_y = 1.0 - uv_frag.y;
        
        frag_color = texture(icon_tex, vec2(final_uv_x, final_uv_y));
        vec4 frag_color_a = texture(tile_tex_a, vec2(final_uv_x, final_uv_y));
        vec4 frag_color_b = texture(tile_tex_b, vec2(final_uv_x, final_uv_y));
        
        if (highlighted)
        {
            frag_color += HIGHLIGHTED_COLOR_INCREASE;
        }
    }

    frag_color.a *= custom_alpha;
}
