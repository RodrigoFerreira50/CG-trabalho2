#ifndef SHADER_HPP
#define SHADER_HPP

// Lê, compila e liga um vertex shader e um fragment shader; devolve o programa GLSL.
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);

#endif
