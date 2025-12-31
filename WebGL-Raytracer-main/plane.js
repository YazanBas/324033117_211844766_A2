class Plane {
    constructor(normal, point, color) {
        this.normal = normal;
        this.point = point;
        this.color = color;
    }

    getUniforms() {
        return {
            normal: this.normal,
            point: this.point,
            color: this.color
        }
    }
}