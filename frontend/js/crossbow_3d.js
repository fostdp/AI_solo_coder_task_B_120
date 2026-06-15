class Crossbow3D {
    constructor(container) {
        this.container = container;
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        this.crossbowGroup = null;
        this.stringLeft = null;
        this.stringRight = null;
        this.arrow = null;
        this.trajectoryLine = null;
        this.arrowPath = null;
        this.animating = false;
        this.currentCrossbow = null;
        this.showTrajectory = true;
        this.showGrid = true;
        this.wireframe = false;

        this.leftArmSkinned = null;
        this.rightArmSkinned = null;
        this.leftArmBones = [];
        this.rightArmBones = [];
        this.leftBowTip = null;
        this.rightBowTip = null;

        this.config = window.CROSSBOW_CONFIG;
        this.armBoneCount = this.config.crossbow.armBoneCount;
        this.animationLerp = this.config.animation.boneRotationLerp;
        this.currentDrawAmount = 0;
        this.targetDrawAmount = 0;

        this.shootingMode = false;
        this.aimingActive = false;
        this.aimAzimuthDeg = 0;
        this.aimElevationDeg = 15;
        this.drawPowerPercent = 0;
        this.isDrawing = false;
        this.targets = [];
        this.targetGroup = null;
        this.hitMarkers = [];
        this.hitMarkerGroup = null;
        this.reticle = null;
        this.reticleGroup = null;
        this.shootingScore = 0;
        this.shotsFired = 0;
        this.hitsScored = 0;
        this.virtualRaycaster = new THREE.Raycaster();
        this.aimPlane = new THREE.Plane(new THREE.Vector3(0, 1, 0), 0);
        this.aimIntersectPoint = new THREE.Vector3();
        this.onShootCallback = null;
        this.onHitCallback = null;
        this.drawingStartTime = 0;
        this.shakeIntensity = 0;
        this.virtualCameraMode = false;
        this.savedCameraPos = new THREE.Vector3();
        this.savedCameraTarget = new THREE.Vector3();
        this.shootingCrossbowData = null;
        this.lastTrajectoryPoints = [];

        this.init();
    }

    init() {
        const rect = this.container.getBoundingClientRect();
        const cfg = this.config.three;

        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(cfg.backgroundColor);
        this.scene.fog = new THREE.Fog(cfg.backgroundColor, cfg.fogNear, cfg.fogFar);

        this.camera = new THREE.PerspectiveCamera(cfg.cameraFov, rect.width / rect.height, cfg.cameraNear, cfg.cameraFar);
        this.camera.position.set(cfg.cameraInitPos.x, cfg.cameraInitPos.y, cfg.cameraInitPos.z);

        this.renderer = new THREE.WebGLRenderer({ antialias: true });
        this.renderer.setSize(rect.width, rect.height);
        this.renderer.setPixelRatio(window.devicePixelRatio);
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.container.appendChild(this.renderer.domElement);

        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = cfg.controlsDamping;
        this.controls.minDistance = cfg.controlsMinDistance;
        this.controls.maxDistance = cfg.controlsMaxDistance;
        this.controls.maxPolarAngle = cfg.controlsMaxPolarAngle;
        this.controls.target.set(cfg.cameraTarget.x, cfg.cameraTarget.y, cfg.cameraTarget.z);

        this.setupLights();
        this.setupGround();
        this.createCrossbow();

        window.addEventListener("resize", () => this.onResize());
        this.animate();
    }

    setupLights() {
        const ambient = new THREE.AmbientLight(0x404080, 0.5);
        this.scene.add(ambient);

        const mainLight = new THREE.DirectionalLight(0xffffff, 1.0);
        mainLight.position.set(5, 10, 7);
        mainLight.castShadow = true;
        mainLight.shadow.mapSize.width = 2048;
        mainLight.shadow.mapSize.height = 2048;
        mainLight.shadow.camera.near = 0.5;
        mainLight.shadow.camera.far = 50;
        mainLight.shadow.camera.left = -10;
        mainLight.shadow.camera.right = 10;
        mainLight.shadow.camera.top = 10;
        mainLight.shadow.camera.bottom = -10;
        this.scene.add(mainLight);

        const fillLight = new THREE.DirectionalLight(0x4a9eff, 0.3);
        fillLight.position.set(-5, 5, -5);
        this.scene.add(fillLight);

        const warmLight = new THREE.PointLight(0xf4a460, 0.5, 15);
        warmLight.position.set(0, 2, 0);
        this.scene.add(warmLight);
    }

    setupGround() {
        const groundGeo = new THREE.PlaneGeometry(50, 50);
        const groundMat = new THREE.MeshStandardMaterial({
            color: 0x1a2744, roughness: 0.9, metalness: 0.1
        });
        const ground = new THREE.Mesh(groundGeo, groundMat);
        ground.rotation.x = -Math.PI / 2;
        ground.receiveShadow = true;
        this.scene.add(ground);

        this.gridHelper = new THREE.GridHelper(40, 40, 0x4a9eff, 0x1e3a5f);
        this.gridHelper.material.opacity = 0.3;
        this.gridHelper.material.transparent = true;
        this.scene.add(this.gridHelper);

        const axesHelper = new THREE.AxesHelper(2);
        axesHelper.position.y = 0.01;
        this.scene.add(axesHelper);

        this.impactMarker = this.createImpactMarker();
        this.scene.add(this.impactMarker);
        this.impactMarker.visible = false;
    }

    createImpactMarker() {
        const group = new THREE.Group();
        const ring1 = new THREE.Mesh(
            new THREE.RingGeometry(0.3, 0.35, 32),
            new THREE.MeshBasicMaterial({ color: 0xef4444, side: THREE.DoubleSide, transparent: true, opacity: 0.8 })
        );
        ring1.rotation.x = -Math.PI / 2;
        ring1.position.y = 0.02;
        group.add(ring1);

        const ring2 = new THREE.Mesh(
            new THREE.RingGeometry(0.15, 0.18, 32),
            new THREE.MeshBasicMaterial({ color: 0xfbbf24, side: THREE.DoubleSide, transparent: true, opacity: 0.9 })
        );
        ring2.rotation.x = -Math.PI / 2;
        ring2.position.y = 0.02;
        group.add(ring2);

        const center = new THREE.Mesh(
            new THREE.CircleGeometry(0.05, 16),
            new THREE.MeshBasicMaterial({ color: 0x4ade80, side: THREE.DoubleSide, transparent: true, opacity: 0.9 })
        );
        center.rotation.x = -Math.PI / 2;
        center.position.y = 0.02;
        group.add(center);
        return group;
    }

    createSkinnedBowArm(armLength, radius, scale, isLeft) {
        const boneCount = this.armBoneCount;
        const segmentLength = armLength / boneCount;
        const geometry = new THREE.CylinderGeometry(radius, radius * 0.85, armLength, 12, boneCount, false);

        const position = geometry.attributes.position;
        const skinIndices = [];
        const skinWeights = [];

        for (let i = 0; i < position.count; i++) {
            const y = position.getY(i);
            const normalizedY = (y + armLength / 2) / armLength;
            const boneIndex = Math.min(boneCount - 1, Math.floor(normalizedY * boneCount));
            let blend = 0;
            if (boneIndex < boneCount - 1) {
                const localY = (normalizedY * boneCount) - boneIndex;
                blend = localY;
            }
            const idx0 = boneIndex;
            const idx1 = Math.min(boneCount - 1, boneIndex + 1);
            skinIndices.push(idx0, idx1, 0, 0);
            skinWeights.push(1 - blend, blend, 0, 0);
        }

        geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute(skinIndices, 4));
        geometry.setAttribute('skinWeight', new THREE.Float32BufferAttribute(skinWeights, 4));

        const material = new THREE.MeshStandardMaterial({
            color: 0xc4a882, roughness: 0.5, metalness: 0.2, skinning: true
        });

        const bones = [];
        let prevBone = new THREE.Bone();
        prevBone.position.y = -armLength / 2;
        bones.push(prevBone);

        for (let i = 1; i < boneCount; i++) {
            const bone = new THREE.Bone();
            bone.position.y = segmentLength;
            prevBone.add(bone);
            bones.push(bone);
            prevBone = bone;
        }

        const skeleton = new THREE.Skeleton(bones);
        const skinnedMesh = new THREE.SkinnedMesh(geometry, material);
        skinnedMesh.castShadow = true;
        skinnedMesh.receiveShadow = true;
        skinnedMesh.add(bones[0]);
        skinnedMesh.bind(skeleton);

        const tipBone = bones[bones.length - 1];
        const tip = new THREE.Mesh(
            new THREE.SphereGeometry(0.04 * scale, 12, 12),
            new THREE.MeshStandardMaterial({ color: 0x8899aa, roughness: 0.35, metalness: 0.85 })
        );
        tip.castShadow = true;
        tipBone.add(tip);

        return { skinnedMesh, bones, skeleton, tip, tipBone };
    }

    getBowArmBendingAngles(drawAmount, maxDraw, boneCount, armLength) {
        const angles = [];
        const normalizedDraw = Math.min(Math.max(drawAmount / maxDraw, 0), 1);
        const maxAngle = Math.PI / 4;
        const cumulativeAngle = normalizedDraw * maxAngle;
        for (let i = 0; i < boneCount; i++) {
            const bonePos = i / (boneCount - 1);
            const angleFactor = Math.sin(Math.PI * 0.5 * bonePos);
            angles.push(cumulativeAngle * angleFactor * 0.5);
        }
        return angles;
    }

    updateArmBones(bones, isLeft, drawAmount, maxDraw) {
        const angles = this.getBowArmBendingAngles(drawAmount, maxDraw, bones.length, 1.0);
        for (let i = 0; i < bones.length; i++) {
            const targetAngle = isLeft ? -angles[i] : angles[i];
            const currentAngle = bones[i].rotation.z;
            bones[i].rotation.z = currentAngle + (targetAngle - currentAngle) * this.animationLerp;
            if (i > 0) bones[i].position.y = bones[i - 1].position.y + (1.0 / bones.length);
        }
    }

    createCrossbow(crossbowData = null) {
        if (this.crossbowGroup) this.scene.remove(this.crossbowGroup);
        this.currentCrossbow = crossbowData;

        const scale = crossbowData ? Math.min(crossbowData.bow_length / 1.4, 1.2) : 1.0;
        const bowLength = crossbowData ? crossbowData.bow_length : 1.4;
        const stringLength = crossbowData ? crossbowData.string_length : 1.45;
        this.bowLength = bowLength;
        this.bowScale = scale;
        this.maxDraw = bowLength * 0.6 * scale;

        this.crossbowGroup = new THREE.Group();
        const woodMat = new THREE.MeshStandardMaterial({ color: 0x8b5a2b, roughness: 0.7, metalness: 0.15 });
        const darkWoodMat = new THREE.MeshStandardMaterial({ color: 0x5c3317, roughness: 0.8, metalness: 0.1 });
        const metalMat = new THREE.MeshStandardMaterial({ color: 0x8899aa, roughness: 0.35, metalness: 0.85 });

        const stockLen = 1.2 * scale;
        const stock = new THREE.Mesh(new THREE.BoxGeometry(0.08 * scale, 0.06 * scale, stockLen), woodMat);
        stock.castShadow = true; stock.receiveShadow = true; this.crossbowGroup.add(stock);

        const grip = new THREE.Mesh(new THREE.BoxGeometry(0.1 * scale, 0.2 * scale, 0.12 * scale), darkWoodMat);
        grip.position.y = -0.12 * scale; grip.position.z = 0.15 * scale;
        grip.castShadow = true; this.crossbowGroup.add(grip);

        const triggerGuard = new THREE.Mesh(new THREE.TorusGeometry(0.05 * scale, 0.015 * scale, 8, 16, Math.PI), metalMat);
        triggerGuard.position.y = -0.08 * scale; triggerGuard.position.z = 0.18 * scale;
        triggerGuard.rotation.x = Math.PI; this.crossbowGroup.add(triggerGuard);

        const bowArmLen = bowLength / 2 * scale;
        const bowArmRadius = 0.035 * scale;

        const leftArmResult = this.createSkinnedBowArm(bowArmLen, bowArmRadius, scale, true);
        leftArmResult.skinnedMesh.position.z = -stockLen / 2 + 0.05;
        leftArmResult.skinnedMesh.rotation.z = -Math.PI / 12;
        this.crossbowGroup.add(leftArmResult.skinnedMesh);

        const rightArmResult = this.createSkinnedBowArm(bowArmLen, bowArmRadius, scale, false);
        rightArmResult.skinnedMesh.position.z = -stockLen / 2 + 0.05;
        rightArmResult.skinnedMesh.rotation.z = Math.PI / 12;
        this.crossbowGroup.add(rightArmResult.skinnedMesh);

        this.leftArmSkinned = leftArmResult.skinnedMesh;
        this.rightArmSkinned = rightArmResult.skinnedMesh;
        this.leftArmBones = leftArmResult.bones;
        this.rightArmBones = rightArmResult.bones;
        this.leftBowTip = leftArmResult.tip;
        this.rightBowTip = rightArmResult.tip;

        this.stringAnchorL = new THREE.Vector3(-bowArmLen * 0.95, bowArmLen * 0.15, -stockLen / 2 + 0.05);
        this.stringAnchorR = new THREE.Vector3(bowArmLen * 0.95, bowArmLen * 0.15, -stockLen / 2 + 0.05);

        const stringMat = new THREE.LineBasicMaterial({ color: 0xd4c4a8, linewidth: 2 });
        this.stringRestZ = -stockLen / 2 + 0.15;
        this.stringDrawnZ = 0.2;

        const stringGeoL = new THREE.BufferGeometry().setFromPoints([
            this.stringAnchorL, new THREE.Vector3(0, 0, this.stringRestZ)
        ]);
        this.stringLeft = new THREE.Line(stringGeoL, stringMat);
        this.crossbowGroup.add(this.stringLeft);

        const stringGeoR = new THREE.BufferGeometry().setFromPoints([
            this.stringAnchorR, new THREE.Vector3(0, 0, this.stringRestZ)
        ]);
        this.stringRight = new THREE.Line(stringGeoR, stringMat);
        this.crossbowGroup.add(this.stringRight);

        this.createArrow(scale);

        const sightHeight = 0.12 * scale;
        const sightPole = new THREE.Mesh(
            new THREE.CylinderGeometry(0.005 * scale, 0.005 * scale, sightHeight, 8), metalMat
        );
        sightPole.position.y = sightHeight / 2; sightPole.position.z = 0.1;
        this.crossbowGroup.add(sightPole);

        const sightPinGeo = new THREE.BoxGeometry(0.02 * scale, 0.003 * scale, 0.003 * scale);
        for (let i = 0; i < 5; i++) {
            const pin = new THREE.Mesh(sightPinGeo, metalMat);
            pin.position.y = 0.03 * scale + i * 0.02 * scale;
            pin.position.z = 0.1;
            this.crossbowGroup.add(pin);
        }

        this.crossbowGroup.position.y = 1.2;
        this.crossbowGroup.rotation.y = Math.PI / 6;
        this.scene.add(this.crossbowGroup);

        this.currentDrawAmount = 0;
        this.targetDrawAmount = 0;
        this.updateArmDeformation(0);
        this.setStringPosition(0, 0, this.stringRestZ);
    }

    updateArmDeformation(drawAmount) {
        this.targetDrawAmount = drawAmount;
        const smoothDraw = this.currentDrawAmount + (this.targetDrawAmount - this.currentDrawAmount) * 0.15;
        this.currentDrawAmount = smoothDraw;
        this.updateArmBones(this.leftArmBones, true, smoothDraw, this.maxDraw);
        this.updateArmBones(this.rightArmBones, false, smoothDraw, this.maxDraw);
        this.updateStringAnchors();
    }

    updateStringAnchors() {
        const worldPosL = new THREE.Vector3();
        const worldPosR = new THREE.Vector3();
        if (this.leftBowTip) {
            this.leftBowTip.getWorldPosition(worldPosL);
            this.stringAnchorL.copy(this.crossbowGroup.worldToLocal(worldPosL.clone()));
        }
        if (this.rightBowTip) {
            this.rightBowTip.getWorldPosition(worldPosR);
            this.stringAnchorR.copy(this.crossbowGroup.worldToLocal(worldPosR.clone()));
        }
    }

    createArrow(scale) {
        const arrowGroup = new THREE.Group();
        const shaftLen = 0.7 * scale;
        const shaft = new THREE.Mesh(
            new THREE.CylinderGeometry(0.005 * scale, 0.005 * scale, shaftLen, 8),
            new THREE.MeshStandardMaterial({ color: 0xb8860b, roughness: 0.7, metalness: 0.1 })
        );
        shaft.rotation.x = Math.PI / 2; shaft.position.z = -shaftLen / 2;
        shaft.castShadow = true; arrowGroup.add(shaft);

        const head = new THREE.Mesh(
            new THREE.ConeGeometry(0.015 * scale, 0.06 * scale, 8),
            new THREE.MeshStandardMaterial({ color: 0x777777, roughness: 0.3, metalness: 0.9 })
        );
        head.rotation.x = -Math.PI / 2; head.position.z = -shaftLen - 0.03 * scale;
        head.castShadow = true; arrowGroup.add(head);

        const fletchingMat = new THREE.MeshStandardMaterial({
            color: 0xdc2626, roughness: 0.8, side: THREE.DoubleSide, transparent: true, opacity: 0.9
        });
        for (let i = 0; i < 3; i++) {
            const fletch = new THREE.Mesh(new THREE.PlaneGeometry(0.04 * scale, 0.02 * scale), fletchingMat);
            fletch.position.z = 0.02 * scale;
            fletch.rotation.z = (i * Math.PI * 2) / 3;
            fletch.rotation.y = Math.PI / 2;
            const radius = 0.015 * scale;
            fletch.position.x = Math.cos(i * Math.PI * 2 / 3) * radius;
            fletch.position.y = Math.sin(i * Math.PI * 2 / 3) * radius;
            arrowGroup.add(fletch);
        }

        this.arrow = arrowGroup;
        this.arrowRestZ = -0.1;
        this.arrowDrawnZ = 0.15;
        this.arrow.position.z = this.arrowRestZ;
        this.crossbowGroup.add(this.arrow);
    }

    setStringPosition(x, y, z) {
        const centerPoint = new THREE.Vector3(x, y, z);
        const drawAmount = (z - this.stringRestZ) / (this.stringDrawnZ - this.stringRestZ) * this.maxDraw;
        this.updateArmDeformation(Math.max(0, drawAmount));

        const posL = this.stringLeft.geometry.attributes.position;
        posL.setXYZ(0, this.stringAnchorL.x, this.stringAnchorL.y, this.stringAnchorL.z);
        posL.setXYZ(1, centerPoint.x, centerPoint.y, centerPoint.z);
        posL.needsUpdate = true;

        const posR = this.stringRight.geometry.attributes.position;
        posR.setXYZ(0, this.stringAnchorR.x, this.stringAnchorR.y, this.stringAnchorR.z);
        posR.setXYZ(1, centerPoint.x, centerPoint.y, centerPoint.z);
        posR.needsUpdate = true;

        if (this.arrow) {
            this.arrow.position.z = z;
            this.arrow.position.x = x;
            this.arrow.position.y = y;
        }
    }

    playDrawAnimation(callback) {
        if (this.animating) return;
        this.animating = true;
        const duration = this.config.animation.drawAnimationDurationMs;
        const start = performance.now();
        const restZ = this.stringRestZ;
        const drawnZ = this.stringDrawnZ;

        const animate = (now) => {
            const elapsed = now - start;
            let t = Math.min(elapsed / duration, 1);
            t = t * t * (3 - 2 * t);
            this.setStringPosition(0, 0, restZ + (drawnZ - restZ) * t);
            if (t < 1) {
                requestAnimationFrame(animate);
            } else {
                setTimeout(() => this.playReleaseAnimation(callback), 300);
            }
        };
        requestAnimationFrame(animate);
    }

    playReleaseAnimation(callback) {
        const duration = this.config.animation.releaseAnimationDurationMs;
        const start = performance.now();
        const drawnZ = this.stringDrawnZ;
        const restZ = this.stringRestZ;

        const animate = (now) => {
            const elapsed = now - start;
            let t = Math.min(elapsed / duration, 1);
            t = 1 - Math.pow(1 - t, 3);
            const z = drawnZ + (restZ - drawnZ) * t;
            const vibrate = t < 0.3 ? Math.sin(t * 30) * (0.3 - t) * 0.02 : 0;
            this.setStringPosition(0, vibrate, z);
            if (t < 1) {
                requestAnimationFrame(animate);
            } else {
                this.animating = false;
                if (callback) callback();
            }
        };
        requestAnimationFrame(animate);
    }

    async playShotAnimation(trajectoryData) {
        if (this.animating) return;
        this.animating = true;
        const duration = 1200;
        const start = performance.now();
        const drawnZ = this.stringDrawnZ;
        const restZ = this.stringRestZ;

        await new Promise(resolve => {
            const drawAnim = (now) => {
                const elapsed = now - start;
                let t = Math.min(elapsed / 600, 1);
                t = t * t * (3 - 2 * t);
                this.setStringPosition(0, 0, restZ + (drawnZ - restZ) * t);
                if (t < 1) requestAnimationFrame(drawAnim);
                else resolve();
            };
            requestAnimationFrame(drawAnim);
        });

        await new Promise(resolve => setTimeout(resolve, 200));
        if (this.arrow) this.arrow.visible = true;

        const releaseStart = performance.now();
        const points = trajectoryData;

        if (points && points.length > 1) {
            this.showTrajectoryPath(points);

            await new Promise(resolve => {
                const flyAnim = () => {
                    const elapsed = performance.now() - releaseStart;
                    const totalTime = points[points.length - 1].t;
                    let currentT = (elapsed / duration) * totalTime;

                    let idx = 0;
                    while (idx < points.length - 1 && points[idx + 1].t < currentT) idx++;

                    if (idx >= points.length - 1) {
                        const last = points[points.length - 1];
                        this.updateArrowPosition(last.x, last.y, last.z, last.vx, last.vy, last.vz);
                        this.impactMarker.position.set(last.x, 0.01, last.z);
                        this.impactMarker.visible = true;
                        if (this.arrow) this.arrow.visible = false;
                        setTimeout(() => {
                            this.animating = false;
                            this.setStringPosition(0, 0, restZ);
                            resolve();
                        }, 500);
                        return;
                    }

                    const p0 = points[idx];
                    const p1 = points[idx + 1];
                    const segmentT = (currentT - p0.t) / (p1.t - p0.t);
                    const x = p0.x + (p1.x - p0.x) * segmentT;
                    const y = p0.y + (p1.y - p0.y) * segmentT;
                    const z = p0.z + (p1.z - p0.z) * segmentT;
                    const vx = p0.vx + (p1.vx - p0.vx) * segmentT;
                    const vy = p0.vy + (p1.vy - p0.vy) * segmentT;
                    const vz = p0.vz + (p1.vz - p0.vz) * segmentT;

                    const releaseT = Math.min((performance.now() - releaseStart) / 100, 1);
                    const stringZ = drawnZ + (restZ - drawnZ) * releaseT;
                    const vibrate = releaseT < 0.3 ? Math.sin(releaseT * 40) * (0.3 - releaseT) * 0.015 : 0;
                    this.setStringPosition(0, vibrate, stringZ);
                    this.updateArrowPosition(x, y, z, vx, vy, vz);
                    requestAnimationFrame(flyAnim);
                };
                requestAnimationFrame(flyAnim);
            });
        } else {
            this.animating = false;
        }
    }

    updateArrowPosition(x, y, z, vx, vy, vz) {
        if (!this.arrow || !this.crossbowGroup) return;
        const worldPos = new THREE.Vector3();
        this.crossbowGroup.getWorldPosition(worldPos);
        this.arrow.position.set(x - worldPos.x, y - worldPos.y, z - worldPos.z);

        const speed = Math.sqrt(vx * vx + vy * vy + vz * vz);
        if (speed > 0.1) {
            this.arrow.lookAt(x - worldPos.x + vx / speed, y - worldPos.y + vy / speed, z - worldPos.z + vz / speed);
            this.arrow.rotateY(Math.PI / 2);
        }
    }

    showTrajectoryPath(points) {
        if (this.trajectoryLine) this.scene.remove(this.trajectoryLine);
        if (this.arrowPath) this.scene.remove(this.arrowPath);
        if (!this.showTrajectory) return;

        const geoPoints = points.map(p => new THREE.Vector3(p.x, p.y, p.z));
        const curve = new THREE.CatmullRomCurve3(geoPoints);

        const tubeGeo = new THREE.TubeGeometry(curve, Math.min(points.length * 2, 200), 0.015, 6, false);
        this.trajectoryLine = new THREE.Mesh(tubeGeo, new THREE.MeshBasicMaterial({
            color: 0x4ade80, transparent: true, opacity: 0.6
        }));
        this.scene.add(this.trajectoryLine);

        const lineGeo = new THREE.BufferGeometry().setFromPoints(geoPoints);
        this.arrowPath = new THREE.Line(lineGeo, new THREE.LineDashedMaterial({
            color: 0xfbbf24, dashSize: 0.15, gapSize: 0.1, transparent: true, opacity: 0.7
        }));
        this.arrowPath.computeLineDistances();
        this.scene.add(this.arrowPath);
    }

    clearTrajectory() {
        if (this.trajectoryLine) { this.scene.remove(this.trajectoryLine); this.trajectoryLine = null; }
        if (this.arrowPath) { this.scene.remove(this.arrowPath); this.arrowPath = null; }
        this.impactMarker.visible = false;
        if (this.arrow) {
            this.arrow.visible = true;
            this.arrow.position.z = this.arrowRestZ;
            this.arrow.position.x = 0;
            this.arrow.position.y = 0;
        }
        this.currentDrawAmount = 0;
        this.targetDrawAmount = 0;
        this.updateArmDeformation(0);
    }

    setShowTrajectory(show) {
        this.showTrajectory = show;
        if (!show) this.clearTrajectory();
    }

    setShowGrid(show) {
        this.showGrid = show;
        if (this.gridHelper) this.gridHelper.visible = show;
    }

    setWireframe(enabled) {
        this.wireframe = enabled;
        this.scene.traverse((obj) => {
            if (obj.isMesh && obj.material) {
                if (Array.isArray(obj.material)) obj.material.forEach(m => m.wireframe = enabled);
                else obj.material.wireframe = enabled;
            }
        });
    }

    resetView() {
        const cfg = this.config.three;
        this.camera.position.set(cfg.cameraInitPos.x, cfg.cameraInitPos.y, cfg.cameraInitPos.z);
        this.controls.target.set(cfg.cameraTarget.x, cfg.cameraTarget.y, cfg.cameraTarget.z);
        this.controls.update();
    }

    updateCrossbow(crossbowData) {
        this.clearTrajectory();
        this.createCrossbow(crossbowData);
    }

    onResize() {
        const rect = this.container.getBoundingClientRect();
        this.camera.aspect = rect.width / rect.height;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(rect.width, rect.height);
    }

    // ===== Virtual Shooting Experience Methods =====

    enableShootingMode(enable, crossbowData) {
        this.shootingMode = enable;
        this.shootingCrossbowData = crossbowData || this.currentCrossbow;
        if (enable) {
            this.controls.enabled = false;
            this.createReticle();
            this.createTargetField();
            this.bindShootingEvents();
            this.resetShootingScore();
        } else {
            this.controls.enabled = true;
            this.removeReticle();
            this.clearTargets();
            this.unbindShootingEvents();
        }
    }

    createReticle() {
        if (this.reticleGroup) return;
        this.reticleGroup = new THREE.Group();

        const reticleMat = new THREE.LineBasicMaterial({ color: 0x00ff88, transparent: true, opacity: 0.85 });

        const outerRing = new THREE.LineLoop(
            new THREE.BufferGeometry().setFromPoints(
                Array.from({length: 48}, (_, i) => {
                    const a = i / 48 * Math.PI * 2;
                    return new THREE.Vector3(Math.cos(a) * 0.05, Math.sin(a) * 0.05, 0);
                })
            ), reticleMat
        );
        this.reticleGroup.add(outerRing);

        const innerRing = new THREE.LineLoop(
            new THREE.BufferGeometry().setFromPoints(
                Array.from({length: 24}, (_, i) => {
                    const a = i / 24 * Math.PI * 2;
                    return new THREE.Vector3(Math.cos(a) * 0.012, Math.sin(a) * 0.012, 0);
                })
            ), new THREE.LineBasicMaterial({ color: 0xff4444, transparent: true, opacity: 0.9 })
        );
        this.reticleGroup.add(innerRing);

        const crossSize = 0.03;
        const crossMat = new THREE.LineBasicMaterial({ color: 0x00ff88, transparent: true, opacity: 0.7 });
        const addLine = (x1, y1, x2, y2) => {
            const g = new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(x1, y1, 0), new THREE.Vector3(x2, y2, 0)
            ]);
            this.reticleGroup.add(new THREE.Line(g, crossMat));
        };
        addLine(-crossSize, 0, -0.008, 0);
        addLine(0.008, 0, crossSize, 0);
        addLine(0, -crossSize, 0, -0.008);
        addLine(0, 0.008, 0, crossSize);

        const rangeTicks = [0.08, 0.12, 0.18];
        rangeTicks.forEach((r, i) => {
            const tickMat = new THREE.LineBasicMaterial({
                color: new THREE.Color().setHSL(0.3, 1, 0.5 - i * 0.1),
                transparent: true, opacity: 0.5
            });
            for (let j = 0; j < 4; j++) {
                const a1 = j * Math.PI / 2 - 0.05;
                const a2 = j * Math.PI / 2 + 0.05;
                const pts = [
                    new THREE.Vector3(Math.cos(a1) * r, Math.sin(a1) * r, 0),
                    new THREE.Vector3(Math.cos(a2) * r, Math.sin(a2) * r, 0)
                ];
                this.reticleGroup.add(new THREE.Line(
                    new THREE.BufferGeometry().setFromPoints(pts), tickMat
                ));
            }
        });

        const cam = this.virtualCameraMode ? this.camera : this.camera;
        const offset = new THREE.Vector3(0, 0, -0.6);
        this.reticleGroup.position.copy(offset);
        this.camera.add(this.reticleGroup);
        this.scene.add(this.camera);
    }

    removeReticle() {
        if (this.reticleGroup && this.reticleGroup.parent) {
            this.reticleGroup.parent.remove(this.reticleGroup);
        }
        this.reticleGroup = null;
    }

    createTargetField() {
        if (this.targetGroup) this.clearTargets();
        this.targetGroup = new THREE.Group();
        this.hitMarkerGroup = new THREE.Group();
        this.scene.add(this.targetGroup);
        this.scene.add(this.hitMarkerGroup);

        const cfg = this.config.shootingExperience || {
            targetCount: 8,
            minRange: 40, maxRange: 220,
            targetRadius: [1.2, 0.9, 0.7, 0.5],
            pointValues: [5, 10, 20, 50]
        };

        const targetCount = cfg.targetCount || 8;
        const minR = cfg.minRange || 40;
        const maxR = cfg.maxRange || 220;
        const radii = cfg.targetRadius || [1.2, 0.9, 0.7, 0.5];
        const points = cfg.pointValues || [5, 10, 20, 50];

        for (let i = 0; i < targetCount; i++) {
            const range = minR + Math.random() * (maxR - minR);
            const azSpread = (Math.random() - 0.5) * 50 * Math.PI / 180;
            const heightSpread = (Math.random() - 0.5) * 6;
            const target = this.createArcheryTarget(radii, points, range, azSpread, heightSpread);
            this.targets.push(target);
            this.targetGroup.add(target.group);
        }
    }

    createArcheryTarget(radii, pointValues, distance, azimuthRad, heightOffset) {
        const group = new THREE.Group();
        const colors = [0xffffff, 0x1e40af, 0x059669, 0xdc2626, 0xfbbf24];

        const rings = [];
        for (let r = 0; r < radii.length; r++) {
            const ringColor = colors[Math.min(r, colors.length - 1)];
            const ringGeo = new THREE.CircleGeometry(radii[r], 48);
            const ringMat = new THREE.MeshBasicMaterial({
                color: ringColor,
                side: THREE.DoubleSide,
                transparent: true,
                opacity: 0.95
            });
            const ring = new THREE.Mesh(ringGeo, ringMat);
            ring.userData.ringIndex = r;
            ring.userData.pointValue = pointValues[r] || (r + 1) * 5;
            rings.push(ring);
            group.add(ring);

            if (r > 0) {
                const outlineGeo = new THREE.RingGeometry(radii[r] - 0.02, radii[r], 48);
                const outlineMat = new THREE.MeshBasicMaterial({
                    color: 0x111111, side: THREE.DoubleSide, transparent: true, opacity: 0.5
                });
                group.add(new THREE.Mesh(outlineGeo, outlineMat));
            }
        }

        const numberGeo = new THREE.CircleGeometry(radii[radii.length - 1] * 0.3, 24);
        const numberMat = new THREE.MeshBasicMaterial({
            color: 0xfbbf24, side: THREE.DoubleSide, transparent: true, opacity: 0.9
        });
        const bullseye = new THREE.Mesh(numberGeo, numberMat);
        bullseye.userData.ringIndex = radii.length;
        bullseye.userData.pointValue = pointValues[pointValues.length - 1] || 100;
        rings.push(bullseye);
        group.add(bullseye);

        const standGeo = new THREE.BoxGeometry(0.1, Math.max(1, radii[0] * 2), 0.1);
        const standMat = new THREE.MeshStandardMaterial({ color: 0x8b4513, roughness: 0.8 });
        const stand = new THREE.Mesh(standGeo, standMat);
        stand.position.y = -radii[0] - standGeo.parameters.height / 2 + 0.1;
        group.add(stand);

        const x = Math.sin(azimuthRad) * distance;
        const z = -Math.cos(azimuthRad) * distance;
        const y = heightOffset + radii[0] + 0.5;
        group.position.set(x, y, z);
        group.lookAt(0, y, 10);

        return {
            group, rings,
            distance,
            centerWorld: new THREE.Vector3(x, y, z),
            radii,
            pointValues,
            active: true,
            hitCount: 0
        };
    }

    clearTargets() {
        if (this.targetGroup && this.targetGroup.parent) {
            this.targetGroup.parent.remove(this.targetGroup);
        }
        if (this.hitMarkerGroup && this.hitMarkerGroup.parent) {
            this.hitMarkerGroup.parent.remove(this.hitMarkerGroup);
        }
        this.targetGroup = null;
        this.hitMarkerGroup = null;
        this.targets = [];
        this.hitMarkers = [];
    }

    bindShootingEvents() {
        const canvas = this.renderer.domElement;
        this._onMouseMove = (e) => this.handleMouseMove(e);
        this._onMouseDown = (e) => this.handleMouseDown(e);
        this._onMouseUp = (e) => this.handleMouseUp(e);
        this._onKeyDown = (e) => this.handleKeyDown(e);
        this._onTouchStart = (e) => { e.preventDefault(); this.handleMouseDown(e.touches[0]); };
        this._onTouchMove = (e) => { e.preventDefault(); this.handleMouseMove(e.touches[0]); };
        this._onTouchEnd = (e) => { e.preventDefault(); this.handleMouseUp(e.changedTouches[0]); };

        canvas.addEventListener('mousemove', this._onMouseMove);
        canvas.addEventListener('mousedown', this._onMouseDown);
        window.addEventListener('mouseup', this._onMouseUp);
        window.addEventListener('keydown', this._onKeyDown);
        canvas.addEventListener('touchstart', this._onTouchStart, { passive: false });
        canvas.addEventListener('touchmove', this._onTouchMove, { passive: false });
        canvas.addEventListener('touchend', this._onTouchEnd, { passive: false });
    }

    unbindShootingEvents() {
        const canvas = this.renderer.domElement;
        if (this._onMouseMove) canvas.removeEventListener('mousemove', this._onMouseMove);
        if (this._onMouseDown) canvas.removeEventListener('mousedown', this._onMouseDown);
        if (this._onMouseUp) window.removeEventListener('mouseup', this._onMouseUp);
        if (this._onKeyDown) window.removeEventListener('keydown', this._onKeyDown);
        if (this._onTouchStart) canvas.removeEventListener('touchstart', this._onTouchStart);
        if (this._onTouchMove) canvas.removeEventListener('touchmove', this._onTouchMove);
        if (this._onTouchEnd) canvas.removeEventListener('touchend', this._onTouchEnd);
    }

    handleMouseMove(e) {
        if (!this.shootingMode) return;
        const rect = this.renderer.domElement.getBoundingClientRect();
        const cx = (e.clientX - rect.left) / rect.width;
        const cy = (e.clientY - rect.top) / rect.height;

        const sensX = (this.config.shootingExperience?.aimSensitivity || 1.5);
        const sensY = sensX * 0.7;
        const centerX = 0.5;
        const centerY = 0.5;

        this.aimAzimuthDeg += (cx - centerX) * sensX * 90 / rect.width * rect.width;
        this.aimElevationDeg -= (cy - centerY) * sensY * 60 / rect.height * rect.height;

        this.aimAzimuthDeg = ((this.aimAzimuthDeg + 180) % 360 + 360) % 360 - 180;
        this.aimElevationDeg = Math.max(-5, Math.min(60, this.aimElevationDeg));

        if (this.crossbowGroup) {
            this.crossbowGroup.rotation.y = this.aimAzimuthDeg * Math.PI / 180;
            this.crossbowGroup.rotation.x = this.aimElevationDeg * Math.PI / 180;
        }
    }

    handleMouseDown(e) {
        if (!this.shootingMode || this.isDrawing) return;
        this.isDrawing = true;
        this.targetDrawAmount = 1;
        this.drawingStartTime = performance.now();
        this.drawPowerPercent = 0;

        if (this.virtualCameraMode) {
            this.savedCameraPos.copy(this.camera.position);
            this.savedCameraTarget.copy(this.controls.target);
            const offset = new THREE.Vector3(0.25, 0.25, -0.4);
            offset.applyEuler(new THREE.Euler(0, this.aimAzimuthDeg * Math.PI / 180, 0));
            this.camera.position.lerp(offset, 0.5);
        }
    }

    handleMouseUp(e) {
        if (!this.shootingMode || !this.isDrawing) return;
        this.isDrawing = false;

        const drawTimeMs = performance.now() - this.drawingStartTime;
        const minDrawTime = this.config.shootingExperience?.minDrawTimeMs || 300;
        const fullDrawTime = this.config.shootingExperience?.fullDrawTimeMs || 1200;

        if (drawTimeMs < minDrawTime) {
            this.targetDrawAmount = 0;
            this.drawPowerPercent = 0;
            return;
        }

        this.drawPowerPercent = Math.min(1, drawTimeMs / fullDrawTime);
        this.performVirtualShot();
    }

    handleKeyDown(e) {
        if (!this.shootingMode) return;
        const step = e.shiftKey ? 0.5 : 2;
        if (e.code === 'ArrowLeft') this.aimAzimuthDeg -= step;
        if (e.code === 'ArrowRight') this.aimAzimuthDeg += step;
        if (e.code === 'ArrowUp') this.aimElevationDeg = Math.min(60, this.aimElevationDeg + step);
        if (e.code === 'ArrowDown') this.aimElevationDeg = Math.max(-5, this.aimElevationDeg - step);
        if (e.code === 'Space') {
            e.preventDefault();
            if (!this.isDrawing) this.handleMouseDown({ clientX: 0, clientY: 0 });
        }
        if (e.code === 'KeyR') {
            this.createTargetField();
            this.resetShootingScore();
        }
        this.aimAzimuthDeg = ((this.aimAzimuthDeg + 180) % 360 + 360) % 360 - 180;
        if (this.crossbowGroup) {
            this.crossbowGroup.rotation.y = this.aimAzimuthDeg * Math.PI / 180;
            this.crossbowGroup.rotation.x = this.aimElevationDeg * Math.PI / 180;
        }
    }

    performVirtualShot() {
        if (this.isAnimatingShot) return;
        this.shotsFired++;

        const cb = this.shootingCrossbowData || this.currentCrossbow;
        const drawWeightKg = cb?.draw_weight_kg || 180;
        const efficiency = 0.78;
        const drawDistance = (cb?.bow_arm_length || 1.5) * 0.75;
        const arrowMassKg = (cb?.arrow_mass_g || 70) / 1000;
        const powerFactor = Math.pow(this.drawPowerPercent || 1, 0.7);

        const drawEnergy = 0.5 * drawWeightKg * 9.81 * drawDistance * (2 / 3) * powerFactor;
        const v0 = Math.sqrt(2 * drawEnergy * efficiency / arrowMassKg);
        const elevRad = this.aimElevationDeg * Math.PI / 180;
        const azRad = this.aimAzimuthDeg * Math.PI / 180;

        const points = this.computeLocalTrajectory(v0, elevRad, 250, arrowMassKg);
        const worldPoints = this.transformTrajectoryToWorld(points, azRad);
        this.lastTrajectoryPoints = worldPoints;
        this.renderTrajectoryVisual(worldPoints);

        const impactInfo = this.checkTargetHits(worldPoints);
        const finalImpact = this.findFinalImpactPoint(worldPoints);

        setTimeout(() => {
            this.playUserShotAnimation(worldPoints, finalImpact, impactInfo);
        }, 50);

        this.shakeIntensity = 0.04 + this.drawPowerPercent * 0.06;
        setTimeout(() => { this.targetDrawAmount = 0; this.drawPowerPercent = 0; }, 200);

        if (this.onShootCallback) {
            this.onShootCallback({
                velocity: v0,
                power: this.drawPowerPercent,
                elevation: this.aimElevationDeg,
                azimuth: this.aimAzimuthDeg,
                impact: impactInfo,
                shotNumber: this.shotsFired
            });
        }
    }

    computeLocalTrajectory(v0, elevRad, maxSteps, arrowMassKg) {
        const g = 9.81;
        const rho = 1.225;
        const area = 0.00025;
        const Cd = 1.2;
        const dt = 0.004;
        const points = [];

        let vx = v0 * Math.cos(elevRad);
        let vy = v0 * Math.sin(elevRad);
        let x = 0, y = 0, z = 0;
        const t0 = performance.now();

        for (let i = 0; i < maxSteps * 250; i++) {
            points.push({ x, y, z, vx, vy, t: i * dt });
            const v = Math.sqrt(vx * vx + vy * vy);
            if (v <= 0.01) break;
            const FD = 0.5 * rho * v * v * area * Cd;
            const ax = -FD / arrowMassKg * (vx / v);
            const ay = -FD / arrowMassKg * (vy / v) - g;
            vx += ax * dt;
            vy += ay * dt;
            x += vx * dt;
            y += vy * dt;

            if (y < -0.05 || x > 600) break;
        }
        return points;
    }

    transformTrajectoryToWorld(localPoints, azRad) {
        return localPoints.map(p => {
            const wx = Math.sin(azRad) * p.x;
            const wz = -Math.cos(azRad) * p.x;
            return { x: wx, y: p.y, z: wz, t: p.t };
        });
    }

    renderTrajectoryVisual(worldPoints) {
        if (!this.showTrajectory || worldPoints.length < 2) return;
        try {
            const simplePoints = worldPoints.map(p => new THREE.Vector3(p.x, p.y + 0.1, p.z));
            this.drawTrajectory(simplePoints, 0);
            if (this.arrowPath) this.arrowPath.visible = false;
        } catch (e) {}
    }

    checkTargetHits(worldPoints) {
        let bestHit = null;
        let bestPointValue = 0;

        for (const target of this.targets) {
            if (!target.active) continue;
            const targetPos = target.group.position;

            for (const p of worldPoints) {
                const dx = p.x - targetPos.x;
                const dy = p.y - targetPos.y;
                const dz = p.z - targetPos.z;
                const distSq = dx * dx + dy * dy + dz * dz;

                for (let r = target.radii.length - 1; r >= 0; r--) {
                    if (distSq < target.radii[r] * target.radii[r]) {
                        const pv = target.pointValues[r] || 5;
                        if (pv > bestPointValue) {
                            bestPointValue = pv;
                            bestHit = {
                                target,
                                ring: r,
                                pointValue: pv,
                                impactPoint: new THREE.Vector3(p.x, p.y, p.z),
                                distance: p.x
                            };
                        }
                        break;
                    }
                }
                if (bestHit) break;
            }
        }
        return bestHit;
    }

    findFinalImpactPoint(worldPoints) {
        for (let i = 1; i < worldPoints.length; i++) {
            const prev = worldPoints[i - 1];
            const curr = worldPoints[i];
            if (prev.y > 0 && curr.y <= 0) {
                const t = prev.y / (prev.y - curr.y);
                return {
                    x: prev.x + t * (curr.x - prev.x),
                    y: 0.02,
                    z: prev.z + t * (curr.z - prev.z)
                };
            }
        }
        return worldPoints[worldPoints.length - 1] || { x: 0, y: 0.02, z: 0 };
    }

    playUserShotAnimation(worldPoints, finalImpact, hitInfo) {
        const totalTime = (worldPoints.length > 0 ? worldPoints[worldPoints.length - 1].t : 2) * 1000;
        const startT = performance.now();
        let idx = 0;

        this.isAnimatingShot = true;
        this.playDrawAnimation(() => {
            this.playReleaseAnimation();

            const tick = () => {
                const elapsed = performance.now() - startT;
                const desiredT = elapsed / 1000;

                while (idx < worldPoints.length - 1 && worldPoints[idx].t < desiredT) idx++;
                if (idx >= worldPoints.length) idx = worldPoints.length - 1;

                const p = worldPoints[idx];
                if (this.arrow) {
                    this.arrow.visible = true;
                    this.arrow.position.set(p.x, p.y + 0.12, p.z);

                    if (idx < worldPoints.length - 1) {
                        const next = worldPoints[idx + 1];
                        const dir = new THREE.Vector3(
                            next.x - p.x, next.y - p.y, next.z - p.z
                        ).normalize();
                        this.arrow.lookAt(p.x + dir.x, p.y + dir.y, p.z + dir.z);
                        this.arrow.rotateX(Math.PI / 2);
                    }
                }

                if (idx < worldPoints.length - 1) {
                    requestAnimationFrame(tick);
                } else {
                    if (this.arrow) {
                        this.arrow.position.set(finalImpact.x, finalImpact.y + 0.05, finalImpact.z);
                    }
                    this.isAnimatingShot = false;

                    if (hitInfo) {
                        this.registerHit(hitInfo);
                    } else {
                        this.spawnImpactMarker(finalImpact);
                    }
                }
            };
            tick();
        });
    }

    registerHit(hitInfo) {
        this.hitsScored++;
        this.shootingScore += hitInfo.pointValue;

        const color = hitInfo.ring === 0 ? 0xfbbf24 :
                        hitInfo.ring === 1 ? 0xdc2626 :
                        hitInfo.ring === 2 ? 0x059669 :
                        hitInfo.ring === 3 ? 0x1e40af : 0xffffff;
        this.spawnHitMarker(hitInfo, color);

        hitInfo.target.hitCount++;
        if (hitInfo.target.hitCount >= (this.config.shootingExperience?.hitsPerTarget || 3)) {
            this.animateTargetDestruction(hitInfo.target);
        }

        this.shakeIntensity = 0.02;
        if (this.onHitCallback) {
            this.onHitCallback({
                points: hitInfo.pointValue,
                ring: hitInfo.ring,
                targetDistance: hitInfo.distance,
                totalScore: this.shootingScore,
                hits: this.hitsScored,
                shots: this.shotsFired,
                accuracy: this.hitsScored / Math.max(1, this.shotsFired)
            });
        }
    }

    spawnHitMarker(hitInfo, color) {
        const group = new THREE.Group();
        const geo = new THREE.SphereGeometry(0.08, 16, 16);
        const mat = new THREE.MeshBasicMaterial({ color, transparent: true, opacity: 1 });
        const sphere = new THREE.Mesh(geo, mat);
        group.add(sphere);

        const ringGeo = new THREE.RingGeometry(0.05, 0.12, 32);
        const ringMat = new THREE.MeshBasicMaterial({
            color, transparent: true, opacity: 0.8, side: THREE.DoubleSide
        });
        const ring = new THREE.Mesh(ringGeo, ringMat);
        ring.lookAt(0, 0, 10);
        group.add(ring);

        group.position.copy(hitInfo.impactPoint);
        if (this.hitMarkerGroup) this.hitMarkerGroup.add(group);
        this.hitMarkers.push(group);

        const start = performance.now();
        const animateMarker = () => {
            const t = (performance.now() - start) / 1000;
            const s = 1 + t * 2;
            sphere.scale.setScalar(Math.max(0.1, 1 - t * 0.8));
            ring.scale.setScalar(s);
            ringMat.opacity = Math.max(0, 0.8 - t * 0.8);
            mat.opacity = Math.max(0, 1 - t);
            if (t < 1.2) requestAnimationFrame(animateMarker);
        };
        animateMarker();
    }

    spawnImpactMarker(point) {
        this.impactMarker.position.set(point.x, point.y + 0.02, point.z);
        this.impactMarker.visible = true;
    }

    animateTargetDestruction(target) {
        const start = performance.now();
        const startPos = target.group.position.clone();
        const animate = () => {
            const t = (performance.now() - start) / 1200;
            target.group.scale.setScalar(Math.max(0.1, 1 - t * 1.2));
            target.group.rotation.z = t * 4;
            target.group.position.y = startPos.y - t * t * 10;
            target.group.traverse(o => {
                if (o.material && 'opacity' in o.material) o.material.opacity = Math.max(0, 0.9 - t);
            });
            if (t < 1.2) requestAnimationFrame(animate);
            else target.active = false;
        };
        animate();
    }

    resetShootingScore() {
        this.shootingScore = 0;
        this.shotsFired = 0;
        this.hitsScored = 0;
    }

    getShootingStats() {
        return {
            score: this.shootingScore,
            shots: this.shotsFired,
            hits: this.hitsScored,
            accuracy: this.hitsScored / Math.max(1, this.shotsFired),
            avgPointsPerHit: this.hitsScored > 0 ? this.shootingScore / this.hitsScored : 0,
            aimAzimuth: this.aimAzimuthDeg,
            aimElevation: this.aimElevationDeg,
            drawPower: this.drawPowerPercent,
            targetsRemaining: this.targets.filter(t => t.active).length
        };
    }

    animate() {
        requestAnimationFrame(() => this.animate());
        this.controls.update();

        if (this.shootingMode && this.isDrawing) {
            const now = performance.now();
            const drawMs = now - this.drawingStartTime;
            const full = this.config.shootingExperience?.fullDrawTimeMs || 1200;
            this.drawPowerPercent = Math.min(1, drawMs / full);
        }

        if (this.shakeIntensity > 0.001) {
            this.camera.position.x += (Math.random() - 0.5) * this.shakeIntensity;
            this.camera.position.y += (Math.random() - 0.5) * this.shakeIntensity;
            this.shakeIntensity *= 0.85;
        }

        this.renderer.render(this.scene, this.camera);
    }
}
