R"(

#version 330 core

in vec2 txCoord;
out vec4 color_out;
uniform sampler2D map_data;

void main()
{
	color_out = texture2D(map_data, txCoord);
}

)"