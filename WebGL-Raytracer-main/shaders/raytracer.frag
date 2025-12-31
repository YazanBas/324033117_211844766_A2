#version 300 es
precision highp float;
precision highp int;

struct Camera {
    vec3 pos;
    vec3 forward;
    vec3 right;
    vec3 up;
};

struct Plane {
    vec3 point;
    vec3 normal;
    vec3 color;
};

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
    int type; // 0: opaque, 1: reflective, 2: refractive
};

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float shininess;
    float cutoff; // if > 0.0 then spotlight else directional light
};

struct HitInfo {
    vec3 rayOrigin;
    vec3 rayDir;
    float t;
    vec3 baseColor;
    int inside; // 1 if inside the sphere, 0 otherwise
    vec3 hitPoint;
    vec3 normal;
    int type; // 0: diffuse, 1: reflective
};

const int TYPE_DIFFUSE = 0;
const int TYPE_REFLECTIVE = 1;
const int TYPE_REFRACTIVE = 2;

const int MAX_SPHERES = 16;
const int MAX_LIGHTS = 4;
const int MAX_DEPTH = 5;

in vec2 vUV;
out vec4 FragColor;

uniform float uTime;
uniform ivec2 uResolution; // width and height of canvas


uniform Camera cam;
uniform Sphere uSpheres[MAX_SPHERES];
uniform int uNumSpheres;

uniform Light uLights[MAX_LIGHTS];
uniform int uNumLights;

uniform Plane uPlane;

vec3 checkerboardColor(vec3 rgbColor, vec3 hitPoint) {
    // Checkerboard pattern
    float scaleParameter = 2.0;
    float checkerboard = 0.0;
    if (hitPoint.x < 0.0) {
    checkerboard += floor((0.5 - hitPoint.x) / scaleParameter);
    }
    else {
    checkerboard += floor(hitPoint.x / scaleParameter);
    }
    if (hitPoint.z < 0.0) {
    checkerboard += floor((0.5 - hitPoint.z) / scaleParameter);
    }
    else {
    checkerboard += floor(hitPoint.z / scaleParameter);
    }
    checkerboard = (checkerboard * 0.5) - float(int(checkerboard * 0.5));
    checkerboard *= 2.0;
    if (checkerboard > 0.5) {
    return 0.5 * rgbColor;
    }
    return rgbColor;
}

// replace void to your hit data type
/* intersects scene. gets ray origin and direction, returns hit data*/
HitInfo intersectScene(vec3 rayOrigin, vec3 rayDir) {
    HitInfo hit;
    hit.t = 1e20;
    hit.type = -1;
    hit.rayOrigin = rayOrigin;
    hit.rayDir = rayDir;
    hit.inside = 0;

    // Plane
    float denom = dot(uPlane.normal, rayDir);
    if (abs(denom) > 1e-6) {
        float t = dot(uPlane.point - rayOrigin, uPlane.normal) / denom;
        if (t > 1e-3 && t < hit.t) {
            hit.t = t;
            hit.hitPoint = rayOrigin + rayDir * t;
            hit.normal = normalize(uPlane.normal);
            hit.baseColor = checkerboardColor(uPlane.color, hit.hitPoint);
            hit.type = TYPE_DIFFUSE;
            hit.inside = 0;
        }
    }

    // Spheres
    for (int i = 0; i < uNumSpheres; ++i) {
        Sphere s = uSpheres[i];
        vec3 oc = rayOrigin - s.center;
        float a = dot(rayDir, rayDir);
        float b = 2.0 * dot(oc, rayDir);
        float c = dot(oc, oc) - s.radius * s.radius;
        float disc = b * b - 4.0 * a * c;
        if (disc < 0.0) continue;
        float sqrtD = sqrt(disc);
        float t0 = (-b - sqrtD) / (2.0 * a);
        float t1 = (-b + sqrtD) / (2.0 * a);
        float t = t0;
        if (t < 1e-3) t = t1;
        if (t < 1e-3) continue;
        if (t < hit.t) {
            vec3 p = rayOrigin + t * rayDir;
            vec3 n = normalize(p - s.center);
            int inside = 0;
            if (dot(rayDir, n) > 0.0) {
                n = -n;
                inside = 1;
            }
            hit.t = t;
            hit.hitPoint = p;
            hit.normal = n;
            hit.baseColor = s.color;
            hit.type = s.type;
            hit.inside = inside;
        }
    }

    return hit;

}

// replace int to your hit data type
/* calculates color based on hit data and uv coordinates */
vec3 calcColor(HitInfo firstHit) {
    vec3 color = vec3(0.0);
    vec3 throughput = vec3(1.0);
    vec3 origin = firstHit.rayOrigin;
    vec3 dir = firstHit.rayDir;
    const vec3 background = vec3(0.0);
    const float airIor = 1.0;
    const float glassIor = 1.5;

    for (int depth = 0; depth < MAX_DEPTH; ++depth) {
        HitInfo hit = intersectScene(origin, dir);
        if (hit.type < 0 || hit.t > 1e19) {
            color += throughput * background;
            break;
        }

        // Diffuse shading
        if (hit.type == TYPE_DIFFUSE) {
            vec3 N = normalize(hit.normal);
            vec3 V = normalize(-dir);

            vec3 baseColor = hit.baseColor;
            vec3 shaded = baseColor * 0.05; // ambient
            for (int i = 0; i < uNumLights; ++i) {
                Light Lgt = uLights[i];
                vec3 L;
                float maxDist = 1e20;
                if (Lgt.cutoff > 0.0) {
                    vec3 toLight = Lgt.position - hit.hitPoint;
                    maxDist = length(toLight);
                    if (maxDist <= 0.0) continue;
                    L = toLight / maxDist;
                    float spotCos = dot(normalize(Lgt.direction), -L);
                    if (spotCos < Lgt.cutoff) continue;
                } else {
                    L = normalize(-Lgt.direction);
                }

                // Shadow test
                bool shadow = false;
                // Plane shadow
                float denomS = dot(uPlane.normal, L);
                if (abs(denomS) > 1e-6) {
                    float tS = dot(uPlane.point - (hit.hitPoint + L * 1e-3), uPlane.normal) / denomS;
                    if (tS > 1e-3 && tS < maxDist) shadow = true;
                }
                // Sphere shadows
                for (int s = 0; s < uNumSpheres && !shadow; ++s) {
                    Sphere sp = uSpheres[s];
                    vec3 oc = (hit.hitPoint + L * 1e-3) - sp.center;
                    float a = dot(L, L);
                    float b = 2.0 * dot(oc, L);
                    float c = dot(oc, oc) - sp.radius * sp.radius;
                    float disc = b * b - 4.0 * a * c;
                    if (disc < 0.0) continue;
                    float sqrtD = sqrt(disc);
                    float t0 = (-b - sqrtD) / (2.0 * a);
                    float t1 = (-b + sqrtD) / (2.0 * a);
                    float t = t0;
                    if (t < 1e-3) t = t1;
                    if (t > 1e-3 && t < maxDist) {
                        shadow = true;
                    }
                }
                if (shadow) continue;

                float diff = max(dot(N, L), 0.0);
                vec3 diffuse = baseColor * Lgt.color * diff;

                vec3 R = reflect(-L, N);
                float spec = pow(max(dot(V, R), 0.0), Lgt.shininess);
                vec3 specular = vec3(0.7) * Lgt.color * spec;

                shaded += diffuse + specular;
            }

            color += throughput * shaded;
            break;
        }

        // Reflective
        if (hit.type == TYPE_REFLECTIVE) {
            vec3 reflectDir = reflect(dir, normalize(hit.normal));
            origin = hit.hitPoint + reflectDir * 1e-3;
            dir = normalize(reflectDir);
            continue;
        }

        // Refractive
        if (hit.type == TYPE_REFRACTIVE) {
            vec3 N = normalize(hit.normal);
            float eta = hit.inside == 1 ? (glassIor / airIor) : (airIor / glassIor);
            float cosi = clamp(dot(-dir, N), -1.0, 1.0);
            float k = 1.0 - eta * eta * (1.0 - cosi * cosi);
            if (k < 0.0) {
                vec3 reflectDir = reflect(dir, N);
                origin = hit.hitPoint + reflectDir * 1e-3;
                dir = normalize(reflectDir);
            } else {
                vec3 refractDir = normalize(eta * dir + (eta * cosi - sqrt(k)) * N);
                origin = hit.hitPoint + refractDir * 1e-3;
                dir = refractDir;
            }
            continue;
        }
    }

    return color;
}

/* scales UV coordinates based on resolution
 * uv given uv are [0, 1] range
 * returns new coordinates where y range [-1, 1] and x scales according to window resolution
 */
vec2 scaleUV(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    float aspect = float(uResolution.x) / float(uResolution.y);
    p.x *= aspect;
    return p;
}

void main() {
    vec2 uv = scaleUV(vUV);
    vec3 rayDir = normalize(cam.forward + uv.x * cam.right + uv.y * cam.up);

    HitInfo hitInfo = intersectScene(cam.pos, rayDir);
    hitInfo.rayOrigin = cam.pos;
    hitInfo.rayDir = rayDir;

    vec3 color = calcColor(hitInfo);

    FragColor = vec4(color, 1.0);
}

