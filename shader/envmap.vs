#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 Pos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    Pos = aPos;
    gl_Position = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
}