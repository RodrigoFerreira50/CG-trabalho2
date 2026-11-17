// Texturas: leitura de imagens PPM / PGM (o formato mais simples que há) e envio para a placa gráfica
// como texturas 2D (com mipmaps) ou como cube map (o céu).
#include <cstdio>
#include <fstream>
#include <map>

#include <common/texture.hpp>

// Lê o cabeçalho (tipo, largura, altura, valor máximo) e depois os píxeis em binário. Com valores até
// 255 cada canal ocupa 1 byte; acima disso (o relevo, 16 bits) ocupa 2 bytes.
Image readPNM(const std::string& path) {
    Image image;
    std::string type;
    std::ifstream file(path, std::ios::binary);
    if (!(file >> type >> image.width >> image.height >> image.maxValue)) {
        printf("Não foi possível abrir %s\n", path.c_str());
        return image;
    }
    file.get();  // um só espaço antes dos píxeis
    image.pixels.resize(size_t(image.width) * image.height * (type == "P6" ? 3 : 1) * (image.maxValue > 255 ? 2 : 1));
    file.read((char*)image.pixels.data(), image.pixels.size());
    return image;
}

// Cria uma textura 2D a partir de uma imagem PPM, com mipmaps e filtragem trilinear e anisotrópica.
GLuint loadTexture(const std::string& path) {
    static std::map<std::string, GLuint> cache;  // caminho -> textura já carregada
    if (cache.count(path)) return cache[path];
    Image image = readPNM(path);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // as linhas da imagem não têm bytes de enchimento
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);  // versões mais pequenas, usadas ao longe
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);  // nítida vista de lado (pista)
    return cache[path] = textureID;
}

// Cria o cube map do céu: seis imagens, uma por face do cubo (+X, -X, +Y, -Y, +Z, -Z). O shader do céu
// lê-o com a direção de cada píxel.
GLuint loadCubemap(const std::string& dir) {
    const char* faces[] = {"right", "left", "top", "bottom", "front", "back"};
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < 6; i++) {
        Image image = readPNM(dir + faces[i] + ".ppm");
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, image.width, image.height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, image.pixels.data());
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // sem repetição nas arestas, para não se verem as costuras entre as faces
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}
