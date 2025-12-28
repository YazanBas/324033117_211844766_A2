#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

enum class ObjectType {
    Opaque,
    Reflective,
    Transparent
};

struct Material {
    glm::vec3 ambient{0.0f};
    glm::vec3 diffuse{0.0f};
    glm::vec3 specular{0.7f, 0.7f, 0.7f};
    float shininess{1.0f};
    ObjectType type{ObjectType::Opaque};
};

struct HitInfo {
    float t{0.0f};
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    const class Object* object{nullptr};
    Material material{};
    bool hit{false};
};

class Object {
  public:
    explicit Object(const Material& mat) : m_Material(mat) {}
    virtual ~Object() = default;

    const Material& material() const { return m_Material; }
    void setMaterial(const Material& mat) { m_Material = mat; }

    virtual bool intersect(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const = 0;
    virtual glm::vec3 colorAt(const glm::vec3& point) const { return m_Material.diffuse; }

  protected:
    Material m_Material;
};

class Sphere : public Object {
  public:
    Sphere(const glm::vec3& c, float r, const Material& mat);
    bool intersect(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const override;

  private:
    glm::vec3 m_Center;
    float m_Radius;
};

class Plane : public Object {
  public:
    Plane(const glm::vec3& normal, float d, const Material& mat);
    bool intersect(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const override;
    glm::vec3 colorAt(const glm::vec3& point) const override;

  private:
    glm::vec3 m_Normal;
    float m_D;  // normalized plane coefficient
};

struct Light {
    glm::vec3 direction{0.0f};  // normalized. For directional lights, points from light to scene.
    glm::vec3 position{0.0f};   // used only for spotlights
    glm::vec3 intensity{0.0f};
    bool isSpot{false};
    float cutoff{0.0f};         // cosine of cutoff angle for spotlights
};

struct CameraParams {
    glm::vec3 eye{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    float screenDistance{1.0f};
    float screenWidth{2.0f};
    float screenHeight{2.0f};
};

struct Scene {
    CameraParams camera{};
    glm::vec3 ambient{0.0f};
    std::vector<Light> lights;
    std::vector<std::unique_ptr<Object>> objects;
};

class RayTracer {
  public:
    RayTracer(int width, int height);

    bool loadScene(const std::string& path);
    std::vector<unsigned char> render();
    bool writePNG(const std::string& path, const std::vector<unsigned char>& pixels) const;

  private:
    bool closestHit(const Ray& ray, float tMin, float tMax, HitInfo& outHit) const;
    bool isShadowed(const glm::vec3& origin, const glm::vec3& dir, float maxDist, const Object* ignore) const;
    glm::vec3 trace(const Ray& ray, int depth) const;
    glm::vec3 shade(const HitInfo& hit, const Ray& ray, int depth) const;
    glm::vec3 handleTransparency(const HitInfo& hit, const Ray& ray, int depth) const;

  private:
    Scene m_Scene{};
    int m_Width;
    int m_Height;
    int m_MaxDepth{5};
    float m_Epsilon{1e-4f};
};
