const char *html_page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Head-Tilt Racing</title>
    <!-- Load TensorFlow.js and Face Landmarks Detection from CDN -->
    <script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs@3.21.0/dist/tf.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/@tensorflow-models/face-landmarks-detection@0.0.3/dist/face-landmarks-detection.js"></script>

    <style>
        body {
            margin: 0;
            padding: 20px;
            background-color: #111;
            color: #fff;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }

        h1 { margin-bottom: 20px; }

        .main-layout {
            display: flex;
            flex-wrap: wrap;
            gap: 20px;
            justify-content: center;
            align-items: flex-start;
        }

        .panel {
            display: flex;
            flex-direction: column;
            align-items: center;
            background: #222;
            padding: 10px;
            border-radius: 10px;
            border: 2px solid #444;
            box-shadow: 0 0 20px rgba(0,0,0,0.5);
        }

        .panel h2 {
            margin: 0 0 10px 0;
            font-size: 18px;
            color: #ccc;
        }

        .canvas-wrapper {
            position: relative;
            width: 480px; /* Scaled down visually to fit side-by-side */
            height: 360px;
        }

        canvas {
            width: 100%;
            height: 100%;
            background: #000;
            border-radius: 4px;
        }

        /* Game Overlays */
        #ui-layer {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            padding: 10px;
            box-sizing: border-box;
        }

        .hud-top {
            display: flex;
            justify-content: space-between;
            font-size: 20px;
            font-weight: bold;
            text-shadow: 2px 2px 0 #000;
        }

        .hud-bottom {
            text-align: center;
            font-size: 16px;
            text-shadow: 1px 1px 0 #000;
            color: #aaa;
        }

        #status-msg {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(0, 0, 0, 0.85);
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            pointer-events: auto;
            z-index: 10;
            width: 80%;
        }

        button {
            background: #00d2ff;
            color: #000;
            border: none;
            padding: 8px 16px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            border-radius: 5px;
            margin-top: 10px;
        }
        button:hover { background: #33e0ff; }
        button:disabled { background: #555; cursor: not-allowed; }

        .controls {
            margin-top: 20px;
            display: flex;
            gap: 20px;
            align-items: center;
            background: #222;
            padding: 15px;
            border-radius: 10px;
            border: 1px solid #444;
        }
        
        label { font-size: 14px; color: #ccc; display: flex; align-items: center; gap: 5px; }
        input[type=range] { width: 100px; }
        input[type=text] { padding: 5px; border-radius: 4px; border: 1px solid #555; background: #333; color: #fff; }

    </style>
</head>
<body>

    <h1>Head-Tilt Racing</h1>

    <div class="main-layout">
        
        <!-- LEFT PANEL: CAMERA FEED -->
        <div class="panel">
            <h2>Camera Feed & Tracking</h2>
            <div class="canvas-wrapper">
                <canvas id="videoCanvas" width="640" height="480"></canvas>
                <div id="video-overlay" style="position:absolute; bottom:10px; left:0; width:100%; text-align:center; color:#0f0; font-weight:bold; text-shadow:1px 1px 0 #000;">
                    <span id="tilt-display">Tilt: 0&deg;</span>
                </div>
            </div>
        </div>

        <!-- RIGHT PANEL: GAME -->
        <div class="panel">
            <h2>Game View</h2>
            <div class="canvas-wrapper">
                <canvas id="gameCanvas" width="640" height="480"></canvas>
                
                <div id="ui-layer">
                    <div class="hud-top">
                        <div id="score-display">Score: 0</div>
                    </div>
                    <div class="hud-bottom">
                        Tilt Head Left/Right to Steer
                    </div>
                </div>

                <!-- Start/Status Overlay -->
                <div id="status-msg">
                    <h3 id="status-title">Loading AI...</h3>
                    <p id="status-text">Please wait.</p>
                    <button id="start-btn" disabled onclick="startGame()">Start Game</button>
                </div>
            </div>
        </div>

    </div>

    <div class="controls">
        <label>
            Camera IP: <input type="text" id="cam-ip" value="" placeholder="e.g. 192.168.1.100" style="width: 120px;">
        </label>
        <button id="webcam-btn" onclick="toggleWebcam()">Use Webcam</button>
        <button id="invert-btn" onclick="toggleInvert()" style="background:#ff5500;">Invert Steering: OFF</button>
        <label>
            Sensitivity: <input type="range" id="sensitivity" min="2" max="20" value="8"> <span id="sens-val">8&deg;</span>
        </label>
        <label>
            Speed: <input type="range" id="speed" min="1" max="10" value="5">
        </label>
    </div>

    <script>
        // ==========================================
        // CONFIGURATION & STATE
        // ==========================================
        
        let ESP_IP = window.location.hostname ? window.location.hostname : "192.168.4.1"; 
        
        // Game Constants
        const CANVAS_W = 640;
        const CANVAS_H = 480;
        const LANE_WIDTH = CANVAS_W / 3;
        const CAR_SIZE = 60;
        
        // State
        let model = null;
        let isGameRunning = false;
        let score = 0;
        let gameLoopId;
        let invertSteering = false;
        
        // Webcam State
        let useWebcam = false;
        const videoElement = document.createElement('video');
        videoElement.autoplay = true;
        videoElement.playsInline = true;

        // Player
        let playerLane = 1; // 0: Left, 1: Center, 2: Right
        let playerY = CANVAS_H - 100;
        let currentTilt = 0;
        
        // Obstacles
        let obstacles = [];
        let obstacleSpawnTimer = 0;
        let gameSpeed = 5;

        // DOM Elements
        const videoCanvas = document.getElementById('videoCanvas');
        const videoCtx = videoCanvas.getContext('2d');
        
        const gameCanvas = document.getElementById('gameCanvas');
        const gameCtx = gameCanvas.getContext('2d');

        const statusTitle = document.getElementById('status-title');
        const statusText = document.getElementById('status-text');
        const startBtn = document.getElementById('start-btn');
        const scoreDisplay = document.getElementById('score-display');
        const tiltDisplay = document.getElementById('tilt-display');
        const ipInput = document.getElementById('cam-ip');
        const sensInput = document.getElementById('sensitivity');
        const sensVal = document.getElementById('sens-val');
        const speedInput = document.getElementById('speed');
        const webcamBtn = document.getElementById('webcam-btn');
        const invertBtn = document.getElementById('invert-btn');

        // Initialize Inputs
        ipInput.value = ESP_IP;
        sensInput.oninput = () => sensVal.innerText = sensInput.value + "°";
        
        // Image Source
        const img = new Image();
        img.crossOrigin = "Anonymous"; 

        // ==========================================
        // INITIALIZATION
        // ==========================================

        async function init() {
            try {
                statusText.innerText = "Loading Face Mesh...";
                model = await faceLandmarksDetection.load(
                    faceLandmarksDetection.SupportedPackages.mediapipeFacemesh
                );
                
                statusTitle.innerText = "Ready!";
                statusText.innerText = "Connect Camera or use Webcam.";
                startBtn.disabled = false;
                startBtn.innerText = "Start Game";
                
                requestAnimationFrame(gameLoop);
                
            } catch (e) {
                console.error(e);
                statusTitle.innerText = "Error";
                statusText.innerText = "Could not load AI model.";
            }
        }

        // ==========================================
        // CONTROLS
        // ==========================================
        
        async function toggleWebcam() {
            useWebcam = !useWebcam;
            if (useWebcam) {
                try {
                    const stream = await navigator.mediaDevices.getUserMedia({ video: { width: 640, height: 480 } });
                    videoElement.srcObject = stream;
                    await new Promise(resolve => videoElement.onloadedmetadata = resolve);
                    webcamBtn.innerText = "Use ESP32";
                    webcamBtn.style.background = "#ff9900";
                    ipInput.disabled = true;
                } catch (err) {
                    console.error("Webcam error:", err);
                    alert("Camera access denied.");
                    useWebcam = false;
                }
            } else {
                if (videoElement.srcObject) {
                    videoElement.srcObject.getTracks().forEach(track => track.stop());
                    videoElement.srcObject = null;
                }
                webcamBtn.innerText = "Use Webcam";
                webcamBtn.style.background = "#00d2ff";
                ipInput.disabled = false;
            }
        }

        function toggleInvert() {
            invertSteering = !invertSteering;
            invertBtn.innerText = invertSteering ? "Invert Steering: ON" : "Invert Steering: OFF";
            invertBtn.style.background = invertSteering ? "#00ff00" : "#ff5500";
            invertBtn.style.color = invertSteering ? "#000" : "#fff";
        }

        // ==========================================
        // GAME LOOP
        // ==========================================

        async function gameLoop(timestamp) {
            let inputImage = null;

            // 1. Get Image
            if (useWebcam) {
                if (videoElement.readyState === 4) inputImage = videoElement;
            } else {
                await fetchFrame();
                if (img.complete && img.naturalWidth > 0) inputImage = img;
            }

            // 2. Render Video & Tracking (Left Panel)
            videoCtx.clearRect(0, 0, CANVAS_W, CANVAS_H);
            
            if (inputImage) {
                videoCtx.save();
                if (useWebcam) {
                    videoCtx.translate(CANVAS_W, 0);
                    videoCtx.scale(-1, 1);
                }
                videoCtx.drawImage(inputImage, 0, 0, CANVAS_W, CANVAS_H);
                
                // Face Detection
                const predictions = await model.estimateFaces({ input: inputImage });
                if (predictions.length > 0) {
                    const keypoints = predictions[0].scaledMesh;
                    processHeadTilt(keypoints);
                    drawFaceMesh(videoCtx, keypoints);
                }
                videoCtx.restore();
            } else {
                videoCtx.fillStyle = "#222";
                videoCtx.fillRect(0, 0, CANVAS_W, CANVAS_H);
                videoCtx.fillStyle = "#555";
                videoCtx.font = "20px Arial";
                videoCtx.fillText("No Signal", CANVAS_W/2 - 40, CANVAS_H/2);
            }

            // 3. Render Game (Right Panel)
            // Always clear with asphalt color
            gameCtx.fillStyle = "#333"; 
            gameCtx.fillRect(0, 0, CANVAS_W, CANVAS_H);
            
            drawGameWorld(gameCtx);

            // 4. Update Logic
            if (isGameRunning) {
                updateGameLogic();
            }

            requestAnimationFrame(gameLoop);
        }

        // ==========================================
        // VIDEO FETCHING
        // ==========================================
        
        let isFetching = false;
        async function fetchFrame() {
            if (isFetching) return;
            isFetching = true;
            const ip = ipInput.value;
            const url = `http://${ip}/capture?t=${Date.now()}`;
            return new Promise((resolve) => {
                img.onload = () => { isFetching = false; resolve(); };
                img.onerror = () => { isFetching = false; resolve(); };
                img.src = url;
            });
        }

        // ==========================================
        // FACE LOGIC
        // ==========================================

        function processHeadTilt(keypoints) {
            const leftEye = keypoints[133];
            const rightEye = keypoints[263];
            const dx = rightEye[0] - leftEye[0];
            const dy = rightEye[1] - leftEye[1];
            let angle = Math.atan2(dy, dx) * (180 / Math.PI);
            
            // Apply Inversion
            if (invertSteering) {
                angle = -angle;
            }

            currentTilt = angle;
            tiltDisplay.innerHTML = `Tilt: ${angle.toFixed(1)}&deg;`;

            const threshold = parseInt(sensInput.value);
            if (angle > threshold) playerLane = 2; // Right
            else if (angle < -threshold) playerLane = 0; // Left
            else playerLane = 1; // Center
        }

        // ==========================================
        // DRAWING HELPERS
        // ==========================================

        function drawFaceMesh(ctx, keypoints) {
            // Draw bright red dots for high visibility
            ctx.fillStyle = '#ff0000'; 
            ctx.strokeStyle = '#ffff00';
            ctx.lineWidth = 3;

            // Draw Eyes
            [133, 263].forEach(id => {
                const p = keypoints[id];
                ctx.beginPath();
                ctx.arc(p[0], p[1], 8, 0, 2 * Math.PI); // Increased size to 8
                ctx.fill();
                ctx.stroke();
            });

            // Draw Line between eyes
            const p1 = keypoints[133];
            const p2 = keypoints[263];
            ctx.beginPath();
            ctx.moveTo(p1[0], p1[1]);
            ctx.lineTo(p2[0], p2[1]);
            ctx.stroke();
            
            // Debug Text
            ctx.save();
            if (useWebcam) {
                 // Un-mirror text so it's readable
                 ctx.translate(CANVAS_W, 0);
                 ctx.scale(-1, 1);
                 ctx.fillStyle = "#00ff00";
                 ctx.font = "20px Arial";
                 ctx.fillText("TRACKING ACTIVE", 20, 30);
            } else {
                 ctx.fillStyle = "#00ff00";
                 ctx.font = "20px Arial";
                 ctx.fillText("TRACKING ACTIVE", 20, 30);
            }
            ctx.restore();
        }

        function drawGameWorld(ctx) {
            // Road Markings
            ctx.strokeStyle = "rgba(255, 255, 255, 0.5)";
            ctx.lineWidth = 4;
            ctx.setLineDash([20, 20]); // Dashed lines
            ctx.beginPath();
            ctx.moveTo(LANE_WIDTH, 0);
            ctx.lineTo(LANE_WIDTH, CANVAS_H);
            ctx.moveTo(LANE_WIDTH * 2, 0);
            ctx.lineTo(LANE_WIDTH * 2, CANVAS_H);
            ctx.stroke();
            ctx.setLineDash([]);

            // Player Car
            const playerX = (playerLane * LANE_WIDTH) + (LANE_WIDTH / 2) - (CAR_SIZE / 2);
            
            // Car Body
            ctx.fillStyle = "#00d2ff";
            ctx.shadowBlur = 15;
            ctx.shadowColor = "#00d2ff";
            ctx.fillRect(playerX, playerY, CAR_SIZE, CAR_SIZE);
            ctx.shadowBlur = 0;
            
            // Windshield
            ctx.fillStyle = "#000";
            ctx.fillRect(playerX + 10, playerY + 10, CAR_SIZE - 20, 15);

            // Obstacles
            if (isGameRunning) {
                ctx.fillStyle = "#ff0055";
                obstacles.forEach(obs => {
                    const obsX = (obs.lane * LANE_WIDTH) + (LANE_WIDTH / 2) - (CAR_SIZE / 2);
                    ctx.shadowBlur = 15;
                    ctx.shadowColor = "#ff0055";
                    ctx.fillRect(obsX, obs.y, CAR_SIZE, CAR_SIZE);
                    ctx.shadowBlur = 0;
                    ctx.fillStyle = "#fff";
                    ctx.fillRect(obsX + 10, obs.y + 10, CAR_SIZE - 20, 5);
                    ctx.fillStyle = "#ff0055"; 
                });
            }
        }

        // ==========================================
        // GAME LOGIC
        // ==========================================

        function startGame() {
            isGameRunning = true;
            score = 0;
            obstacles = [];
            document.getElementById('status-msg').style.display = 'none';
        }

        function stopGame(reason) {
            isGameRunning = false;
            document.getElementById('status-msg').style.display = 'block';
            statusTitle.innerText = "Game Over";
            statusText.innerText = `${reason} Score: ${score}`;
            startBtn.innerText = "Play Again";
        }

        function updateGameLogic() {
            gameSpeed = parseInt(speedInput.value);
            score++;
            scoreDisplay.innerText = `Score: ${score}`;

            obstacleSpawnTimer++;
            if (obstacleSpawnTimer > (60 - gameSpeed * 3)) { 
                spawnObstacle();
                obstacleSpawnTimer = 0;
            }

            for (let i = obstacles.length - 1; i >= 0; i--) {
                let obs = obstacles[i];
                obs.y += gameSpeed + 2;

                const playerX = (playerLane * LANE_WIDTH) + (LANE_WIDTH / 2);
                if (obs.lane === playerLane) {
                    if (obs.y + 40 > playerY && obs.y < playerY + CAR_SIZE) {
                        stopGame("CRASHED!");
                    }
                }
                if (obs.y > CANVAS_H) obstacles.splice(i, 1);
            }
        }

        function spawnObstacle() {
            const lane = Math.floor(Math.random() * 3);
            obstacles.push({ lane: lane, y: -50 });
        }

        init();

    </script>
</body>
</html>
)rawliteral";
