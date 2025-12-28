#include <RayTracer.h>
#include <stb/stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

namespace {
    constexpr float kAirRefractiveIndex = 1.0f;
    constexpr float kGlassRefractiveIndex = 1.5f;
    constexpr float kMaxDistance = std::numeric_limits<float>::infinity();

    glm::vec3 clampColor(const glm::vec3& c)
    {
        return glm::clamp(c, glm::vec3(0.0f), glm::vec3(1.0f));
    }
}

Sphere::Sphere(const glm::vec3& c, float r, const Material& mat)
    : Object(mat), m_Center(c), m_Radius(r) {}

bool Sphere::intersect(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const
{
    glm::vec3 oc = ray.origin - m_Center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - m_Radius * m_Radius;
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    float sqrtD = std::sqrt(discriminant);
    float t = (-b - sqrtD) / (2.0f * a);
    if (t < tMin || t > tMax) {
        t = (-b + sqrtD) / (2.0f * a);
        if (t < tMin || t > tMax) {
            return false;
        }
    }

    outHit.t = t;
    outHit.point = ray.origin + t * ray.direction;
    outHit.normal = glm::normalize(outHit.point - m_Center);
    outHit.material = m_Material;
    outHit.object = this;
    outHit.hit = true;
    return true;
}

Plane::Plane(const glm::vec3& normal, float d, const Material& mat)
    : Object(mat)
{
    float len = glm::length(normal);
    if (len == 0.0f) {
        m_Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        m_D = d;
    } else {
        m_Normal = glm::normalize(normal);
        m_D = d / len;  // keep plane equation consistent after normalization
    }
}

bool Plane::intersect(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const
{
    float denom = glm::dot(m_Normal, ray.direction);
    if (std::abs(denom) < 1e-6f) {
        return false;  // Parallel
    }

    float t = -(glm::dot(m_Normal, ray.origin) + m_D) / denom;
    if (t < tMin || t > tMax) {
        return false;
    }

    outHit.t = t;
    outHit.point = ray.origin + t * ray.direction;
    outHit.normal = m_Normal;
    outHit.material = m_Material;
    outHit.object = this;
    outHit.hit = true;
    return true;
}

glm::vec3 Plane::colorAt(const glm::vec3& point) const
{
    // Checkerboard pattern projected on the XY plane
    glm::vec3 rgbColor = m_Material.diffuse;
    float scaleParameter = 0.5f;
    float checkerboard = 0.0f;
    if (point.x < 0.0f) {
        checkerboard += std::floor((0.5f - point.x) / scaleParameter);
    } else {
        checkerboard += std::floor(point.x / scaleParameter);
    }

    if (point.y < 0.0f) {
        checkerboard += std::floor((0.5f - point.y) / scaleParameter);
    } else {
        checkerboard += std::floor(point.y / scaleParameter);
    }

    checkerboard = (checkerboard * 0.5f) - int(checkerboard * 0.5f);
    checkerboard *= 2.0f;
    if (checkerboard > 0.5f) {
        return 0.5f * rgbColor;
    }
    return rgbColor;
}

RayTracer::RayTracer(int width, int height)
    : m_Width(width), m_Height(height)
{}

bool RayTracer::loadScene(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open scene file: " << path << std::endl;
        return false;
    }

    m_Scene = Scene{};

    struct PendingLight {
        Light light;
        bool hasPosition{false};
        bool hasIntensity{false};
    };
    std::vector<PendingLight> pendingLights;
    std::vector<Object*> objectOrder;

    std::string tag;
    while (in >> tag) {
        if (tag == "e") {
            in >> m_Scene.camera.eye.x >> m_Scene.camera.eye.y >> m_Scene.camera.eye.z >> m_Scene.camera.screenDistance;
        } else if (tag == "u") {
            in >> m_Scene.camera.up.x >> m_Scene.camera.up.y >> m_Scene.camera.up.z >> m_Scene.camera.screenHeight;
        } else if (tag == "f") {
            in >> m_Scene.camera.forward.x >> m_Scene.camera.forward.y >> m_Scene.camera.forward.z >> m_Scene.camera.screenWidth;
        } else if (tag == "a") {
            in >> m_Scene.ambient.r >> m_Scene.ambient.g >> m_Scene.ambient.b;
            float ignore;
            in >> ignore;
        } else if (tag == "d") {
            Light l{};
            in >> l.direction.x >> l.direction.y >> l.direction.z;
            float typeFlag;
            in >> typeFlag;
            l.direction = glm::normalize(l.direction);
            l.isSpot = typeFlag > 0.5f;
            l.cutoff = 0.0f;
            pendingLights.push_back({l, false, false});
        } else if (tag == "p") {
            glm::vec3 pos;
            float cutoff = 0.0f;
            in >> pos.x >> pos.y >> pos.z >> cutoff;
            for (auto& pl : pendingLights) {
                if (pl.light.isSpot && !pl.hasPosition) {
                    pl.light.position = pos;
                    pl.light.cutoff = cutoff;
                    pl.hasPosition = true;
                    break;
                }
            }
        } else if (tag == "i") {
            glm::vec3 intensity;
            float ignore = 0.0f;
            in >> intensity.r >> intensity.g >> intensity.b >> ignore;
            for (auto& pl : pendingLights) {
                if (!pl.hasIntensity) {
                    pl.light.intensity = intensity;
                    pl.hasIntensity = true;
                    break;
                }
            }
        } else if (tag == "o" || tag == "r" || tag == "t") {
            float a, b, c, d;
            in >> a >> b >> c >> d;
            ObjectType type = ObjectType::Opaque;
            if (tag == "r") {
                type = ObjectType::Reflective;
            } else if (tag == "t") {
                type = ObjectType::Transparent;
            }
            Material mat{};
            mat.type = type;
            // Reflective and transparent objects ignore ambient/diffuse
            bool isSphere = d > 0.0f;
            if (isSphere) {
                m_Scene.objects.push_back(std::make_unique<Sphere>(glm::vec3(a, b, c), d, mat));
            } else {
                m_Scene.objects.push_back(std::make_unique<Plane>(glm::vec3(a, b, c), d, mat));
            }
            objectOrder.push_back(m_Scene.objects.back().get());
        } else if (tag == "c") {
            glm::vec3 color;
            float shininess = 1.0f;
            in >> color.r >> color.g >> color.b >> shininess;
            if (!objectOrder.empty()) {
                Object* obj = objectOrder.front();
                objectOrder.erase(objectOrder.begin());
                Material mat = obj->material();
                mat.ambient = color;
                mat.diffuse = color;
                mat.shininess = shininess;
                obj->setMaterial(mat);
            }
        } else {
            std::string line;
            std::getline(in, line); // skip unknown tag
        }
    }

    // Finalize lights (fill defaults if needed)
    m_Scene.lights.clear();
    for (auto& pl : pendingLights) {
        m_Scene.lights.push_back(pl.light);
    }
    return true;
}

bool RayTracer::closestHit(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const
{
    HitInfo closest{};
    closest.t = tMax;
    bool hitSomething = false;

    for (const auto& obj : m_Scene.objects) {
        HitInfo temp{};
        if (obj->intersect(ray, tMin, closest.t, temp)) {
            hitSomething = true;
            closest = temp;
        }
    }

    if (hitSomething) {
        outHit = closest;
    }
    return hitSomething;
}

bool RayTracer::isShadowed(const glm::vec3& origin, const glm::vec3& dir, float maxDist, const Object* ignore) const
{
    Ray shadowRay{origin + dir * m_Epsilon, dir};
    for (const auto& obj : m_Scene.objects) {
        if (obj.get() == ignore) {
            continue;
        }
        HitInfo hit{};
        if (obj->intersect(shadowRay, m_Epsilon, maxDist, hit)) {
            return true;
        }
    }
    return false;
}

glm::vec3 RayTracer::shade(const HitInfo& hit, const Ray& ray, int depth) const
{
    const Material& mat = hit.material;
    glm::vec3 normal = hit.normal;
    if (glm::dot(ray.direction, normal) > 0.0f) {
        normal = -normal;
    }

    glm::vec3 baseColor = hit.object ? hit.object->colorAt(hit.point) : mat.diffuse;
    glm::vec3 result = mat.ambient * m_Scene.ambient;

    glm::vec3 viewDir = glm::normalize(m_Scene.camera.eye - hit.point);

    for (const auto& light : m_Scene.lights) {
        glm::vec3 L;
        float maxDist = kMaxDistance;
        if (light.isSpot) {
            glm::vec3 toLight = light.position - hit.point;
            maxDist = glm::length(toLight);
            if (maxDist <= 0.0f) {
                continue;
            }
            L = toLight / maxDist;
            float spotCos = glm::dot(glm::normalize(light.direction), -L);
            if (spotCos < light.cutoff) {
                continue;
            }
        } else {
            // Directional light direction points from light toward the scene
            L = glm::normalize(-light.direction);
        }

        if (isShadowed(hit.point, L, maxDist - m_Epsilon, hit.object)) {
            continue;
        }

        float diff = std::max(glm::dot(normal, L), 0.0f);
        glm::vec3 diffuse = baseColor * light.intensity * diff;

        glm::vec3 reflectDir = glm::reflect(-L, normal);
        float spec = std::pow(std::max(glm::dot(viewDir, reflectDir), 0.0f), mat.shininess);
        glm::vec3 specular = mat.specular * light.intensity * spec;

        result += diffuse + specular;
    }

    return clampColor(result);
}

glm::vec3 RayTracer::handleTransparency(const HitInfo& hit, const Ray& ray, int depth) const
{
    glm::vec3 normal = hit.normal;
    bool outside = glm::dot(ray.direction, normal) < 0.0f;
    glm::vec3 n = outside ? normal : -normal;
    float eta = outside ? (kAirRefractiveIndex / kGlassRefractiveIndex) : (kGlassRefractiveIndex / kAirRefractiveIndex);

    glm::vec3 refractDir = glm::refract(glm::normalize(ray.direction), n, eta);
    if (glm::dot(refractDir, refractDir) < 1e-6f) {
        // Total internal reflection
        glm::vec3 reflectDir = glm::reflect(ray.direction, n);
        return trace({hit.point + reflectDir * m_Epsilon, glm::normalize(reflectDir)}, depth + 1);
    }

    Ray insideRay{hit.point + refractDir * m_Epsilon, glm::normalize(refractDir)};

    // Advance until exiting the sphere
    const Sphere* sphere = dynamic_cast<const Sphere*>(hit.object);
    HitInfo exitHit{};
    if (sphere && sphere->intersect(insideRay, m_Epsilon, kMaxDistance, exitHit)) {
        glm::vec3 exitNormal = exitHit.normal;
        if (glm::dot(insideRay.direction, exitNormal) > 0.0f) {
            exitNormal = -exitNormal;
        }
        glm::vec3 refractOutDir = glm::refract(insideRay.direction, exitNormal, kGlassRefractiveIndex / kAirRefractiveIndex);
        if (glm::dot(refractOutDir, refractOutDir) < 1e-6f) {
            refractOutDir = glm::reflect(insideRay.direction, exitNormal);
        }
        Ray outRay{exitHit.point + refractOutDir * m_Epsilon, glm::normalize(refractOutDir)};
        return trace(outRay, depth + 1);
    }

    return trace(insideRay, depth + 1);
}

glm::vec3 RayTracer::trace(const Ray& ray, int depth) const
{
    if (depth > m_MaxDepth) {
        return glm::vec3(0.0f);
    }

    HitInfo hit{};
    if (!closestHit(ray, m_Epsilon, kMaxDistance, hit)) {
        return glm::vec3(0.0f);  // background
    }

    switch (hit.material.type) {
        case ObjectType::Reflective: {
            glm::vec3 normal = hit.normal;
            if (glm::dot(ray.direction, normal) > 0.0f) {
                normal = -normal;
            }
            glm::vec3 reflectDir = glm::reflect(ray.direction, normal);
            return trace({hit.point + reflectDir * m_Epsilon, glm::normalize(reflectDir)}, depth + 1);
        }
        case ObjectType::Transparent:
            return handleTransparency(hit, ray, depth);
        case ObjectType::Opaque:
        default:
            return shade(hit, ray, depth);
    }
}

std::vector<unsigned char> RayTracer::render()
{
    std::vector<unsigned char> pixels(static_cast<size_t>(m_Width) * m_Height * 3, 0);

    glm::vec3 forward = glm::normalize(m_Scene.camera.forward);
    glm::vec3 right = glm::normalize(glm::cross(forward, m_Scene.camera.up));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    glm::vec3 screenCenter = m_Scene.camera.eye + forward * m_Scene.camera.screenDistance;

    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            float px = ((static_cast<float>(x) + 0.5f) / static_cast<float>(m_Width) - 0.5f) * m_Scene.camera.screenWidth;
            float py = (0.5f - (static_cast<float>(y) + 0.5f) / static_cast<float>(m_Height)) * m_Scene.camera.screenHeight;

            glm::vec3 pixelPos = screenCenter + right * px + up * py;
            glm::vec3 dir = glm::normalize(pixelPos - m_Scene.camera.eye);
            glm::vec3 color = trace({m_Scene.camera.eye, dir}, 0);
            color = clampColor(color);

            size_t idx = (static_cast<size_t>(y) * m_Width + x) * 3;
            pixels[idx + 0] = static_cast<unsigned char>(color.r * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(color.g * 255.0f);
            pixels[idx + 2] = static_cast<unsigned char>(color.b * 255.0f);
        }
    }

    return pixels;
}

bool RayTracer::writePNG(const std::string& path, const std::vector<unsigned char>& pixels) const
{
    int stride = m_Width * 3;
    int success = stbi_write_png(path.c_str(), m_Width, m_Height, 3, pixels.data(), stride);
    if (!success) {
        std::cerr << "Failed to write image: " << path << std::endl;
        return false;
    }
    return true;
}
