# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -g -O0 -static -static-libgcc -static-libstdc++ -Iinclude

# Paths (NO SPACES IN PATHS - fix the SFML path first!)
SFML_PATH = C:/Users/NITRO/Documents/SFML Download/SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit/SFML-3.0.2
ASSIMP_INCLUDE = C:/Users/NITRO/Documents/ASSIMP/assimp-headers
ASSIMP_LIB = C:/Users/NITRO/Documents/MyGameEngine

# Include paths
INCLUDES = -I"$(SFML_PATH)/include" -I"$(ASSIMP_INCLUDE)" -I"include"

# Optionally enable nlohmann/json single-header parsing. To enable, run:
#    make USE_NLOHMANN=1 fetch_nlohmann
# which will download the header into include/nlohmann/json.hpp and build
ifeq ($(USE_NLOHMANN),1)
	CXXFLAGS += -DUSE_NLOHMANN
	# include/ already in INCLUDES
endif

# Library paths and libraries
LDFLAGS = -L"$(SFML_PATH)/lib" -L"$(ASSIMP_LIB)"
LIBS = -lsfml-graphics -lsfml-window -lsfml-system "$(ASSIMP_LIB)/libassimp.dll.a" -lopengl32 -lgdiplus -lole32

# Source files
SRC_DIR = .
CORE_DIR = Core
GRAPHICS_DIR = Graphics
INPUT_DIR = Input
SCENE_DIR = Scene

SOURCES = \
	main.cpp \
	$(CORE_DIR)/Application.cpp \
	$(CORE_DIR)/Engine.cpp \
	$(CORE_DIR)/Time.cpp \
	$(GRAPHICS_DIR)/Renderer.cpp \
	$(GRAPHICS_DIR)/Camera.cpp \
	$(GRAPHICS_DIR)/ShaderManager.cpp \
	$(GRAPHICS_DIR)/Model.cpp \
	$(GRAPHICS_DIR)/Mesh.cpp \
	$(GRAPHICS_DIR)/Light.cpp \
	$(GRAPHICS_DIR)/ManualMesh.cpp \
	$(GRAPHICS_DIR)/TextureManager.cpp \
	$(GRAPHICS_DIR)/DebugQuad.cpp \
	$(INPUT_DIR)/InputSystem.cpp \
	$(SCENE_DIR)/Scene.cpp \
	$(SCENE_DIR)/GameObject.cpp \
	src/glad.c \
	src/stb_image_impl.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)

# Output executable
TARGET = game.exe

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS) $(LIBS)
	@echo "✅ BUILD SUCCESSFUL!"

# Compile C++ source files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile C source files
%.o: %.c
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	@echo "Cleaning build files..."
	del /Q $(subst /,\,$(OBJECTS)) $(TARGET) 2>nul || true

# Fetch nlohmann json.hpp single-header via PowerShell (Windows)
.PHONY: fetch_nlohmann
fetch_nlohmann:
	@echo "Fetching nlohmann/json.hpp into include/nlohmann/..."
	@powershell -NoProfile -Command "mkdir -Force include\nlohmann | Out-Null; Invoke-WebRequest -UseBasicParsing -Uri 'https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp' -OutFile 'include\\nlohmann\\json.hpp'"
	@echo "Downloaded include/nlohmann/json.hpp"

# Run the game
run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean run rebuild