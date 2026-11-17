# Makefile do simulador de aterragem: compila src/*.cpp para obj/*.o e liga tudo em bin/aterragem.
# Comandos: make (compilar), make run (compilar e executar), make clean (apagar o que foi compilado).
SHELL = /bin/sh
CXX := g++ -std=c++17
CXXFLAGS := -O2 -Wall -MMD
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
INC_DIR := .

EXE := $(BIN_DIR)/aterragem
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# ver o sistema operativo (Windows com MSYS2, macOS ou Linux) para escolher as bibliotecas a ligar
UNAME := $(shell uname -s)
ifeq ($(OS),Windows_NT)
	EXE := $(BIN_DIR)/aterragem.exe
	CXXFLAGS += -D_USE_MATH_DEFINES
	LDLIBS := -lglfw3 -lglew32 -lopengl32
else ifeq ($(UNAME),Darwin)
	BREW := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
	CXXFLAGS += -I$(BREW)/include -DGL_SILENCE_DEPRECATION
	LDLIBS := -L$(BREW)/lib -lglfw -lGLEW -framework OpenGL
else
	LDLIBS := -lm -lGL -lGLEW -lglfw
endif

# alvos: all (compilar), run (compilar e executar), noite (executar de noite) e clean
.PHONY: all run noite clean

all: $(EXE)

run: $(EXE)
	./$(EXE)

noite: $(EXE)
	./$(EXE) noite

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR) build

# dependências dos .hpp geradas pelo compilador (-MMD)
-include $(OBJ:.o=.d)
