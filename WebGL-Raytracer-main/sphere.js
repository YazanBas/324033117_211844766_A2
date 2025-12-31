class Sphere {
    constructor(center, radius, color, type) {
        this.center = center;
        this.startingHight = center[1];
        this.radius = radius;
        this.color = color;
        this.type = type; // 0: diffuse, 1: reflective 2: refractive
    }

    getUniforms() {
        return {
            center: this.center,
            radius: this.radius,
            color: this.color,
            type: this.type
        }
    }
}