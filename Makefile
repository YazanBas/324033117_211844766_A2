workspaceFolder = .

# Detect OS
ifeq ($(OS),Windows_NT) # Windows
    CPPFLAGS = g++ --std=c++17 -fdiagnostics-color=always -Wall -g -I${workspaceFolder}/include -I${workspaceFolder}/src
    CFLAGS = gcc -std=c11 -Wall -g -I${workspaceFolder}/include -I${workspaceFolder}/src
    CLIBS =
    LDFLAGS =
    all: build
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Darwin) # macOS
        CPPFLAGS = clang++ -std=c++17 -fcolor-diagnostics -fansi-escape-codes -Wall -g -I${workspaceFolder}/include -I${workspaceFolder}/src
        CFLAGS = clang -std=c11 -Wall -g -I${workspaceFolder}/include -I${workspaceFolder}/src
        CLIBS =
        LDFLAGS =
        all: build
    else ifeq ($(UNAME_S), Linux) # Linux
        CPPFLAGS = g++ --std=c++17 -fdiagnostics-color=always -Wall -g -I${workspaceFolder}/include -I${workspaceFolder}/src
        CFLAGS = gcc -std=c11 -Wall -g -I${workspaceFolder}/include -I${workspaceFolder}/src
        CLIBS =
        LDFLAGS =
        all: build
    else
        $(error Unsupported OS: $(UNAME_S))
    endif
endif

# Source and object files
SRC_FILES = ${workspaceFolder}/src/main.cpp ${workspaceFolder}/src/RayTracer.cpp ${workspaceFolder}/src/stb_image.cpp ${workspaceFolder}/src/stb_image_write.cpp
OBJ_FILES = $(patsubst ${workspaceFolder}/src/%.cpp, ${workspaceFolder}/bin/%.o, $(SRC_FILES))

# Rule to compile .o files from .cpp files
${workspaceFolder}/bin/%.o: ${workspaceFolder}/src/%.cpp | $(workspaceFolder)/bin
	$(CPPFLAGS) -c $< -o $@

build: $(OBJ_FILES) | $(workspaceFolder)/bin
	$(CPPFLAGS) $(CLIBS) $(OBJ_FILES) -o ${workspaceFolder}/bin/main $(LDFLAGS)

# Cleanup
clean:
	rm -f ${workspaceFolder}/bin/*.o ${workspaceFolder}/bin/main

# Copy library and resources (MacOS)
copy_lib_m:
	@echo "Copying library for MacOS..."
	cp -rf ${workspaceFolder}/lib/macOS/libglfw.3.dylib ${workspaceFolder}/bin/libglfw.3.dylib

copy_res_m:
	@echo "Copying resources for MacOS..."
	mkdir -p ${workspaceFolder}/bin/res && cp -rf ${workspaceFolder}/src/res/* ${workspaceFolder}/bin/res

# Copy library and resources (Windows)
copy_lib_w:
	@echo "Copying library for Windows..."
	cmd /c xcopy /D /I ${workspaceFolder}\lib\windows\glfw3.dll ${workspaceFolder}\bin\

copy_res_w:
	@echo "Copying resources for Windows..."
	cmd /c xcopy /E /I /Y ${workspaceFolder}\src\res ${workspaceFolder}\bin\res

# Copy library and resources (Linux)
copy_lib_l:
	@echo "Copying library for Linux... [Skipped!]"
	# cp -rf ${workspaceFolder}/lib/linux/* ${workspaceFolder}/bin/

copy_res_l:
	@echo "Copying resources for Linux..."
	mkdir -p ${workspaceFolder}/bin/res && cp -rf ${workspaceFolder}/src/res/* ${workspaceFolder}/bin/res

# Parallel build (add -jN option to run with N jobs)
.PHONY: all clean copy_res_m copy_res_w
