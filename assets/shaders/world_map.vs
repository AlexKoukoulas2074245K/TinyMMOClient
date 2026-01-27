#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

uniform mat4 world;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 rot;

uniform bool texture_sheet;
uniform float min_u;
uniform float min_v;
uniform float max_u;
uniform float max_v;

out vec2 uv_frag;
out vec3 frag_unprojected_pos;
out vec3 frag_pos;
out vec3 normal_interp;

void main()
{
    uv_frag = uv;
    normal_interp = (rot * vec4(normal, 0.0f)).rgb;
    
    if (texture_sheet)
    {
        if (uv_frag.x > 0.0f) uv_frag.x = max_u;
        else                  uv_frag.x = min_u;
        
        if (uv_frag.y > 0.0f) uv_frag.y = max_v;
        else                  uv_frag.y = min_v;
    }
    
    gl_Position = proj * view * world * vec4(position, 1.0f);
    frag_unprojected_pos = (world * vec4(position, 1.0f)).rgb;
    frag_pos = gl_Position.rgb;
}
