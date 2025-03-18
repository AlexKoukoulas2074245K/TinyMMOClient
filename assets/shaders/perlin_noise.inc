float transformTime(float time)
{
    float usableTime = mod(time, 60.0f);
    if (usableTime >= 30.0f)
    {
        usableTime = 60.0f - usableTime;
    }
    return usableTime;
}

vec2 randomGradient(vec2 p, float time, float time_speed)
{
    p = p + 0.02f;
    float x = dot(p, vec2(0.1234f, 0.2345f));
    float y = dot(p, vec2(0.2345f, 0.3456f));
    vec2 gradient = vec2(x, y);
    gradient = sin(gradient);
    gradient = gradient * 43.7585453f;//43758.5453f;
    
    // part 4.5 - update noise function with time
    gradient = sin(gradient + transformTime(time) * time_speed);
    return gradient;
}

vec2 cubic(vec2 p)
{
    return p * p * (3.0 - p * 2.0);
}

vec2 quintic(vec2 p)
{
    return p * p * p * (10.0 + p * (-15.0 + p * 6.0));
}

float perlin(float perlin_resolution, float time, float time_speed)
{
    float final_uv_x = uv_frag.x;
    float final_uv_y = 1.0 - uv_frag.y;
    vec2 coord = vec2(final_uv_x * 480.0f, final_uv_y * 480.0f);
    
    // part 0 - basic shader setup
    vec2 res = vec2(perlin_resolution, perlin_resolution);
    vec2 uv = coord.xy/res.y;
    
    vec3 black = vec3(0.0);
    vec3 white = vec3(1.0);
    vec3 color = black;
    
    // part 1 - set up a grid of cells
    uv = uv * 4.0;
    vec2 gridId = floor(uv);
    vec2 gridUv = fract(uv);
    color = vec3(gridId, 0.0);
    color = vec3(gridUv, 0.0);
    
    // part 2.1 - start by finding the coords of grid corners
    vec2 bl = gridId + vec2(0.0, 0.0);
    vec2 br = gridId + vec2(1.0, 0.0);
    vec2 tl = gridId + vec2(0.0, 1.0);
    vec2 tr = gridId + vec2(1.0, 1.0);
    
    // part 2.2 - find random gradient for each grid corner
    vec2 gradBl = randomGradient(bl, time, time_speed);
    vec2 gradBr = randomGradient(br, time, time_speed);
    vec2 gradTl = randomGradient(tl, time, time_speed);
    vec2 gradTr = randomGradient(tr, time, time_speed);
    
    // part 3.2 - find distance from current pixel to each grid corner
    vec2 distFromPixelToBl = gridUv - vec2(0.0, 0.0);
    vec2 distFromPixelToBr = gridUv - vec2(1.0, 0.0);
    vec2 distFromPixelToTl = gridUv - vec2(0.0, 1.0);
    vec2 distFromPixelToTr = gridUv - vec2(1.0, 1.0);
    
    // part 4.1 - calculate the dot products of gradients + distances
    float dotBl = dot(gradBl, distFromPixelToBl);
    float dotBr = dot(gradBr, distFromPixelToBr);
    float dotTl = dot(gradTl, distFromPixelToTl);
    float dotTr = dot(gradTr, distFromPixelToTr);
 
    gridUv = quintic(gridUv);
    
    float b = mix(dotBl, dotBr, gridUv.x);
    float t = mix(dotTl, dotTr, gridUv.x);
    return mix(b, t, gridUv.y);
}
