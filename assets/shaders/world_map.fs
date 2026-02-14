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
uniform float map_width;
uniform float map_height;
uniform bool debug_grid;
out vec4 frag_color;

const float MAP_SEAM_OFFSET_X = 0.0f;
const float MAP_SEAM_OFFSET_Y = 0.0f;
const float MAP_SEAM_SIZE = 0.0002f;
const float MAP_TEXTURE_SIZE = 4096.0f;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    frag_color = texture(tex, vec2(final_uv_x, final_uv_y));
    
    if ((abs((final_uv_x + MAP_SEAM_OFFSET_X) - (0.5f + map_width/2)) < MAP_SEAM_SIZE) ||
        (abs((final_uv_x + MAP_SEAM_OFFSET_X) - (0.5f - map_width/2)) < MAP_SEAM_SIZE))
    {
        if (final_uv_y + MAP_SEAM_OFFSET_Y > (0.5f - map_height/2) &&
            final_uv_y + MAP_SEAM_OFFSET_Y < (0.5f + map_height/2))
        {
            frag_color = texelFetch(tex, ivec2(final_uv_x * MAP_TEXTURE_SIZE, final_uv_y * MAP_TEXTURE_SIZE), 0);
        }
    }
    
    if ((abs((final_uv_y + MAP_SEAM_OFFSET_Y) - (0.5f + map_height/2)) < MAP_SEAM_SIZE) ||
        (abs((final_uv_y + MAP_SEAM_OFFSET_Y) - (0.5f - map_height/2)) < MAP_SEAM_SIZE))
    {
        if (final_uv_x + MAP_SEAM_OFFSET_X > (0.5f - map_width/2) &&
            final_uv_x + MAP_SEAM_OFFSET_X < (0.5f + map_width/2))
        {
            frag_color = texelFetch(tex, ivec2(final_uv_x * MAP_TEXTURE_SIZE, final_uv_y * MAP_TEXTURE_SIZE), 0);
        }
    }
    
    if (debug_grid)
    {
        vec2 grid_uv = vec2(final_uv_x * 64.0f, final_uv_y * 64.0f);
        vec2 cell = abs(fract(grid_uv) - 0.5f);
        float lineWidth = 0.02f;
        float line = smoothstep(0.5f - lineWidth, 0.5f, max(cell.x, cell.y));
        frag_color = mix(frag_color, vec4(vec3(1.0f - line), line), line);
    }
    
    if (frag_color.a < 0.1) discard;
    frag_color.a *= custom_alpha;
}
