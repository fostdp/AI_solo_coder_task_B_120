var SightOptimizerPanel = (function() {
    function SightOptimizerPanel(containerElement, apiBaseUrl) {
        this.container = containerElement;
        this.apiBase = apiBaseUrl || (window.CROSSBOW_CONFIG ? window.CROSSBOW_CONFIG.apiBase : 'http://localhost:8080/api');
        this.sightTypes = [];
        this.selectedSight = null;
        this.errorBudget = null;
        this.chart = null;
        this.currentCEP = null;
        this.basicCEP = 3.5;
        this.render();
        this.loadSightTypes();
    }

    SightOptimizerPanel.prototype.render = function() {
        var self = this;
        var html = '';
        html += '<div class="panel-section">';
        html += '<h3>瞄准具优化器</h3>';
        html += '<div style="margin-bottom:12px;">';
        html += '<label style="display:block;font-size:12px;color:#94a3b8;margin-bottom:6px;">瞄准具类型</label>';
        html += '<select id="so-sight-select" class="select-input">';
        html += '<option value="">加载中...</option>';
        html += '</select>';
        html += '</div>';
        html += '<div class="slider-group">';
        html += '<label>目标射程: <span id="so-range-value">150</span> m</label>';
        html += '<input type="range" id="so-range-slider" min="50" max="500" step="10" value="150">';
        html += '</div>';
        html += '<div class="slider-group">';
        html += '<label>风速: <span id="so-wind-value">5.0</span> m/s</label>';
        html += '<input type="range" id="so-wind-slider" min="0" max="20" step="0.5" value="5">';
        html += '</div>';
        html += '<button id="so-btn-optimize" class="btn btn-primary">优化瞄准方案</button>';
        html += '<button id="so-btn-error-budget" class="btn btn-secondary">显示误差预算</button>';
        html += '</div>';
        html += '<div class="panel-section">';
        html += '<h3>优化结果</h3>';
        html += '<div id="so-results" style="font-size:12px;color:#cbd5e1;line-height:1.8;">';
        html += '<div style="color:#64748b;text-align:center;padding:20px 0;">请配置参数并点击"优化瞄准方案"</div>';
        html += '</div>';
        html += '</div>';
        html += '<div class="panel-section">';
        html += '<h3>误差预算雷达图</h3>';
        html += '<div class="chart-container" style="height:320px;padding:0;">';
        html += '<canvas id="so-radar-chart"></canvas>';
        html += '</div>';
        html += '</div>';
        this.container.innerHTML = html;

        var rangeSlider = document.getElementById('so-range-slider');
        var windSlider = document.getElementById('so-wind-slider');
        if (rangeSlider) {
            rangeSlider.addEventListener('input', function(e) {
                document.getElementById('so-range-value').textContent = e.target.value;
            });
        }
        if (windSlider) {
            windSlider.addEventListener('input', function(e) {
                document.getElementById('so-wind-value').textContent = parseFloat(e.target.value).toFixed(1);
            });
        }

        var btnOpt = document.getElementById('so-btn-optimize');
        var btnBud = document.getElementById('so-btn-error-budget');
        if (btnOpt) {
            btnOpt.addEventListener('click', function() {
                var r = parseFloat(document.getElementById('so-range-slider').value);
                var w = parseFloat(document.getElementById('so-wind-slider').value);
                self.optimize(r, w);
            });
        }
        if (btnBud) {
            btnBud.addEventListener('click', function() {
                self.showErrorBudget();
            });
        }
    };

    SightOptimizerPanel.prototype.loadSightTypes = function() {
        var self = this;
        var url = this.apiBase + '/sight-types';
        fetch(url).then(function(res) {
            return res.json();
        }).then(function(data) {
            self.sightTypes = data;
            self.populateSelect();
        }).catch(function(e) {
            console.error('加载瞄准具类型失败:', e);
            self.sightTypes = [
                { id: 'BASIC_NOTCH', name: '简易缺口式', cep_mrad: 3.5, description: '最基础的瞄准方式' },
                { id: 'MULTI_PIN', name: '多针式', cep_mrad: 2.2, description: '多距离刻度针' },
                { id: 'PIN_SIGHT', name: '单针式', cep_mrad: 2.8, description: '可调单针瞄准' },
                { id: 'PRECISION_PEEP', name: '精密觇孔式', cep_mrad: 1.5, description: '高精度觇孔系统' }
            ];
            self.populateSelect();
        });
    };

    SightOptimizerPanel.prototype.populateSelect = function() {
        var sel = document.getElementById('so-sight-select');
        if (!sel) return;
        sel.innerHTML = '<option value="">请选择瞄准具类型</option>';
        for (var i = 0; i < this.sightTypes.length; i++) {
            var s = this.sightTypes[i];
            sel.innerHTML += '<option value="' + s.id + '">' + s.name + ' (CEP ' + s.cep_mrad + 'mrad)</option>';
        }
    };

    SightOptimizerPanel.prototype.optimize = function(targetRange, wind) {
        var self = this;
        var sightId = document.getElementById('so-sight-select').value;
        if (!sightId) {
            alert('请先选择瞄准具类型');
            return;
        }
        var url = this.apiBase + '/optimize-sight?sight_id=' + sightId + '&range=' + targetRange + '&wind=' + wind;
        fetch(url).then(function(res) {
            return res.json();
        }).then(function(data) {
            self.currentCEP = data.cep_mrad;
            self.errorBudget = data.error_budget;
            self.displayResults(data);
            self.renderRadarChart();
        }).catch(function(e) {
            console.error('优化失败,使用本地模拟:', e);
            var sight = null;
            for (var i = 0; i < self.sightTypes.length; i++) {
                if (self.sightTypes[i].id === sightId) { sight = self.sightTypes[i]; break; }
            }
            if (!sight) sight = { cep_mrad: 3.5, name: '未知' };
            var windFactor = 1 + (wind / 20) * 0.3;
            var rangeFactor = 1 + (Math.abs(targetRange - 150) / 350) * 0.2;
            self.currentCEP = sight.cep_mrad * windFactor * rangeFactor;
            self.errorBudget = {
                visual_acuity: sight.cep_mrad * 0.25,
                parallax: sight.cep_mrad * 0.18,
                refraction: sight.cep_mrad * 0.12 + wind * 0.03,
                thermal: sight.cep_mrad * 0.10,
                mechanical: sight.cep_mrad * 0.22,
                glare: sight.cep_mrad * 0.13
            };
            self.displayResults({
                sight_name: sight.name,
                cep_mrad: self.currentCEP,
                range_m: targetRange,
                wind_m_s: wind,
                recommended_adjustment: 0.002 * targetRange,
                optimal_elevation_mil: (targetRange * 0.0015).toFixed(3)
            });
            self.renderRadarChart();
        });
    };

    SightOptimizerPanel.prototype.displayResults = function(data) {
        var resEl = document.getElementById('so-results');
        if (!resEl) return;
        var improvement = 0;
        if (this.currentCEP && this.basicCEP > 0) {
            improvement = ((this.basicCEP - this.currentCEP) / this.basicCEP) * 100;
        }
        var html = '';
        html += '<div class="info-row"><span>瞄准具:</span><span style="color:#f4a460;font-weight:500;">' + data.sight_name + '</span></div>';
        html += '<div class="info-row"><span>目标射程:</span><span>' + data.range_m + ' m</span></div>';
        html += '<div class="info-row"><span>风速:</span><span>' + data.wind_m_s + ' m/s</span></div>';
        html += '<div class="info-row"><span>圆概率误差 CEP:</span><span style="color:#4ade80;font-weight:600;font-family:Consolas,monospace;">' + data.cep_mrad.toFixed(2) + ' mrad</span></div>';
        html += '<div class="info-row"><span>对比基础缺口:</span><span style="color:' + (improvement >= 0 ? '#4ade80' : '#ef4444') + ';font-weight:600;">' + (improvement >= 0 ? '+' : '') + improvement.toFixed(1) + '%</span></div>';
        html += '<div class="info-row"><span>推荐仰角调整:</span><span style="font-family:Consolas,monospace;">' + data.optimal_elevation_mil + ' mil</span></div>';
        html += '<div class="info-row"><span>风偏修正:</span><span style="font-family:Consolas,monospace;">' + (data.wind_m_s * 0.015 * data.range_m / 100).toFixed(3) + ' mil</span></div>';
        if (data.recommended_adjustment !== undefined) {
            html += '<div class="info-row"><span>望山刻度调整:</span><span style="font-family:Consolas,monospace;">' + (data.recommended_adjustment * 1000).toFixed(2) + ' mm</span></div>';
        }
        resEl.innerHTML = html;
    };

    SightOptimizerPanel.prototype.showErrorBudget = function() {
        if (!this.errorBudget) {
            this.errorBudget = {
                visual_acuity: 0.88,
                parallax: 0.63,
                refraction: 0.42,
                thermal: 0.35,
                mechanical: 0.77,
                glare: 0.45
            };
        }
        this.renderRadarChart();
        var resEl = document.getElementById('so-results');
        if (!resEl) return;
        var html = '';
        html += '<div style="margin-bottom:8px;color:#4a9eff;font-weight:500;">误差预算分解 (mrad):</div>';
        html += '<div class="info-row"><span>视觉锐度 (Visual):</span><span style="font-family:Consolas,monospace;color:#cbd5e1;">' + this.errorBudget.visual_acuity.toFixed(3) + '</span></div>';
        html += '<div class="info-row"><span>视差误差 (Parallax):</span><span style="font-family:Consolas,monospace;color:#cbd5e1;">' + this.errorBudget.parallax.toFixed(3) + '</span></div>';
        html += '<div class="info-row"><span>大气折射 (Refraction):</span><span style="font-family:Consolas,monospace;color:#cbd5e1;">' + this.errorBudget.refraction.toFixed(3) + '</span></div>';
        html += '<div class="info-row"><span>热扰动 (Thermal):</span><span style="font-family:Consolas,monospace;color:#cbd5e1;">' + this.errorBudget.thermal.toFixed(3) + '</span></div>';
        html += '<div class="info-row"><span>机械公差 (Mechanical):</span><span style="font-family:Consolas,monospace;color:#cbd5e1;">' + this.errorBudget.mechanical.toFixed(3) + '</span></div>';
        html += '<div class="info-row"><span>眩光干扰 (Glare):</span><span style="font-family:Consolas,monospace;color:#cbd5e1;">' + this.errorBudget.glare.toFixed(3) + '</span></div>';
        if (resEl.innerHTML.indexOf('瞄准具') === -1) {
            resEl.innerHTML = html;
        } else {
            resEl.innerHTML += '<hr style="border:0;border-top:1px solid rgba(74,158,255,0.15);margin:12px 0;">' + html;
        }
    };

    SightOptimizerPanel.prototype.renderRadarChart = function() {
        var ctx = document.getElementById('so-radar-chart');
        if (!ctx) return;
        ctx = ctx.getContext('2d');

        var eb = this.errorBudget || {
            visual_acuity: 0.88, parallax: 0.63, refraction: 0.42,
            thermal: 0.35, mechanical: 0.77, glare: 0.45
        };

        var labels = ['视觉锐度', '视差误差', '大气折射', '热扰动', '机械公差', '眩光干扰'];
        var data = [eb.visual_acuity, eb.parallax, eb.refraction, eb.thermal, eb.mechanical, eb.glare];
        var maxVal = 0;
        for (var i = 0; i < data.length; i++) { if (data[i] > maxVal) maxVal = data[i]; }
        maxVal = maxVal * 1.2;

        if (this.chart) {
            this.chart.destroy();
        }

        this.chart = new Chart(ctx, {
            type: 'radar',
            data: {
                labels: labels,
                datasets: [{
                    label: '误差分量 (mrad)',
                    data: data,
                    borderColor: '#4a9eff',
                    backgroundColor: 'rgba(74,158,255,0.2)',
                    borderWidth: 2,
                    pointBackgroundColor: '#4a9eff',
                    pointBorderColor: '#fff',
                    pointHoverBackgroundColor: '#fff',
                    pointHoverBorderColor: '#4a9eff',
                    pointRadius: 4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    title: {
                        display: true,
                        text: '误差预算分量雷达图',
                        color: '#f4a460',
                        font: { size: 13 }
                    },
                    legend: { display: false }
                },
                scales: {
                    r: {
                        beginAtZero: true,
                        max: maxVal,
                        angleLines: { color: 'rgba(74,158,255,0.15)' },
                        grid: { color: 'rgba(74,158,255,0.1)' },
                        pointLabels: {
                            color: '#94a3b8',
                            font: { size: 11 }
                        },
                        ticks: {
                            color: '#64748b',
                            backdropColor: 'transparent',
                            font: { size: 10 }
                        }
                    }
                }
            }
        });
    };

    return SightOptimizerPanel;
})();
