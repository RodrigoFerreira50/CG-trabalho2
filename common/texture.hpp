#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>
#include <vector>

#include <GL/glew.h>

// Imagem PPM (P6, RGB) ou PGM (P5, tons de cinzento, 8 ou 16 bits): um cabeçalho de texto
// "P6 largura altura máximo" e depois os píxeis em binário, linha a linha de cima para baixo.
struct Image {
    int width = 0, height = 0, maxValue = 0;
    std::vector<unsigned char> pixels;
};
Image readPNM(const std::string& path);

// Textura 2D com mipmaps (cada ficheiro só vai uma vez para a placa gráfica).
GLuint loadTexture(const std::string& path);

// Cube map do céu (skybox): dir + right, left, top, bottom, front, back (.ppm).
GLuint loadCubemap(const std::string& dir);

#endif
