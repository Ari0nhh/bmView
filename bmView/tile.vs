R"(
#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

uniform mat4 world;
uniform vec3 pos;
uniform vec3 size;

out vec2 txCoord;

void main()
{
	gl_Position = world * vec4(position.x * size.x + pos.x, position.y * size.y + pos.y, 0.f, 1.0f);
	txCoord = texCoord;
}

)"