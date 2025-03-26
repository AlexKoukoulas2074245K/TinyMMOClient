#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 position;
layout(location = 3) in float lifetime;
layout(location = 4) in float size;
layout(location = 5) in float angle;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 rotation_axis;

out float frag_lifetime;
out vec2 uv_frag;
out vec3 frag_unprojected_pos;

mat3 calculate_rotation_matrix(vec3 axis, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c        );
}


void main()
{
    uv_frag = uv;
    
    mat3 rotation_matrix = calculate_rotation_matrix(rotation_axis, -angle);
    
    vec3 scaled_position = vertex_position * size;
    vec3 rotated_position = scaled_position * rotation_matrix;
    frag_unprojected_pos = rotated_position + position;

    frag_lifetime = lifetime;
    gl_Position = proj * view * vec4(frag_unprojected_pos, 1.0f);
}
