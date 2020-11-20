#version 330 core
out vec4 FragColor;

in vec3 Pos;

uniform samplerCube envmap;

const float pi = 3.14159265;

void main()
{
    vec3 normal = normalize(Pos);
    vec3 color = vec3(0.0);
    vec3 up = vec3(0.0034, 1.0, 0.0071);
    vec3 right = normalize(cross(up, normal));
    up = cross(normal, right);
    float delta = 0.025;
    float sampleCnt = 0.0;
    for (float phi = 0.0; phi < 2.0 * pi; phi += delta)
    {
        for (float theta = 0.0; theta < 0.5 * pi; theta += delta)
        {
            vec3 sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 v = sample.x * right + sample.y * up + sample.z * normal;
            color += texture(envmap, v).rgb * cos(theta) * sin(theta);
            sampleCnt++;
        }
    }
    color = pi * color / sampleCnt;
    FragColor = vec4(color, 1.0);
}
