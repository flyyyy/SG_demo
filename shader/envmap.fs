#version 330 core
out vec4 FragColor;

in vec3 Pos;

uniform samplerCube envmap;

void main()
{
    vec3 color = texture(envmap, Pos).rgb;
    FragColor = vec4(color, 1.0);
}
