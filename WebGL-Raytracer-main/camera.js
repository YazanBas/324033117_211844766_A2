const { vec3, quat } = glMatrix;

class Camera {
    constructor(position = [0.0, 0.0, -3.0]) {
        this.position = vec3.fromValues(...position);
        this.orientation = quat.create(); // identity quaternion
        this.forward = vec3.create();
        this.right = vec3.create();
        this.up = vec3.create();

        this.worldUp = vec3.fromValues(0.0, 1.0, 0.0);

        this.updateBasis();
    }

    updateBasis() {
        vec3.transformQuat(this.forward, [0.0, 0.0, 1.0], this.orientation);
        vec3.normalize(this.forward, this.forward);

        vec3.transformQuat(this.right, [1.0, 0.0, 0.0], this.orientation);
        vec3.normalize(this.right, this.right);

        vec3.transformQuat(this.up, [0.0, 1.0, 0.0], this.orientation);
        vec3.normalize(this.up, this.up);
    }


    move(displacement) {
        const tmp = vec3.create();
        vec3.scaleAndAdd(tmp, this.position, this.right, displacement[0]);
        vec3.scaleAndAdd(tmp, tmp, this.up, displacement[1]);
        vec3.scaleAndAdd(this.position, tmp, this.forward, displacement[2]);
    }

    rotate(deltaX, deltaY, sensitivity=0.003) {
        const qYaw = quat.create();
        quat.setAxisAngle(qYaw, this.worldUp, deltaX * sensitivity);

        const qPitch = quat.create();
        quat.setAxisAngle(qPitch, this.right, deltaY * sensitivity);

        const qDelta = quat.create();
        quat.multiply(qDelta, qYaw, qPitch);
        quat.multiply(this.orientation, qDelta, this.orientation);

        quat.normalize(this.orientation, this.orientation);

        this.updateBasis();
    }


    getUniforms() {
        return {
            pos: this.position,
            forward: this.forward,
            right: this.right,
            up: this.up
        }
    }
}