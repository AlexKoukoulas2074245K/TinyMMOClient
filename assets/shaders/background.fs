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
uniform sampler2D background_mask_tex;
uniform float custom_alpha;
uniform float mask_alpha_comp;
out vec4 frag_color;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    frag_color = texture(tex, vec2(final_uv_x, final_uv_y));
    
    frag_color.a *= min(1.0f, mask_alpha_comp + texture(background_mask_tex, vec2(final_uv_x, final_uv_y)).a);
    frag_color.a *= custom_alpha;
}
