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
            padding: 0;
            background-color: #111;
            color: #fff;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            overflow: hidden; /* Prevent scrolling */
            display: flex;
            flex-direction: column;
            align-items: center;
            height: 100vh;
        }

        #game-container {
            position: relative;
            width: 640px;
            height: 480px;
            margin-top: 20px;
            border: 4px solid #333;
            border-radius: 8px;
            box-shadow: 0 0 20px rgba(0, 255, 255, 0.2);
            background: #000;
        }

        /* The video feed is drawn on the background canvas */
        canvas {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
        }

        #ui-layer {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none; /* Let clicks pass through to buttons below if needed */
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            padding: 10px;
            box-sizing: border-box;
        }

        .hud-top {
            display: flex;
            justify-content: space-between;
            font-size: 24px;
            font-weight: bold;
            text-shadow: 2px 2px 0 #000;
        }

        .hud-bottom {
            text-align: center;
            font-size: 18px;
            text-shadow: 1px 1px 0 #000;
            color: #aaa;
        }

        #status-msg {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(0, 0, 0, 0.8);
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            pointer-events: auto;
            z-index: 10;
        }

        button {
            background: #00d2ff;
            color: #000;
            border: none;
            padding: 10px 20px;
            font-size: 18px;
            font-weight: bold;
            cursor: pointer;
            border-radius: 5px;
            margin-top: 10px;
        }
        button:hover { background: #33e0ff; }
        button:disabled { background: #555; cursor: not-allowed; }

        .controls {
            margin-top: 15px;
            display: flex;
            gap: 15px;
            align-items: center;
        }
        
        label { font-size: 14px; color: #ccc; }
        input[type=range] { width: 100px; }

        /* Lane Indicators */
        .lane-marker {
            position: absolute;
            bottom: 20px;
            width: 30px;
            height: 30px;
            border-radius: 50%;
            background: #333;
            border: 2px solid #555;
            transition: all 0.1s;
        }
        #lane-left { left: 20%; }
        #lane-center { left: 50%; transform: translateX(-50%); }
        #lane-right { right: 20%; }
        
        .active-lane {
            background: #0f0 !important;
            box-shadow: 0 0 10px #0f0;
            border-color: #fff !important;
        }

    </style>
</head>
<body>

    <h1>Head-Tilt Racing</h1>

    <div id="game-container">
        <!-- Canvas for rendering the game and video -->
        <canvas id="gameCanvas" width="640" height="480"></canvas>

        <!-- Lane Indicators (Visual Feedback) -->
        <div id="lane-left" class="lane-marker"></div>
        <div id="lane-center" class="lane-marker"></div>
        <div id="lane-right" class="lane-marker"></div>

        <div id="ui-layer">
            <div class="hud-top">
                <div id="score-display">Score: 0</div>
                <div id="tilt-display">Tilt: 0&deg;</div>
            </div>
            <div class="hud-bottom">
                Tilt your head LEFT or RIGHT to switch lanes!
            </div>
        </div>

        <!-- Start/Status Overlay -->
        <div id="status-msg">
            <h2 id="status-title">Loading AI Model...</h2>
            <p id="status-text">Please wait while we load the face tracking model.</p>
            <button id="start-btn" disabled onclick="startGame()">Start Game</button>
        </div>
    </div>

    <div class="controls">
        <label>
            Camera IP: <input type="text" id="cam-ip" value="" placeholder="e.g. 192.168.1.100" style="width: 120px;">
        </label>
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
        
        // If served from ESP32, use relative path. If local file, use default IP.
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
        let lastFrameTime = 0;
        
        // Player
        let playerLane = 1; // 0: Left, 1: Center, 2: Right
        let playerY = CANVAS_H - 100;
        let currentTilt = 0;
        
        // Obstacles
        let obstacles = [];
        let obstacleSpawnTimer = 0;
        let gameSpeed = 5;

        // DOM Elements
        const canvas = document.getElementById('gameCanvas');
        const ctx = canvas.getContext('2d');
        const statusTitle = document.getElementById('status-title');
        const statusText = document.getElementById('status-text');
        const startBtn = document.getElementById('start-btn');
        const scoreDisplay = document.getElementById('score-display');
        const tiltDisplay = document.getElementById('tilt-display');
        const ipInput = document.getElementById('cam-ip');
        const sensInput = document.getElementById('sensitivity');
        const sensVal = document.getElementById('sens-val');
        const speedInput = document.getElementById('speed');

        // Initialize Inputs
        ipInput.value = ESP_IP;
        sensInput.oninput = () => sensVal.innerText = sensInput.value + "°";
        
        // Image Source
        const img = new Image();
        img.crossOrigin = "Anonymous"; // Critical for CORS

        // ==========================================
        // INITIALIZATION
        // ==========================================

        async function init() {
            try {
                statusText.innerText = "Loading TensorFlow.js Face Mesh...";
                // Load the Face Landmarks Detection model
                model = await faceLandmarksDetection.load(
                    faceLandmarksDetection.SupportedPackages.mediapipeFacemesh
                );
                
                statusTitle.innerText = "Ready to Race!";
                statusText.innerText = "Make sure your camera is on and connected.";
                startBtn.disabled = false;
                startBtn.innerText = "Start Game";
                
                // Start the polling loop (fetches images even if game not running, for preview)
                requestAnimationFrame(gameLoop);
                
            } catch (e) {
                console.error(e);
                statusTitle.innerText = "Error Loading AI";
                statusText.innerText = "Could not load Face Model. Check internet connection (needed for first load).";
            }
        }

        // ==========================================
        // GAME LOOP
        // ==========================================

        async function gameLoop(timestamp) {
            // 1. Fetch Frame from ESP32
            await fetchFrame();

            // 2. Process Face (if image is valid)
            if (img.complete && img.naturalWidth > 0) {
                // Draw video background
                ctx.drawImage(img, 0, 0, CANVAS_W, CANVAS_H);
                
                // Detect Face
                const predictions = await model.estimateFaces({ input: img });
                
                if (predictions.length > 0) {
                    const keypoints = predictions[0].scaledMesh;
                    processHeadTilt(keypoints);
                    drawFaceMesh(keypoints); // Debug overlay
                }
            } else {
                // No video signal
                ctx.fillStyle = "#333";
                ctx.fillRect(0, 0, CANVAS_W, CANVAS_H);
                ctx.fillStyle = "#fff";
                ctx.font = "20px Arial";
                ctx.fillText("Waiting for Camera...", CANVAS_W/2 - 80, CANVAS_H/2);
            }

            // 3. Update & Draw Game (if running)
            if (isGameRunning) {
                updateGame();
                drawGame();
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
            // Add random query param to prevent caching
            const url = `http://${ip}/capture?t=${Date.now()}`;
            
            return new Promise((resolve) => {
                img.onload = () => {
                    isFetching = false;
                    resolve();
                };
                img.onerror = () => {
                    isFetching = false;
                    resolve(); // Resolve anyway to keep loop going
                };
                img.src = url;
            });
        }

        // ==========================================
        // FACE LOGIC
        // ==========================================

        function processHeadTilt(keypoints) {
            // MediaPipe Facemesh Keypoints:
            // Left Eye: 33 (inner), 133 (outer)
            // Right Eye: 362 (inner), 263 (outer)
            // We'll use outer corners for wider baseline: 133 (Left) and 263 (Right)
            
            const leftEye = keypoints[133];  // [x, y, z]
            const rightEye = keypoints[263];
            
            // Calculate slope (dy/dx)
            const dx = rightEye[0] - leftEye[0];
            const dy = rightEye[1] - leftEye[1];
            
            // Calculate angle in degrees
            // Note: Y axis is inverted in canvas (down is positive), so we flip sign if needed
            // But usually: if Right Eye is LOWER (larger Y) than Left Eye, head is tilted RIGHT.
            const angle = Math.atan2(dy, dx) * (180 / Math.PI);
            
            currentTilt = angle;
            tiltDisplay.innerHTML = `Tilt: ${angle.toFixed(1)}&deg;`;

            // Map to Lanes
            const threshold = parseInt(sensInput.value);
            
            // Reset active classes
            document.getElementById('lane-left').classList.remove('active-lane');
            document.getElementById('lane-center').classList.remove('active-lane');
            document.getElementById('lane-right').classList.remove('active-lane');

            if (angle > threshold) {
                playerLane = 2; // Right
                document.getElementById('lane-right').classList.add('active-lane');
            } else if (angle < -threshold) {
                playerLane = 0; // Left
                document.getElementById('lane-left').classList.add('active-lane');
            } else {
                playerLane = 1; // Center
                document.getElementById('lane-center').classList.add('active-lane');
            }
        }

        function drawFaceMesh(keypoints) {
            ctx.fillStyle = '#00ff00';
            // Draw just eyes to verify tracking
            [133, 263].forEach(id => {
                const p = keypoints[id];
                ctx.beginPath();
                ctx.arc(p[0], p[1], 3, 0, 2 * Math.PI);
                ctx.fill();
            });
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
            statusText.innerText = `${reason} Final Score: ${score}`;
            startBtn.innerText = "Play Again";
        }

        function updateGame() {
            gameSpeed = parseInt(speedInput.value);
            score++;
            scoreDisplay.innerText = `Score: ${score}`;

            // Spawn Obstacles
            obstacleSpawnTimer++;
            if (obstacleSpawnTimer > (60 - gameSpeed * 3)) { // Higher speed = faster spawn
                spawnObstacle();
                obstacleSpawnTimer = 0;
            }

            // Move Obstacles
            for (let i = obstacles.length - 1; i >= 0; i--) {
                let obs = obstacles[i];
                obs.y += gameSpeed + 2;

                // Collision Detection
                // Simple box collision. Player is at specific lane X, fixed Y.
                // Lane Centers: Left (1/6), Center (3/6), Right (5/6) of Width
                const playerX = (playerLane * LANE_WIDTH) + (LANE_WIDTH / 2);
                const obsX = (obs.lane * LANE_WIDTH) + (LANE_WIDTH / 2);

                // Check if in same lane and overlapping Y
                if (obs.lane === playerLane) {
                    if (obs.y + 40 > playerY && obs.y < playerY + CAR_SIZE) {
                        stopGame("CRASHED!");
                    }
                }

                // Remove if off screen
                if (obs.y > CANVAS_H) {
                    obstacles.splice(i, 1);
                }
            }
        }

        function spawnObstacle() {
            // Random lane 0, 1, 2
            const lane = Math.floor(Math.random() * 3);
            obstacles.push({ lane: lane, y: -50 });
        }

        function drawGame() {
            // Draw Road Lines
            ctx.strokeStyle = "rgba(255, 255, 255, 0.3)";
            ctx.lineWidth = 4;
            ctx.beginPath();
            ctx.moveTo(LANE_WIDTH, 0);
            ctx.lineTo(LANE_WIDTH, CANVAS_H);
            ctx.moveTo(LANE_WIDTH * 2, 0);
            ctx.lineTo(LANE_WIDTH * 2, CANVAS_H);
            ctx.stroke();

            // Draw Player Car
            const playerX = (playerLane * LANE_WIDTH) + (LANE_WIDTH / 2) - (CAR_SIZE / 2);
            
            // Car Body
            ctx.fillStyle = "#00d2ff";
            ctx.shadowBlur = 20;
            ctx.shadowColor = "#00d2ff";
            ctx.fillRect(playerX, playerY, CAR_SIZE, CAR_SIZE);
            ctx.shadowBlur = 0;
            
            // Car Details (Windshield)
            ctx.fillStyle = "#000";
            ctx.fillRect(playerX + 10, playerY + 10, CAR_SIZE - 20, 15);

            // Draw Obstacles
            ctx.fillStyle = "#ff0055";
            obstacles.forEach(obs => {
                const obsX = (obs.lane * LANE_WIDTH) + (LANE_WIDTH / 2) - (CAR_SIZE / 2);
                ctx.shadowBlur = 20;
                ctx.shadowColor = "#ff0055";
                ctx.fillRect(obsX, obs.y, CAR_SIZE, CAR_SIZE);
                ctx.shadowBlur = 0;
                
                // Danger stripes
                ctx.fillStyle = "#fff";
                ctx.fillRect(obsX + 10, obs.y + 10, CAR_SIZE - 20, 5);
                ctx.fillStyle = "#ff0055"; // reset
            });
        }

        // Start Init
        init();

    </script>
</body>
</html>
)rawliteral";
