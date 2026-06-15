var VirtualShootingPanel = (function() {
    function VirtualShootingPanel(containerElement, apiBaseUrl) {
        this.container = containerElement;
        this.apiBase = apiBaseUrl || (window.CROSSBOW_CONFIG ? window.CROSSBOW_CONFIG.apiBase : 'http://localhost:8080/api');
        this.canvas = null;
        this.canvasCtx = null;
        this.sessionActive = false;
        this.targets = [];
        this.aimX = 0;
        this.aimY = 0;
        this.shots = 0;
        this.hits = 0;
        this.score = 0;
        this.sessionId = null;
        this.impacts = [];
        this.keyHandler = null;
        this.render();
        this.bindEvents();
    }

    VirtualShootingPanel.prototype.render = function() {
        var self = this;
        var html = '';
        html += '<div class="panel-section">';
        html += '<h3>虚拟射击训练</h3>';
        html += '<div style="display:flex;gap:8px;margin-bottom:12px;">';
        html += '<button id="vs-btn-start" class="btn btn-primary" style="flex:1;">开始训练</button>';
        html += '<button id="vs-btn-stop" class="btn btn-secondary" style="flex:1;" disabled>结束训练</button>';
        html += '</div>';
        html += '<div style="background:rgba(15,23,42,0.8);border-radius:8px;padding:6px;position:relative;">';
        html += '<canvas id="vs-canvas" width="600" height="420" style="width:100%;height:auto;display:block;border-radius:6px;cursor:crosshair;"></canvas>';
        html += '<div style="position:absolute;bottom:12px;left:12px;font-size:11px;color:#94a3b8;background:rgba(15,23,42,0.75);padding:4px 10px;border-radius:4px;">';
        html += '点击瞄准 · 空格键射击';
        html += '</div>';
        html += '</div>';
        html += '</div>';
        html += '<div class="panel-section">';
        html += '<h3>射击统计</h3>';
        html += '<div class="accuracy-stats" style="grid-template-columns:repeat(4,1fr);padding:0;">';
        html += '<div class="stat-card">';
        html += '<div class="stat-label">总射击</div>';
        html += '<div class="stat-value" id="vs-shots">0</div>';
        html += '</div>';
        html += '<div class="stat-card">';
        html += '<div class="stat-label">命中数</div>';
        html += '<div class="stat-value" id="vs-hits" style="color:#4ade80;">0</div>';
        html += '</div>';
        html += '<div class="stat-card">';
        html += '<div class="stat-label">准确率</div>';
        html += '<div class="stat-value" id="vs-accuracy" style="color:#fbbf24;">0%</div>';
        html += '</div>';
        html += '<div class="stat-card">';
        html += '<div class="stat-label">得分</div>';
        html += '<div class="stat-value" id="vs-score" style="color:#a855f7;">0</div>';
        html += '</div>';
        html += '</div>';
        html += '</div>';
        this.container.innerHTML = html;

        this.canvas = document.getElementById('vs-canvas');
        if (this.canvas) {
            this.canvasCtx = this.canvas.getContext('2d');
            this.drawCanvas();
        }

        var btnStart = document.getElementById('vs-btn-start');
        var btnStop = document.getElementById('vs-btn-stop');
        if (btnStart) {
            btnStart.addEventListener('click', function() { self.startSession(); });
        }
        if (btnStop) {
            btnStop.addEventListener('click', function() { self.stopSession(); });
        }
    };

    VirtualShootingPanel.prototype.bindEvents = function() {
        var self = this;
        if (this.canvas) {
            this.canvas.addEventListener('click', function(e) {
                var rect = self.canvas.getBoundingClientRect();
                var scaleX = self.canvas.width / rect.width;
                var scaleY = self.canvas.height / rect.height;
                var x = (e.clientX - rect.left) * scaleX;
                var y = (e.clientY - rect.top) * scaleY;
                self.aimAt(x, y);
            });
        }

        this.keyHandler = function(e) {
            if (e.code === 'Space' || e.keyCode === 32) {
                e.preventDefault();
                if (self.sessionActive) self.fire();
            }
        };
        document.addEventListener('keydown', this.keyHandler);
    };

    VirtualShootingPanel.prototype.startSession = function() {
        var self = this;
        var btnStart = document.getElementById('vs-btn-start');
        var btnStop = document.getElementById('vs-btn-stop');

        var url = this.apiBase + '/start-shooting-session';
        fetch(url, { method: 'POST' }).then(function(res) {
            return res.json();
        }).then(function(data) {
            self.sessionId = data.session_id || Date.now();
            self.shots = 0;
            self.hits = 0;
            self.score = 0;
            self.targets = data.targets || [];
            self.impacts = [];
            self.sessionActive = true;
            if (self.targets.length === 0) self.generateDefaultTargets();
            if (btnStart) btnStart.disabled = true;
            if (btnStop) btnStop.disabled = false;
            self.updateStats();
            self.drawCanvas();
        }).catch(function(e) {
            console.error('开始训练失败,使用本地模式:', e);
            self.sessionId = 'local_' + Date.now();
            self.shots = 0;
            self.hits = 0;
            self.score = 0;
            self.targets = [];
            self.impacts = [];
            self.generateDefaultTargets();
            self.sessionActive = true;
            if (btnStart) btnStart.disabled = true;
            if (btnStop) btnStop.disabled = false;
            self.updateStats();
            self.drawCanvas();
        });
    };

    VirtualShootingPanel.prototype.generateDefaultTargets = function() {
        var w = this.canvas ? this.canvas.width : 600;
        var h = this.canvas ? this.canvas.height : 420;
        var radii = [35, 25, 18, 14];
        var points = [5, 10, 20, 35];
        var numTargets = 4;
        for (var i = 0; i < numTargets; i++) {
            var x = w * 0.2 + Math.random() * w * 0.7;
            var y = h * 0.15 + Math.random() * h * 0.55;
            this.addTarget(x, y, radii[i % radii.length], points[i % points.length]);
        }
    };

    VirtualShootingPanel.prototype.stopSession = function() {
        var self = this;
        var btnStart = document.getElementById('vs-btn-start');
        var btnStop = document.getElementById('vs-btn-stop');

        var url = this.apiBase + '/end-shooting-session?session_id=' + (this.sessionId || '');
        fetch(url, { method: 'POST' }).then(function(res) {
            return res.json();
        }).catch(function(e) {
            console.error('结束会话API调用失败:', e);
        });

        this.sessionActive = false;
        if (btnStart) btnStart.disabled = false;
        if (btnStop) btnStop.disabled = true;
        alert('训练结束!\n总射击: ' + this.shots + '\n命中: ' + this.hits + '\n准确率: ' + (this.shots > 0 ? (this.hits / this.shots * 100).toFixed(1) : 0) + '%\n得分: ' + this.score);
    };

    VirtualShootingPanel.prototype.addTarget = function(x, y, radius, pointValue) {
        this.targets.push({
            x: x,
            y: y,
            radius: radius || 20,
            pointValue: pointValue || 10,
            hitCount: 0
        });
        this.drawCanvas();
    };

    VirtualShootingPanel.prototype.aimAt = function(x, y) {
        this.aimX = x;
        this.aimY = y;
        this.drawCanvas();
    };

    VirtualShootingPanel.prototype.fire = function() {
        if (!this.sessionActive) return;
        var self = this;
        this.shots++;
        var jitterX = (Math.random() - 0.5) * 16;
        var jitterY = (Math.random() - 0.5) * 16;
        var impactX = this.aimX + jitterX;
        var impactY = this.aimY + jitterY;

        this.impacts.push({ x: impactX, y: impactY, hit: false, points: 0 });
        var impactIdx = this.impacts.length - 1;

        var hitTarget = -1;
        var hitPoints = 0;
        for (var i = 0; i < this.targets.length; i++) {
            var t = this.targets[i];
            var dx = impactX - t.x;
            var dy = impactY - t.y;
            var dist = Math.sqrt(dx * dx + dy * dy);
            if (dist < t.radius) {
                hitTarget = i;
                var ringFactor = 1 - (dist / t.radius) * 0.4;
                hitPoints = Math.round(t.pointValue * ringFactor);
                break;
            }
        }

        if (hitTarget >= 0) {
            this.hits++;
            this.score += hitPoints;
            this.targets[hitTarget].hitCount++;
            this.impacts[impactIdx].hit = true;
            this.impacts[impactIdx].points = hitPoints;
        }

        var url = this.apiBase + '/record-shot';
        fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                session_id: this.sessionId,
                aim_x: this.aimX,
                aim_y: this.aimY,
                impact_x: impactX,
                impact_y: impactY,
                hit: hitTarget >= 0,
                points: hitPoints,
                target_idx: hitTarget
            })
        }).catch(function(e) {});

        this.updateStats();
        this.drawCanvas();
    };

    VirtualShootingPanel.prototype.updateStats = function() {
        var elShots = document.getElementById('vs-shots');
        var elHits = document.getElementById('vs-hits');
        var elAcc = document.getElementById('vs-accuracy');
        var elScore = document.getElementById('vs-score');
        if (elShots) elShots.textContent = this.shots;
        if (elHits) elHits.textContent = this.hits;
        if (elAcc) {
            var acc = this.shots > 0 ? (this.hits / this.shots * 100).toFixed(1) : '0.0';
            elAcc.textContent = acc + '%';
        }
        if (elScore) elScore.textContent = this.score;
    };

    VirtualShootingPanel.prototype.drawCanvas = function() {
        var ctx = this.canvasCtx;
        if (!ctx) return;
        var w = this.canvas.width;
        var h = this.canvas.height;

        var grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, '#1e3a5f');
        grad.addColorStop(0.6, '#2d4a6f');
        grad.addColorStop(1, '#3a5a40');
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, w, h);

        ctx.strokeStyle = 'rgba(148,163,184,0.12)';
        ctx.lineWidth = 1;
        for (var gx = 0; gx < w; gx += 40) {
            ctx.beginPath();
            ctx.moveTo(gx, 0);
            ctx.lineTo(gx, h);
            ctx.stroke();
        }
        for (var gy = 0; gy < h; gy += 40) {
            ctx.beginPath();
            ctx.moveTo(0, gy);
            ctx.lineTo(w, gy);
            ctx.stroke();
        }

        ctx.fillStyle = 'rgba(100,140,80,0.25)';
        ctx.fillRect(0, h * 0.7, w, h * 0.3);
        ctx.fillStyle = 'rgba(148,163,184,0.5)';
        ctx.font = 'bold 13px Consolas, monospace';
        ctx.fillText('射击距离: ~150m', 12, 22);
        ctx.fillText('目标数: ' + this.targets.length, w - 130, 22);

        for (var i = 0; i < this.targets.length; i++) {
            var t = this.targets[i];
            var rings = 4;
            for (var r = rings; r >= 1; r--) {
                var rr = t.radius * (r / rings);
                ctx.beginPath();
                ctx.arc(t.x, t.y, rr, 0, Math.PI * 2);
                var shade = r % 2 === 0 ? '#e8e8e8' : '#e53e3e';
                ctx.fillStyle = shade;
                ctx.fill();
                ctx.strokeStyle = '#1e293b';
                ctx.lineWidth = 1;
                ctx.stroke();
            }
            ctx.beginPath();
            ctx.arc(t.x, t.y, 2, 0, Math.PI * 2);
            ctx.fillStyle = '#1e293b';
            ctx.fill();

            if (t.hitCount > 0) {
                ctx.fillStyle = '#fbbf24';
                ctx.font = 'bold 11px Consolas, monospace';
                ctx.fillText('x' + t.hitCount, t.x + t.radius + 4, t.y - t.radius);
            }
            ctx.fillStyle = '#cbd5e1';
            ctx.font = '10px Consolas, monospace';
            ctx.fillText(t.pointValue + 'pt', t.x - 12, t.y + t.radius + 14);
        }

        for (var j = 0; j < this.impacts.length; j++) {
            var imp = this.impacts[j];
            ctx.beginPath();
            ctx.arc(imp.x, imp.y, 3, 0, Math.PI * 2);
            ctx.fillStyle = imp.hit ? '#fbbf24' : '#94a3b8';
            ctx.fill();
            ctx.strokeStyle = '#0f172a';
            ctx.lineWidth = 1;
            ctx.stroke();
            if (imp.points > 0) {
                ctx.fillStyle = '#fbbf24';
                ctx.font = 'bold 10px Consolas, monospace';
                ctx.fillText('+' + imp.points, imp.x + 6, imp.y - 5);
            }
        }

        if (this.sessionActive) {
            this.drawTrajectory(ctx);
            this.drawCrosshair(ctx);
        } else {
            ctx.fillStyle = 'rgba(15,23,42,0.55)';
            ctx.fillRect(w / 2 - 130, h / 2 - 28, 260, 56);
            ctx.fillStyle = '#4a9eff';
            ctx.font = 'bold 15px Consolas, monospace';
            ctx.textAlign = 'center';
            ctx.fillText('点击"开始训练"按钮', w / 2, h / 2 - 5);
            ctx.fillStyle = '#94a3b8';
            ctx.font = '11px Consolas, monospace';
            ctx.fillText('进入虚拟射击模式', w / 2, h / 2 + 14);
            ctx.textAlign = 'left';
        }
    };

    VirtualShootingPanel.prototype.drawTrajectory = function(ctx) {
        if (!ctx) return;
        var w = this.canvas.width;
        var startX = 40;
        var startY = this.canvas.height - 30;
        var aimX = this.aimX || w / 2;
        var aimY = this.aimY || this.canvas.height / 2;

        var steps = 22;
        var prevX = startX;
        var prevY = startY;

        ctx.strokeStyle = 'rgba(74,158,255,0.6)';
        ctx.lineWidth = 1.5;
        ctx.setLineDash([5, 4]);
        ctx.beginPath();
        for (var i = 0; i <= steps; i++) {
            var t = i / steps;
            var x = startX + (aimX - startX) * t;
            var arcHeight = -80 * (1 - Math.abs(t - 0.5) * 2);
            var y = startY + (aimY - startY) * t + arcHeight * t * (1 - t) * 3;
            if (i === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
            prevX = x;
            prevY = y;
        }
        ctx.stroke();
        ctx.setLineDash([]);

        ctx.fillStyle = '#4ade80';
        ctx.beginPath();
        ctx.arc(startX, startY, 5, 0, Math.PI * 2);
        ctx.fill();
        ctx.strokeStyle = '#0f172a';
        ctx.lineWidth = 1.5;
        ctx.stroke();

        ctx.fillStyle = '#4ade80';
        ctx.font = '10px Consolas, monospace';
        ctx.fillText('弩位', startX - 12, startY + 18);
    };

    VirtualShootingPanel.prototype.drawCrosshair = function(ctx) {
        if (!ctx) return;
        var x = this.aimX;
        var y = this.aimY;
        if (x === 0 && y === 0) {
            x = this.canvas.width / 2;
            y = this.canvas.height / 2;
        }

        ctx.strokeStyle = '#ef4444';
        ctx.lineWidth = 2;

        ctx.beginPath();
        ctx.moveTo(x - 18, y);
        ctx.lineTo(x - 6, y);
        ctx.moveTo(x + 6, y);
        ctx.lineTo(x + 18, y);
        ctx.moveTo(x, y - 18);
        ctx.lineTo(x, y - 6);
        ctx.moveTo(x, y + 6);
        ctx.lineTo(x, y + 18);
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(x, y, 3, 0, Math.PI * 2);
        ctx.strokeStyle = '#ef4444';
        ctx.lineWidth = 1.5;
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(x, y, 16, 0, Math.PI * 2);
        ctx.strokeStyle = 'rgba(239,68,68,0.35)';
        ctx.lineWidth = 1;
        ctx.stroke();

        ctx.fillStyle = '#ef4444';
        ctx.font = '10px Consolas, monospace';
        ctx.fillText('X:' + Math.round(x) + ' Y:' + Math.round(y), x + 22, y - 14);
    };

    return VirtualShootingPanel;
})();
