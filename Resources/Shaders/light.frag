#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;
uniform float lightScale;

void main()
{
    FragColor = vec4(lightColor * lightScale, 1.0);
}