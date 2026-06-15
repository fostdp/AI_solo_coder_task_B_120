var StringComparatorPanel = (function() {
    function StringComparatorPanel(containerElement, apiBaseUrl) {
        this.container = containerElement;
        this.apiBase = apiBaseUrl || (window.CROSSBOW_CONFIG ? window.CROSSBOW_CONFIG.apiBase : 'http://localhost:8080/api');
        this.materials = [];
        this.selectedIndex = -1;
        this.chart = null;
        this.render();
        this.loadMaterials();
    }

    StringComparatorPanel.prototype.render = function() {
        var self = this;
        var html = '';
        html += '<div class="panel-section">';
        html += '<h3>弩弦材料对比</h3>';
        html += '<div style="margin-bottom:12px;">';
        html += '<button id="sc-btn-compare" class="btn btn-primary">对比全部材料</button>';
        html += '</div>';
        html += '<div id="sc-table-container" style="overflow-x:auto;margin-bottom:16px;">';
        html += '<table style="width:100%;border-collapse:collapse;font-size:12px;">';
        html += '<thead><tr>';
        html += '<th style="padding:8px;text-align:left;color:#4a9eff;border-bottom:1px solid rgba(74,158,255,0.2);">材料名称</th>';
        html += '<th style="padding:8px;text-align:right;color:#4a9eff;border-bottom:1px solid rgba(74,158,255,0.2);">效率</th>';
        html += '<th style="padding:8px;text-align:right;color:#4a9eff;border-bottom:1px solid rgba(74,158,255,0.2);">初速 (m/s)</th>';
        html += '<th style="padding:8px;text-align:right;color:#4a9eff;border-bottom:1px solid rgba(74,158,255,0.2);">寿命 (cycles)</th>';
        html += '<th style="padding:8px;text-align:right;color:#4a9eff;border-bottom:1px solid rgba(74,158,255,0.2);">综合评分</th>';
        html += '</tr></thead>';
        html += '<tbody id="sc-material-rows">';
        html += '<tr><td colspan="5" style="padding:20px;text-align:center;color:#64748b;">点击"对比全部材料"加载数据</td></tr>';
        html += '</tbody>';
        html += '</table>';
        html += '</div>';
        html += '</div>';
        html += '<div class="panel-section">';
        html += '<h3>效率对比图表</h3>';
        html += '<div class="chart-container" style="height:280px;padding:0;">';
        html += '<canvas id="sc-efficiency-chart"></canvas>';
        html += '</div>';
        html += '</div>';
        this.container.innerHTML = html;

        var btn = document.getElementById('sc-btn-compare');
        if (btn) {
            btn.addEventListener('click', function() {
                self.compareAll();
            });
        }
    };

    StringComparatorPanel.prototype.loadMaterials = function() {
        var self = this;
        var url = this.apiBase + '/string-materials';
        fetch(url).then(function(res) {
            return res.json();
        }).then(function(data) {
            self.materials = data;
        }).catch(function(e) {
            console.error('加载材料数据失败:', e);
            self.materials = [
                { name: 'SINEW (牛筋)', energy_efficiency: 0.85, estimated_velocity_ms: 82.5, fatigue_life_cycles: 12500, overall_score: 0.88 },
                { name: 'HORN (角)', energy_efficiency: 0.78, estimated_velocity_ms: 76.3, fatigue_life_cycles: 18200, overall_score: 0.79 },
                { name: 'SILK (蚕丝)', energy_efficiency: 0.82, estimated_velocity_ms: 79.8, fatigue_life_cycles: 9800, overall_score: 0.81 },
                { name: 'HEMP (大麻)', energy_efficiency: 0.65, estimated_velocity_ms: 68.2, fatigue_life_cycles: 7500, overall_score: 0.62 },
                { name: 'LINEN (亚麻)', energy_efficiency: 0.60, estimated_velocity_ms: 64.7, fatigue_life_cycles: 6200, overall_score: 0.58 }
            ];
        });
    };

    StringComparatorPanel.prototype.compareAll = function() {
        var self = this;
        var url = this.apiBase + '/compare-string-materials';
        fetch(url).then(function(res) {
            return res.json();
        }).then(function(data) {
            self.materials = data;
            self.renderTable();
            self.renderChart();
        }).catch(function(e) {
            console.error('对比失败,使用本地数据:', e);
            if (self.materials.length === 0) {
                self.materials = [
                    { name: 'SINEW (牛筋)', energy_efficiency: 0.85, estimated_velocity_ms: 82.5, fatigue_life_cycles: 12500, overall_score: 0.88 },
                    { name: 'HORN (角)', energy_efficiency: 0.78, estimated_velocity_ms: 76.3, fatigue_life_cycles: 18200, overall_score: 0.79 },
                    { name: 'SILK (蚕丝)', energy_efficiency: 0.82, estimated_velocity_ms: 79.8, fatigue_life_cycles: 9800, overall_score: 0.81 },
                    { name: 'HEMP (大麻)', energy_efficiency: 0.65, estimated_velocity_ms: 68.2, fatigue_life_cycles: 7500, overall_score: 0.62 },
                    { name: 'LINEN (亚麻)', energy_efficiency: 0.60, estimated_velocity_ms: 64.7, fatigue_life_cycles: 6200, overall_score: 0.58 }
                ];
            }
            self.renderTable();
            self.renderChart();
        });
    };

    StringComparatorPanel.prototype.renderTable = function() {
        var self = this;
        var tbody = document.getElementById('sc-material-rows');
        if (!tbody) return;
        if (this.materials.length === 0) {
            tbody.innerHTML = '<tr><td colspan="5" style="padding:20px;text-align:center;color:#64748b;">无数据</td></tr>';
            return;
        }
        var rows = '';
        for (var i = 0; i < this.materials.length; i++) {
            var m = this.materials[i];
            var eff = (m.energy_efficiency * 100).toFixed(1);
            var vel = m.estimated_velocity_ms ? m.estimated_velocity_ms.toFixed(1) : '0.0';
            var life = m.fatigue_life_cycles ? m.fatigue_life_cycles.toLocaleString() : '0';
            var score = m.overall_score ? (m.overall_score.toFixed(3)) : '0.000';
            var rowClass = (i === this.selectedIndex) ? ' style="cursor:pointer;background:rgba(74,158,255,0.1);"' : ' style="cursor:pointer;"';
            rows += '<tr data-idx="' + i + '"' + rowClass + '>';
            rows += '<td style="padding:8px;border-bottom:1px solid rgba(74,158,255,0.1);color:#f4a460;font-weight:500;">' + m.name + '</td>';
            rows += '<td style="padding:8px;text-align:right;border-bottom:1px solid rgba(74,158,255,0.1);font-family:Consolas,monospace;color:#4ade80;">' + eff + '%</td>';
            rows += '<td style="padding:8px;text-align:right;border-bottom:1px solid rgba(74,158,255,0.1);font-family:Consolas,monospace;color:#cbd5e1;">' + vel + '</td>';
            rows += '<td style="padding:8px;text-align:right;border-bottom:1px solid rgba(74,158,255,0.1);font-family:Consolas,monospace;color:#a855f7;">' + life + '</td>';
            rows += '<td style="padding:8px;text-align:right;border-bottom:1px solid rgba(74,158,255,0.1);font-family:Consolas,monospace;color:#4a9eff;font-weight:600;">' + score + '</td>';
            rows += '</tr>';
        }
        tbody.innerHTML = rows;

        var trs = tbody.querySelectorAll('tr');
        for (var j = 0; j < trs.length; j++) {
            (function(idx) {
                trs[idx].addEventListener('click', function() {
                    self.selectMaterial(idx);
                });
            })(j);
        }
    };

    StringComparatorPanel.prototype.selectMaterial = function(idx) {
        this.selectedIndex = idx;
        this.renderTable();
    };

    StringComparatorPanel.prototype.renderChart = function() {
        var ctx = document.getElementById('sc-efficiency-chart');
        if (!ctx) return;
        ctx = ctx.getContext('2d');

        var labels = [];
        var effData = [];
        for (var i = 0; i < this.materials.length; i++) {
            labels.push(this.materials[i].name);
            effData.push((this.materials[i].energy_efficiency * 100).toFixed(1));
        }

        var colors = [
            'rgba(74,158,255,0.7)',
            'rgba(244,164,96,0.7)',
            'rgba(74,222,128,0.7)',
            'rgba(239,68,68,0.7)',
            'rgba(168,85,247,0.7)'
        ];

        if (this.chart) {
            this.chart.destroy();
        }

        this.chart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [{
                    label: '能量效率 (%)',
                    data: effData,
                    backgroundColor: colors.slice(0, labels.length),
                    borderColor: '#4a9eff',
                    borderWidth: 1,
                    borderRadius: 4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    title: {
                        display: true,
                        text: '弩弦材料能量效率对比',
                        color: '#4a9eff',
                        font: { size: 13 }
                    },
                    legend: { display: false }
                },
                scales: {
                    x: {
                        title: { display: true, text: '材料类型', color: '#94a3b8' },
                        grid: { color: 'rgba(74,158,255,0.08)' },
                        ticks: { color: '#64748b', font: { size: 11 } }
                    },
                    y: {
                        title: { display: true, text: '效率 (%)', color: '#94a3b8' },
                        grid: { color: 'rgba(74,158,255,0.08)' },
                        ticks: { color: '#64748b' },
                        min: 0,
                        max: 100
                    }
                }
            }
        });
    };

    return StringComparatorPanel;
})();
