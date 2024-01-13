#version 430

uniform vec4 qt_lineColor;

out vec4 fragColor;

void main()
{
    fragColor = qt_lineColor;
}
