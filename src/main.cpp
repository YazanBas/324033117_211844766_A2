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
#include <exception>
#include <vector>
#include <set>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

// Discover scene definition files so the user can choose one interactively.
std::vector<std::string> discoverSceneFiles()
{
    std::vector<std::string> scenes;
    std::set<std::string> seen;
    std::vector<std::string> searchRoots = {".", ".."};

    for (const auto& root : searchRoots) {
        DIR* dir = opendir(root.c_str());
        if (!dir) {
            continue;
        }

        while (dirent* entry = readdir(dir)) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") {
                continue;
            }

            std::string path = root + "/" + name;
            struct stat st{};
            if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
                continue;
            }

            const size_t extLen = 4; // ".txt"
            if (name.size() <= extLen || name.compare(name.size() - extLen, extLen, ".txt") != 0) {
                continue;
            }

            std::string stem = name.substr(0, name.size() - extLen);
            if (stem.rfind("scene", 0) != 0) {
                continue;
            }

            if (seen.insert(path).second) {
                scenes.push_back(path);
            }
        }

        closedir(dir);
    }

    std::sort(scenes.begin(), scenes.end());
    return scenes;
}

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

    if (argc < 2) {
        auto scenes = discoverSceneFiles();
        if (!scenes.empty()) {
            std::cout << "Available scenes:\n";
            for (size_t i = 0; i < scenes.size(); ++i) {
                std::cout << "  [" << i + 1 << "] " << scenes[i] << "\n";
            }
            std::cout << "Pick a scene number or enter a path (empty to exit): " << std::flush;

            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) {
                std::cout << "No selection made. Exiting without rendering.\n";
                return 0;
            }

            try {
                size_t idxEnd = 0;
                int idx = std::stoi(input, &idxEnd);
                if (idxEnd == input.size() && idx >= 1 && static_cast<size_t>(idx) <= scenes.size()) {
                    scenePath = scenes[static_cast<size_t>(idx) - 1];
                } else {
                    scenePath = input;
                }
            } catch (const std::exception&) {
                scenePath = input;
            }
        }
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
