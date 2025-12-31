let lastTime = performance.now();
let fps = 0;
let frames = 0;
let fpsInterval = 0;

let paused = true

let timeAcc = 0.0;

function updateFPS() {
    const now = performance.now();
    frames++;

    // Measure every 500ms (half a second)
    if (now > lastTime + 500) {
        fps = Math.round((frames * 1000) / (now - lastTime));
        lastTime = now;
        frames = 0;
    }

    // Display to screen
    document.getElementById("fps-counter").textContent = fps + " FPS";
}



// ----- Helper to load shader files -----
async function loadShaderText(url) {
    const response = await fetch(url);
    return await response.text();
}

// ----- Create and compile a shader -----
function createShader(gl, type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        console.error("Shader compile error:", gl.getShaderInfoLog(shader));
        return null;
    }
    return shader;
}

// ----- Create a shader program -----
function createProgram(gl, vertexSrc, fragmentSrc) {
    const vertShader = createShader(gl, gl.VERTEX_SHADER, vertexSrc);
    const fragShader = createShader(gl, gl.FRAGMENT_SHADER, fragmentSrc);
    const program = gl.createProgram();
    gl.attachShader(program, vertShader);
    gl.attachShader(program, fragShader);
    gl.linkProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        console.error("Program link error:", gl.getProgramInfoLog(program));
        return null;
    }
    return program;
}

// ----- Fullscreen quad setup -----
function createFullscreenQuad(gl) {
    const quadVerts = new Float32Array([
        -1, -1,
         1, -1,
        -1,  1,
         1,  1
    ]);

    const vao = gl.createVertexArray();
    const vbo = gl.createBuffer();

    gl.bindVertexArray(vao);
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
    gl.bufferData(gl.ARRAY_BUFFER, quadVerts, gl.STATIC_DRAW);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
    gl.bindVertexArray(null);

    return vao;
}

const maxPitch = Math.PI / 2 -0.01

const keysPressed = {};

const camera = new Camera([0.0, 1.0, -3.0]);
const plane = new Plane([0.0, 1.0, 0.0], [0.0, -2.0, 0.0], [0.54, 0.6, 0.35]);
const lights = [
    new Light({
        position: [0.0, 10.0, 4.0],
        direction: [0.0, -1.0, 0.0],
        color: [0.8, 0.3, 6.0],
        shininess: 40.0,
        cutoff: 0.9
    }),

    new Light({
        position: [-4.0, 10.0, -2.0],
        direction: [0.0, -1.0, 0.0],
        color: [0.0, 1.0, 1.0],
        shininess: 16.0,
        cutoff: 0.9
        // color uses default [1,1,1]
    }),

    new Light({
        position: [4.0, 10.0, -2.0],
        direction: [0.0, -1.0, 0.0],
        color: [1, 0.722, 0.306],
        shininess: 64.0,
        cutoff: 0.9
        // color uses default [1,1,1]
    }),

    // new Light({
    //     position: [0.0, 3.0, -3.0],
    //     direction: [0.0, -1.0, -0.5],
    //     color: [0.5, 0.5, 0.5],
    //     shininess: 32.0
    //     // cutoff uses default -1.0
    //     // color uses default [1,1,1]
    // })
];


const spheres = [
    new Sphere([0.0, 0.0, 4.0], 2.0, [1.0, 0.0, 0.0], 0),
    new Sphere([4.0, 0.5, -2.0], 2.0, [0.0, 1.0, 0.0], 1),
    new Sphere([-4.0, 0.3, -2.0], 2.0, [0.0, 0.0, 1.0], 0),
    new Sphere([-30.0, 3.0, -2.0], 3.0, [0.0, 0.0, 1.0], 2),

    // new Sphere(camera.position, camera, [0.306, 0.824, 1]) // camera shpere
];

window.addEventListener("keydown", (e) => {
    keysPressed[e.code] = true;
})

window.addEventListener("keyup", (e) => {
    keysPressed[e.code] = false;
})




// ----- Main function -----
async function main() {
    let lightRotation = 0.0; // in radians
    
    const canvas = document.getElementById("glCanvas");
    const gl = canvas.getContext("webgl2");
    if (!gl) { alert("WebGL2 not supported!"); return; }
    
    // Load shaders from external files
    const vertexSrc = await loadShaderText("shaders/quad.vert");
    const fragmentSrc = await loadShaderText("shaders/raytracer.frag?no-cache=" + Math.random());
    
    const program = createProgram(gl, vertexSrc, fragmentSrc);
    gl.useProgram(program);
    
    // Get uniform location
    const uTimeLoc = gl.getUniformLocation(program, "uTime");
    const uResolutionLoc = gl.getUniformLocation(program, "uResolution");
    
    const uCamLocs = {
        pos: gl.getUniformLocation(program, 'cam.pos'),
        forward: gl.getUniformLocation(program, 'cam.forward'),
        right: gl.getUniformLocation(program, 'cam.right'),
        up: gl.getUniformLocation(program, 'cam.up')
    };



    canvas.addEventListener("click", () => {
        canvas.requestPointerLock();
    });

    document.addEventListener("pointerlockchange", () => {
        console.log("Pointer locked:", document.pointerLockElement === canvas);
    });

    document.addEventListener("mousemove", (e) => {
        if(document.pointerLockElement === canvas) {
            camera.rotate(e.movementX, e.movementY);
        }
    });

    function updateCam(dt) {
        let velocity = [0.0, 0.0, 0.0];
        if (keysPressed["KeyW"]) {
            velocity[2] += 3.0;
        }
        if (keysPressed["KeyS"]) {
            velocity[2] -= 3.0;
        }
        if (keysPressed["KeyA"]) {
            velocity[0] -= 3.0;
        }
        if (keysPressed["KeyD"]) {
            velocity[0] += 3.0;
        }
        if (keysPressed["Space"]) {
            velocity[1] += 3.0;
        }
        if (keysPressed["ShiftLeft"]) {
            velocity[1] -= 3.0;
        }
        const displacement = [velocity[0] * dt, velocity[1] * dt, velocity[2] * dt];

        camera.move(displacement);

        // Destructure for easier access
        const { pos, forward, right, up } = camera.getUniforms();

        // Update camera uniforms
        gl.useProgram(program);
        gl.uniform3fv(uCamLocs.pos, pos);
        gl.uniform3fv(uCamLocs.forward, forward);
        gl.uniform3fv(uCamLocs.right, right);
        gl.uniform3fv(uCamLocs.up, up);  
    }

    function updatePlane(dt) {
        const baseName = `uPlane`;
        const locs = {
            normal: gl.getUniformLocation(program, `${baseName}.normal`),
            point: gl.getUniformLocation(program, `${baseName}.point`),
            color: gl.getUniformLocation(program, `${baseName}.color`)
        };

        const { normal, point, color } = plane.getUniforms();
        gl.uniform3fv(locs.normal, normal);
        gl.uniform3fv(locs.point, point);
        gl.uniform3fv(locs.color, color);
    }
    updatePlane(0.016);

    function updateSpheres(dt) {
        gl.uniform1i(gl.getUniformLocation(program, 'uNumSpheres'), spheres.length);
        spheres.forEach((sphere, index) => {
            sphere.center[1] = sphere.startingHight + Math.sin(timeAcc * 0.1 + 1.2 * index) * 0.3;
            const baseName = `uSpheres[${index}]`;
            const locs = {
                center: gl.getUniformLocation(program, `${baseName}.center`),
                radius: gl.getUniformLocation(program, `${baseName}.radius`),
                color: gl.getUniformLocation(program, `${baseName}.color`),
                type: gl.getUniformLocation(program, `${baseName}.type`)
            };


            const { center, radius, color, type } = sphere.getUniforms();
            console.log("Updating sphere", index, "center:", center, "type:", type);
            gl.uniform3fv(locs.center, center);
            gl.uniform1f(locs.radius, radius);
            gl.uniform3fv(locs.color, color);
            gl.uniform1i(locs.type, type);
        });
    }

    updateSpheres(0.016);

    function updateLight(dt) {


        gl.uniform1i(gl.getUniformLocation(program, 'uNumLights'), lights.length);
        lights.forEach((light, index) => {
            vec3.rotateY(light.position, light.position, [0,0,0], 0.5 * dt);
            const baseName = `uLights[${index}]`;
            const locs = {
                position: gl.getUniformLocation(program, `${baseName}.position`),
                direction: gl.getUniformLocation(program, `${baseName}.direction`),
                color: gl.getUniformLocation(program, `${baseName}.color`),
                shininess: gl.getUniformLocation(program, `${baseName}.shininess`),
                cutoff: gl.getUniformLocation(program, `${baseName}.cutoff`)
            };


            const { position, direction, color, shininess, cutoff } = light.getUniforms();

            // console.log("position:", position);
            // console.log("direction:", direction);
            // console.log("color:", color);

            gl.uniform3fv(locs.position, position);
            gl.uniform3fv(locs.direction, direction);
            gl.uniform3fv(locs.color, color);
            gl.uniform1f(locs.shininess, shininess);
            gl.uniform1f(locs.cutoff, cutoff);
        });
    }
    updateLight(0.016);


    // Resize canvas function
    function resizeCanvas() {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        gl.viewport(0, 0, canvas.width, canvas.height);

        // Update resolution uniform
        gl.useProgram(program);
        gl.uniform2iv(uResolutionLoc, [canvas.width, canvas.height]);
    }

    window.addEventListener("resize", resizeCanvas);
    resizeCanvas();


    // Setup fullscreen quad
    const quadVAO = createFullscreenQuad(gl);

    // Render loop
    function render(time) {
        
        time *= 0.001; // convert to seconds
        updateFPS();
        keysPressed["KeyP"] ? paused = !paused : null;


        updateCam(0.016); // assuming ~60fps, for simplicity
        if (!paused) {
            timeAcc++;
            updateSpheres(0.016);
            updateLight(0.016);
        }
        // updatePlane(0.016);

        gl.clearColor(0, 0, 0.5, 1);
        gl.clear(gl.COLOR_BUFFER_BIT);

        gl.useProgram(program);
        gl.uniform1f(uTimeLoc, time);

        gl.bindVertexArray(quadVAO);
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        gl.bindVertexArray(null);

        requestAnimationFrame(render);
    }

    requestAnimationFrame(render);
}

// Run
main();
