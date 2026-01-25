#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 position;
layout(location = 3) in vec3 scale;
layout(location = 4) in vec2 min_uv;
layout(location = 5) in vec2 max_uv;
layout(location = 6) in float alpha;

uniform mat4 view;
uniform mat4 proj;

out vec2 uv_frag;
out vec3 frag_unprojected_pos;
out float frag_alpha;

void main()
{
    uv_frag = uv;
    if (uv_frag.x > 0.0f) uv_frag.x = max_uv.x;
    else                  uv_frag.x = min_uv.x;
    
    if (uv_frag.y > 0.0f) uv_frag.y = max_uv.y;
    else                  uv_frag.y = min_uv.y;
    
    frag_unprojected_pos = (vertex_position  * scale) + position;
//    vec3 rotated_position = scaled_position;
//    frag_unprojected_pos = rotated_position + position;
    frag_alpha = alpha;

    gl_Position = proj * view * vec4(frag_unprojected_pos, 1.0f);
}
