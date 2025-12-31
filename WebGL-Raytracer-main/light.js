class Light {
    constructor({
            position,
            direction,
            color = [1.0, 1.0, 1.0],
            shininess = 32.0,
            cutoff = -1.0
        }) {
        if (!position) throw new Error("Light requires 'position'");
        if (!direction) throw new Error("Light requires 'direction'");

        this.position = new Float32Array(position);
        this.direction = glMatrix.vec3.normalize([], direction);
        this.color = new Float32Array(color);
        this.shininess = shininess;
        this.cutoff = cutoff;
    }

    getUniforms() {
        return {
            position: this.position,
            direction: this.direction,
            color: this.color,
            shininess: this.shininess,
            cutoff: this.cutoff
        }
    }
}