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
uniform vec3 point_light_position;
uniform float point_light_power;
uniform float time;
uniform float custom_alpha;
uniform float time_speed;
uniform float perlin_resolution;
uniform float perlin_clarity;
uniform float perlin_color_r_multipier;
uniform float perlin_color_g_multipier;
uniform float perlin_color_b_multipier;
uniform bool affected_by_light;
out vec4 frag_color;

#include "perlin_noise.inc"

void main()
{
    float perlinNoise = perlin(perlin_resolution, time, time_speed);
        
    float distanceFromCenter = 1.0f - distance(uv_frag, vec2(0.5f, 0.5f));
    distanceFromCenter -= 0.5f;
    
    frag_color = vec4(perlin_color_r_multipier * vec3(perlinNoise + perlin_clarity).r, perlin_color_g_multipier * vec3(perlinNoise + perlin_clarity).g, perlin_color_b_multipier * vec3(perlinNoise + perlin_clarity).b, (perlinNoise + perlin_clarity) * distanceFromCenter);

    frag_color.a *= custom_alpha;
}
