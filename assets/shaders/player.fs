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
uniform float custom_alpha;
uniform bool affected_by_light;
uniform bool is_local;
out vec4 frag_color;

void main()
{
    float final_uv_x = uv_frag.x * 0.999f;
    float final_uv_y = 1.0 - uv_frag.y;
    frag_color = texture(tex, vec2(final_uv_x, final_uv_y));

    if (frag_color.a < 0.1) discard;
    
    if (!is_local)
    {
        frag_color *= vec4(1.0f, 0.1f, 0.1f, 1.0f);
    }

    if (affected_by_light)
    {
        vec4 light_accumulator = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        vec3 view_direction = normalize(vec3(0.0f, 0.0f, -1.0f) /* eye pos */ - frag_pos);
        
        for (int i = 0; i < 1; ++i)
        {
            vec3 normal = normalize(normal_interp);

            vec3 light_direction = normalize(point_light_position - frag_pos);
            vec4 diffuse_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
            vec4 specular_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);

            float diffuse_factor = max(dot(normal, light_direction), 0.0f);
            if (diffuse_factor > 0.0f)
            {
                diffuse_color = vec4(0.6f, 0.6f, 0.6f, 1.0f) /* mat_diffuse */ * diffuse_factor;
                diffuse_color = clamp(diffuse_color, 0.0f, 1.0f);

                vec3 reflected_direction = normalize(reflect(-light_direction, normal));

                specular_color = vec4(0.8f, 0.8f, 0.8f, 1.0f) /* mat_spec */ * pow(max(dot(view_direction, reflected_direction), 0.0f), 1.0f /* shiny */);
                specular_color = clamp(specular_color, 0.0f, 1.0f);
            }
            
            float distance = distance(point_light_position, frag_unprojected_pos);
            float attenuation = point_light_power / (distance * distance);
            
            light_accumulator.rgb += (diffuse_color * attenuation + specular_color * attenuation).rgb;
        }
        
        frag_color = frag_color * vec4(0.5f, 0.5f, 0.5f, 1.0f) /* ambient_light_color */ + light_accumulator;
    }
    
    frag_color.a *= custom_alpha;
}
