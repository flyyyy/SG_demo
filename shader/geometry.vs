#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in float aMask;

out vec3 Pos;
out vec3 Normal;
out vec2 TexCoords;
out float Mask;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 modelPos = model * vec4(aPos, 1.0);
    Pos = modelPos.xyz;
    Normal = transpose(inverse(mat3(model))) * aNormal;
    TexCoords = aTexCoords;
    Mask = aMask;
    gl_Position = projection * view * modelPos;
}
