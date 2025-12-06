#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;

void main()
{
    vec3 worldPos = aPos * 0.3 + lightPos;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}