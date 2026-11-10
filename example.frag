#version 330 core

in vec4 ex_Color;
out vec4 out_Color;

uniform int codCol;

void main(void)
{
    switch (codCol)
    {
        case 0:
            // Culoare din atributul de varf
            out_Color = ex_Color;
            break;
        case 1:
            // maro
            out_Color = vec4(0.38f, 0.20f, 0.07f, 1.0f);
            break;
        case 2:
            // verde
            out_Color = vec4(0.12f, 0.52f, 0.28f, 1.0f);
            break;
        case 3:
            // negru
            out_Color = vec4(0.08f, 0.08f, 0.08f, 1.0f);
            break;
        case 4:
            // Bila alba
            out_Color = vec4(0.95f, 0.95f, 0.93f, 1.0f);
            break;
        case 5:
            // Linie de tintire sau elemente speciale
            out_Color = vec4(1.0f, 1.0f, 1.0f, 0.6f);
            break;
        default:
            out_Color = ex_Color;
            break;
    }
}
