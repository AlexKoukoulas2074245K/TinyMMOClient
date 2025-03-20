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
uniform float time;
uniform float interactive_color_threshold;
uniform float interactive_color_time_multiplier;
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

    if (frag_color.a < 0.1) discard;
    
    if (distance(frag_color, INTERACTIVE_COLOR) < interactive_color_threshold)
    {
        float initialHueValue = mapXPositionToHue(frag_unprojected_pos.x);
        frag_color.rgb = hsv2rgb(vec3(initialHueValue + (time * interactive_color_time_multiplier), 1.0f, 1.0f));
    }
    
    frag_color.a *= custom_alpha;
}
