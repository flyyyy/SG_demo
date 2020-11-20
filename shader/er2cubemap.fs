#version 330 core
out vec4 FragColor;

in vec3 Pos;

uniform sampler2D erMap;

const vec2 ratio = vec2(0.5 / 3.14159265, 1.0 / 3.14159265);

vec2 sphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y)) * ratio;
    return uv + 0.5;
}

void main()
{
    vec2 uv = sphericalMap(normalize(Pos));
    vec3 color = texture(erMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
