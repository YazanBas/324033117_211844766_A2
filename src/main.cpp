#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Debugger.h>
#include <VertexBuffer.h>
#include <VertexBufferLayout.h>
#include <IndexBuffer.h>
#include <VertexArray.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>

#include <RayTracer.h>

#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[])
{
    std::string scenePath = "scene1.txt";
    std::string outputPath = "render.png";
    if (argc >= 2) {
        scenePath = argv[1];
    }
    if (argc >= 3) {
        outputPath = argv[2];
    }

    // Allow running from the bin directory by falling back to parent path
    auto pathExists = [](const std::string& path) {
        std::ifstream f(path);
        return f.good();
    };

    if (!pathExists(scenePath)) {
        std::string parentCandidate = std::string("..") + "/" + scenePath;
        if (pathExists(parentCandidate)) {
            scenePath = parentCandidate;
        }
    }

    const int width = 1000;
    const int height = 1000;

    RayTracer tracer(width, height);
    if (!tracer.loadScene(scenePath)) {
        std::cerr << "Failed to load scene: " << scenePath << std::endl;
        return 1;
    }

    auto pixels = tracer.render();
    if (!tracer.writePNG(outputPath, pixels)) {
        return 1;
    }

    std::cout << "Rendered " << scenePath << " -> " << outputPath << std::endl;
    return 0;
}
