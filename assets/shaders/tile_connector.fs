#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in vec2 uv_frag;
in vec3 frag_unprojected_pos;
in vec3 frag_pos;
in vec3 normal_interp;

uniform sampler2D tile_tex_a;
uniform sampler2D tile_tex_b;
uniform vec3 point_light_position;
uniform float point_light_power;
uniform float custom_alpha;
uniform bool affected_by_light;
out vec4 frag_color;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    
    vec4 frag_color_a = texture(tile_tex_a, vec2(final_uv_x, final_uv_y));
    vec4 frag_color_b = texture(tile_tex_b, vec2(final_uv_x, final_uv_y));

    frag_color = mix(frag_color_a, frag_color_b, final_uv_x);
    frag_color.a *= custom_alpha;
}
