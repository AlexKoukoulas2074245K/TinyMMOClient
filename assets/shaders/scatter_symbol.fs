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
uniform sampler2D scatter_effect_tex;
uniform sampler2D scatter_background_mask_tex;
uniform float custom_alpha;
uniform float time;
uniform float interactive_color_threshold;
uniform float interactive_color_time_multiplier;
uniform float scatter_effect_stretch_multiplier;
uniform bool grayscale;

out vec4 frag_color;

const vec4 INTERACTIVE_COLOR = vec4(0.0f, 0.0f, 0.0f, 1.0f);

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// All components are in the range [0…1], including hue.
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float mapXPositionToHue(float x)
{
    // Map range ref
    // x' = ((x - a) / (b - a)) * (b' - a') + a'
    return ((x + 0.288f) / (0.1f + 0.288f));
}


void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    frag_color = texture(tex, vec2(final_uv_x, final_uv_y));
    
    vec4 maskColor = texture(scatter_effect_tex, vec2(final_uv_x, final_uv_y));
    vec4 background_mask_color = texture(scatter_background_mask_tex, vec2(final_uv_x, final_uv_y));

    if (frag_color.a < 0.1) discard;
    
    float displacement = 0.0;
    float adjustedTime = time;
    if (grayscale)
    {
        adjustedTime = 0.0;
    }

    if (grayscale)
    {
        vec2 displaced_coords = vec2(final_uv_x, final_uv_y) + vec2(0.0, displacement);
        vec4 displaced_mask_color = texture(scatter_effect_tex, displaced_coords);

        frag_color = mix(frag_color, displaced_mask_color, displaced_mask_color.a);
        frag_color.rgb = vec3((frag_color.r + frag_color.g + frag_color.b)/6.0f);
    }
    else
    {
        // Identify letters based on color
        float letterID = floor(final_uv_x * 5.0) / 5.0;
        displacement = sin(time * (letterID + 5.0) + 18.28) * scatter_effect_stretch_multiplier;
        
        // Shift the mask texture coordinates vertically
        vec2 displaced_coords = vec2(final_uv_x, final_uv_y) + vec2(0.0, displacement);
        vec4 displaced_mask_color = texture(scatter_effect_tex, displaced_coords);
        displaced_mask_color.rgb += vec3(max(0.0f, sin(time - letterID)*2.0f)/5.0f);
        
        if (background_mask_color.g > 0.95f && background_mask_color.b < 0.01f && background_mask_color.r < 0.01f)
        {
            float shine = sin(adjustedTime + final_uv_x * 5.5f) * 0.273f + 0.168f;
            vec3 foil_color = vec3(0.576f, 0.576f, 0.576f + 0.2f) * shine;
            frag_color.rgb += foil_color;
        }
        
        
        frag_color = mix(frag_color, displaced_mask_color, displaced_mask_color.a);
    }
    
    frag_color.a *= custom_alpha;
}
