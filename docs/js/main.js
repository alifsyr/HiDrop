    import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js";
    import { getDatabase, ref, onValue, set, get, child } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-database.js";
    import { plantData } from "./plants.js";

    let isEditingTargets = false;
    let editingTimer = null;
    let isUpdatingAutoDose = false;

    const firebaseConfig = {
      apiKey: "AIzaSyDWsj_Z0CHJsdG9wlDrv_9o1evizNLUv3M",
      authDomain: "hidrop0n1c.firebaseapp.com",
      databaseURL: "https://hidrop0n1c-default-rtdb.asia-southeast1.firebasedatabase.app",
      projectId: "hidrop0n1c",
      storageBucket: "hidrop0n1c.firebasestorage.app",
      messagingSenderId: "850371070464",
      appId: "1:850371070464:web:656085af5584e3d92ea6e5",
      measurementId: "G-PV1TNPV642"
    };

    let app, db;
    try {
        app = initializeApp(firebaseConfig);
        db = getDatabase(app);
    } catch(e) {
        console.error("Firebase not configured", e);
    }

    let latestStatus = null;
    let latestHistory = null;

    function formatNumber(value, digits) {
      if (typeof value !== "number" || Number.isNaN(value)) return "-";
      return value.toFixed(digits);
    }

    function formatDurationLabel(totalMs) {
      const totalSeconds = Math.max(0, Math.round((Number(totalMs) || 0) / 1000));
      const hours = Math.floor(totalSeconds / 3600);
      const minutes = Math.floor((totalSeconds % 3600) / 60);
      const seconds = totalSeconds % 60;
      if (hours > 0) return `${hours}h ${minutes}m`;
      if (minutes > 0) return `${minutes}m ${seconds}s`;
      return `${seconds}s`;
    }

    function createChartEmptyState(svgId, message) {
      const svg = document.getElementById(svgId);
      svg.innerHTML = `
        <rect x="0" y="0" width="640" height="240" rx="18" fill="rgba(13, 141, 119, 0.04)"></rect>
        <text x="320" y="120" text-anchor="middle" fill="#5f786d" font-size="16" font-family="Trebuchet MS, sans-serif">
          ${message}
        </text>
      `;
    }

    function buildChartPath(points) {
      if (!Array.isArray(points) || points.length === 0) return "";
      return points.map((point, index) => `${index === 0 ? "M" : "L"} ${point.x.toFixed(2)} ${point.y.toFixed(2)}`).join(" ");
    }

    function renderLineChart(options) {
      const {
        svgId,
        rangeId,
        latestId,
        values,
        digits,
        unit,
        color,
        targetMin,
        targetMax,
        hardMin,
        hardMax,
        sampleIntervalMs,
        lastLabel
      } = options;

      const svg = document.getElementById(svgId);
      const rangeLabel = document.getElementById(rangeId);
      const latestLabel = document.getElementById(latestId);
      if (!Array.isArray(values) || values.length < 2) {
        createChartEmptyState(svgId, "Need more samples to draw chart");
        rangeLabel.textContent = "Collecting history...";
        latestLabel.textContent = "-";
        return;
      }

      const numericValues = values
        .map((value) => Number(value))
        .filter((value) => Number.isFinite(value));

      if (numericValues.length < 2) {
        createChartEmptyState(svgId, "Invalid chart data");
        rangeLabel.textContent = "Collecting history...";
        latestLabel.textContent = "-";
        return;
      }

      const margin = { top: 16, right: 18, bottom: 34, left: 56 };
      const width = 640;
      const height = 240;
      const chartWidth = width - margin.left - margin.right;
      const chartHeight = height - margin.top - margin.bottom;

      let minValue = hardMin;
      let maxValue = hardMax;

      if (maxValue <= minValue) {
        maxValue = minValue + (digits === 2 ? 1 : 100);
      }

      const scaleX = (index) => margin.left + (index / (numericValues.length - 1)) * chartWidth;
      const scaleY = (value) => {
        const clampedValue = Math.max(minValue, Math.min(maxValue, value));
        return margin.top + ((maxValue - clampedValue) / (maxValue - minValue)) * chartHeight;
      };
      const chartPoints = numericValues.map((value, index) => ({ x: scaleX(index), y: scaleY(value), value }));

      const path = buildChartPath(chartPoints);
      const areaPath = `${path} L ${chartPoints[chartPoints.length - 1].x.toFixed(2)} ${(margin.top + chartHeight).toFixed(2)} L ${chartPoints[0].x.toFixed(2)} ${(margin.top + chartHeight).toFixed(2)} Z`;

      const bandTop = scaleY(targetMax);
      const bandBottom = scaleY(targetMin);
      const yTicks = 4;
      const grid = Array.from({ length: yTicks + 1 }, (_, index) => {
        const ratio = index / yTicks;
        const y = margin.top + ratio * chartHeight;
        const value = maxValue - ratio * (maxValue - minValue);
        return `
          <line x1="${margin.left}" y1="${y.toFixed(2)}" x2="${(margin.left + chartWidth).toFixed(2)}" y2="${y.toFixed(2)}" stroke="rgba(19,52,43,0.12)" stroke-dasharray="4 6"></line>
          <text x="${margin.left - 12}" y="${(y + 5).toFixed(2)}" text-anchor="end" fill="#5f786d" font-size="14" font-weight="500">${value.toFixed(digits)}</text>
        `;
      }).join("");

      const windowMs = Math.max(0, (numericValues.length - 1) * Math.max(0, Number(sampleIntervalMs) || 0));
      const firstLabel = windowMs > 0 ? `${formatDurationLabel(windowMs)} ago` : "Start";
      const lastValue = numericValues[numericValues.length - 1];

      svg.innerHTML = `
        <rect x="0" y="0" width="${width}" height="${height}" rx="18" fill="rgba(13, 141, 119, 0.02)"></rect>
        ${grid}
        <rect x="${margin.left}" y="${bandTop}" width="${chartWidth}" height="${Math.max(0, bandBottom - bandTop)}" fill="rgba(255,255,255,0.03)"></rect>
        <path d="${areaPath}" fill="url(#chart-gradient-${svgId})" opacity="0.6"></path>
        <path d="${path}" fill="none" stroke="${color}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"></path>
        ${chartPoints.map(p => `<circle cx="${p.x}" cy="${p.y}" r="3" fill="${color}"></circle>`).join("")}
        <text x="${width - margin.right}" y="${height - margin.bottom + 20}" fill="var(--muted)" font-size="13" font-weight="500" text-anchor="end">${lastLabel || "Now"}</text>
        <defs>
          <linearGradient id="chart-gradient-${svgId}" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="${color}" stop-opacity="0.3"></stop>
            <stop offset="100%" stop-color="${color}" stop-opacity="0"></stop>
          </linearGradient>
        </defs>
      `;
      
      rangeLabel.textContent = `Target ${formatNumber(targetMin, digits)} - ${formatNumber(targetMax, digits)} ${unit}`;
      latestLabel.textContent = `${formatNumber(lastValue, digits)} ${unit}`;
    }

    function renderReports(reports) {
      const container = document.getElementById("reportContainer");
      if (!Array.isArray(reports) || reports.length === 0) {
        container.innerHTML = '<p class="empty">Belum ada event dosing yang selesai.</p>';
        return;
      }

      const rows = reports.map((report) => `
        <tr>
          <td data-label="Timestamp" class="mono">${report.date} ${report.time}</td>
          <td data-label="PPM">${formatNumber(report.ppm_start, 0)} -> ${formatNumber(report.ppm_end, 0)}</td>
          <td data-label="pH">${formatNumber(report.ph_start, 2)} -> ${formatNumber(report.ph_end, 2)}</td>
          <td data-label="Nutrient A/B">${formatNumber(report.na_ml, 2)} / ${formatNumber(report.nb_ml, 2)}</td>
          <td data-label="pH Down/Up">${formatNumber(report.ph_down_ml, 2)} / ${formatNumber(report.ph_up_ml, 2)}</td>
          <td data-label="Result">${report.manual_dilution_required ? "Manual dilution" : "Completed"}</td>
        </tr>
      `).join("");

      container.innerHTML = `
        <div class="table-responsive">
          <table>
            <thead>
              <tr>
                <th>Timestamp</th>
                <th>PPM</th>
                <th>pH</th>
                <th>Nutrient A/B (ml)</th>
                <th>pH Down/Up (ml)</th>
                <th>Result</th>
              </tr>
            </thead>
            <tbody>${rows}</tbody>
          </table>
        </div>
      `;
    }

    function renderHistory() {
      const sampleIntervalMs = Number(latestHistory?.sample_interval_ms || 0);
      const lastLabel = latestStatus?.device?.time_valid ? (latestStatus?.device?.time || "Now") : "Now";

      renderLineChart({
        svgId: "phChart",
        rangeId: "phChartRange",
        latestId: "phChartLatest",
        values: Array.isArray(latestHistory?.ph_x100) ? latestHistory.ph_x100.map((value) => Number(value) / 100) : [],
        digits: 2,
        unit: "pH",
        color: "#0d8d77",
        targetMin: Number(latestStatus?.targets?.ph_min || 0),
        targetMax: Number(latestStatus?.targets?.ph_max || 14),
        hardMin: 0,
        hardMax: 14,
        sampleIntervalMs,
        lastLabel
      });
      renderLineChart({
        svgId: "ppmChart",
        rangeId: "ppmChartRange",
        latestId: "ppmChartLatest",
        values: Array.isArray(latestHistory?.ppm) ? latestHistory.ppm.map((value) => Number(value)) : [],
        digits: 0,
        unit: "ppm",
        color: "#2b6cb0",
        targetMin: Number(latestStatus?.targets?.ppm_min || 0),
        targetMax: Number(latestStatus?.targets?.ppm_max || 1500),
        hardMin: 0,
        hardMax: 5000,
        sampleIntervalMs,
        lastLabel
      });
    }

    function applyReports(reports) {
      renderReports(Array.isArray(reports) ? reports : []);
    }

    function applyStatus(data) {
      latestStatus = data;
      
      const nowMs = Date.now();
      
      if (data.device && data.device.firmware_version) {
          backgroundOtaCheck(data.device.firmware_version);
      }
      let isOnline = false;
      
      // Track the last time the payload changed (timestamp or uptime)
      if (data.timestamp) {
          if (window._lastDeviceTimestamp !== data.timestamp) {
              window._lastDeviceTimestamp = data.timestamp;
              window._lastReceiveMs = nowMs;
          }
      } else if (data.device && data.device.uptime_seconds) {
          if (window._lastDeviceUptime !== data.device.uptime_seconds) {
              window._lastDeviceUptime = data.device.uptime_seconds;
              window._lastReceiveMs = nowMs;
          }
      }
      
      // If we received an update in the last 12 seconds (ESP32 sends every 5s), it is online
      const lastReceiveMs = window._lastReceiveMs || nowMs;
      isOnline = (nowMs - lastReceiveMs) < 12000;
      
      // Determine what time to show on the UI
      let displayTimeMs = lastReceiveMs;
      if (data.device && data.device.time_valid && isOnline) {
          const espTime = new Date(`${data.device.date.replace(/-/g, '/')} ${data.device.time}`).getTime();
          if (!isNaN(espTime)) {
              displayTimeMs = espTime;
          }
      }
      document.getElementById("heroStatus").textContent = isOnline
        ? "Device online and serving live telemetry"
        : "Device offline (showing last known state)";
      document.getElementById("wifiDot").classList.toggle("ok", isOnline);
      const dateObj = new Date(displayTimeMs);
      const dateStr = dateObj.toLocaleDateString("id-ID", { timeZone: "Asia/Jakarta", day: "2-digit", month: "short", year: "numeric" });
      const timeStr = dateObj.toLocaleTimeString("id-ID", { timeZone: "Asia/Jakarta", hour: "2-digit", minute: "2-digit", second: "2-digit" });
      document.getElementById("refreshedAt").innerHTML = `<div>${dateStr}</div><div style="font-size: 0.9em; color: var(--muted); margin-top: 2px;">${timeStr}</div>`;

      document.getElementById("ipAddress").textContent = `IP: ${data?.device?.ip_address || "-"}`;

      document.getElementById("phValue").innerHTML = `${formatNumber(data?.sensor?.ph, 2)}<small>pH</small>`;
      document.getElementById("ppmValue").innerHTML = `${formatNumber(data?.sensor?.ppm, 0)}<small>ppm</small>`;
      document.getElementById("tempValue").innerHTML = `${formatNumber(data?.sensor?.temperature_c, 2)}<small>&deg;C</small>`;
      document.getElementById("dosingState").textContent = data?.dosing?.state || "-";

      document.getElementById("phMeta").textContent = `Target: ${formatNumber(data?.targets?.ph_min, 2)} - ${formatNumber(data?.targets?.ph_max, 2)}`;
      document.getElementById("ppmMeta").textContent = `Target: ${formatNumber(data?.targets?.ppm_min, 0)} - ${formatNumber(data?.targets?.ppm_max, 0)}`;
      document.getElementById("tempMeta").textContent = `Voltage pH probe: ${formatNumber(data?.sensor?.ph_voltage, 3)} V`;
      document.getElementById("dosingMeta").textContent = `Mode: ${data?.dosing?.display_mode || "-"} | Busy: ${data?.dosing?.busy ? "YES" : "NO"}`;
      
      const autoDoseToggle = document.getElementById('autoDoseToggle');
      const autoDoseStatusText = document.getElementById('autoDoseStatusText');
      if (data?.dosing && data.dosing.hasOwnProperty('auto_dosing_enabled')) {
        autoDoseToggle.disabled = false;
        if (!isUpdatingAutoDose) {
          autoDoseToggle.checked = data.dosing.auto_dosing_enabled;
          autoDoseStatusText.textContent = data.dosing.auto_dosing_enabled ? "ON (Automated)" : "OFF (Manual)";
          autoDoseStatusText.style.color = data.dosing.auto_dosing_enabled ? "var(--ok)" : "var(--warn)";
        }
      }

      if(data?.device?.ota_available) {
        document.getElementById('otaBadge').style.display = 'block';
      } else {
        document.getElementById('otaBadge').style.display = 'none';
      }
      
      if (!isEditingTargets && data?.targets) {
        document.getElementById('phMinSlider').value = data.targets.ph_min;
        document.getElementById('phMaxSlider').value = data.targets.ph_max;
        document.getElementById('ppmMinSlider').value = data.targets.ppm_min;
        document.getElementById('ppmMaxSlider').value = data.targets.ppm_max;
        updateDualSlider(null, 'ph');
        updateDualSlider(null, 'ppm');
      }
      
      renderHistory();
    }

    function updateDualSlider(triggerEl, prefix) {
      const minSlider = document.getElementById(prefix + 'MinSlider');
      const maxSlider = document.getElementById(prefix + 'MaxSlider');
      const minVal = document.getElementById(prefix + 'MinVal');
      const maxVal = document.getElementById(prefix + 'MaxVal');
      const progress = document.getElementById(prefix + 'Progress');

      let min = parseFloat(minSlider.value);
      let max = parseFloat(maxSlider.value);
      
      // Use larger gap for PPM
      const minGap = prefix === 'ph' ? 0.2 : 150;

      if (min > max - minGap) {
        if (triggerEl === minSlider) {
          minSlider.value = (max - minGap).toString();
          min = max - minGap;
        } else {
          maxSlider.value = (min + minGap).toString();
          max = min + minGap;
        }
      }

      minVal.textContent = prefix === 'ph' ? min.toFixed(2) : min.toString();
      maxVal.textContent = prefix === 'ph' ? max.toFixed(2) : max.toString();

      const total = minSlider.max - minSlider.min;
      const leftPercent = ((min - minSlider.min) / total) * 100;
      const widthPercent = ((max - min) / total) * 100;

      progress.style.left = leftPercent + '%';
      progress.style.width = widthPercent + '%';
      
      // Haptic feedback
      if (triggerEl && 'vibrate' in navigator) {
        navigator.vibrate(10);
      }
    }

    function applyHistory(data) {
      latestHistory = data;
      renderHistory();
    }

    const targetSliders = ['phMinSlider', 'phMaxSlider', 'ppmMinSlider', 'ppmMaxSlider'];
    targetSliders.forEach(id => {
      const el = document.getElementById(id);
      if (el) {
        el.addEventListener('mousedown', () => {
          isEditingTargets = true;
          clearTimeout(editingTimer);
        });
        el.addEventListener('touchstart', () => {
          isEditingTargets = true;
          clearTimeout(editingTimer);
        }, {passive: true});
        el.addEventListener('mouseup', () => {
          editingTimer = setTimeout(() => { isEditingTargets = false; }, 3000);
        });
        el.addEventListener('touchend', () => {
          editingTimer = setTimeout(() => { isEditingTargets = false; }, 3000);
        });
      }
    });

    
    async function saveTarget(prefix) {
      if(!db) return;
      const status = document.getElementById(prefix + 'SaveStatus');
      
      const targets = {
        ph_min: Number(document.getElementById('phMinSlider').value),
        ph_max: Number(document.getElementById('phMaxSlider').value),
        ppm_min: Number(document.getElementById('ppmMinSlider').value),
        ppm_max: Number(document.getElementById('ppmMaxSlider').value),
        timestamp: Date.now(),
        pending_update: true
      };

      try {
        await set(ref(db, 'hydroponic/commands/targets'), targets);
        status.textContent = 'Saved to Cloud!';
        status.style.color = 'var(--ok)';
        status.style.display = 'block';
        setTimeout(() => { status.style.display = 'none'; }, 2000);
      } catch (error) {
        status.textContent = 'Error saving';
        status.style.color = 'var(--danger)';
        status.style.display = 'block';
        setTimeout(() => { status.style.display = 'none'; }, 2000);
      }
    }


    let latestGithubRelease = null;
    let otaCheckDone = false;

    async function backgroundOtaCheck(currentVer) {
      if (otaCheckDone || !currentVer) return;
      otaCheckDone = true;
      try {
        const res = await fetch('https://api.github.com/repos/alifsyr/HiDrop/releases/latest', {
          headers: { 'Accept': 'application/vnd.github.v3+json' }
        });
        if (res.ok) {
          latestGithubRelease = await res.json();
          const latestTag = latestGithubRelease.tag_name || '-';
          const latestClean = latestTag.replace(/^v/, '');
          const currentClean = currentVer.replace(/^v/, '');
          if (latestClean !== currentClean && latestClean !== '-') {
             document.getElementById('otaBadge').style.display = 'block';
          }
        }
      } catch (err) {
        console.error("OTA Check Failed", err);
        otaCheckDone = false;
      }
    }

    function openOtaModal() {
      document.getElementById('otaModal').style.display = 'flex';
      
      const currentVer = latestStatus?.device?.firmware_version || '-';
      document.getElementById('otaCurrentVer').textContent = currentVer;
      
      const latestTag = latestGithubRelease?.tag_name || '-';
      document.getElementById('otaLatestVer').textContent = latestTag;
      
      const status = document.getElementById('otaStatusText');
      status.textContent = '';
      
      const latestClean = latestTag.replace(/^v/, '');
      const currentClean = currentVer.replace(/^v/, '');
      
      if (!currentClean || currentClean === '-') {
        status.textContent = 'Cannot determine current version.';
        status.style.color = 'var(--warn)';
        document.getElementById('otaStartBtn').style.display = 'none';
      } else if (latestClean === '-' || latestClean === currentClean) {
        status.textContent = '✓ Already up to date.';
        status.style.color = 'var(--ok)';
        document.getElementById('otaStartBtn').style.display = 'none';
      } else {
        status.textContent = `Update available! (${currentClean} → ${latestClean})`;
        status.style.color = 'var(--warn)';
        document.getElementById('otaStartBtn').style.display = 'block';
      }
    }

    function closeOtaModal() {
      document.getElementById('otaModal').style.display = 'none';
    }
    
    async function startOtaUpdate() {
      const btn = document.getElementById('otaStartBtn');
      const status = document.getElementById('otaStatusText');
      const progressWrap = document.getElementById('otaProgressWrap');
      const progressBar = document.getElementById('otaProgressBar');
      const progressPct = document.getElementById('otaProgressPct');

      if (!db) {
        status.textContent = 'Firebase not connected.';
        status.style.color = 'var(--danger)';
        return;
      }
      btn.disabled = true;
      btn.textContent = 'Sending command...';
      status.textContent = 'Fetching firmware URL from GitHub...';
      status.style.color = 'var(--muted)';

      try {
        // Step 1: Get download URL from GitHub
        const res = await fetch('https://api.github.com/repos/alifsyr/HiDrop/releases/latest', {
          headers: { 'Accept': 'application/vnd.github.v3+json' }
        });
        if (!res.ok) throw new Error(`GitHub API error: ${res.status}`);
        const release = await res.json();

        const latestTag = release.tag_name || '';
        let firmwareUrl = '';
        for (const asset of (release.assets || [])) {
          if (asset.name === 'firmware.bin') {
            firmwareUrl = asset.browser_download_url;
            break;
          }
        }
        if (!firmwareUrl) throw new Error('firmware.bin not found in latest release assets.');

        // Step 2: Write OTA command to Firebase
        await set(ref(db, 'hydroponic/commands/ota'), {
          trigger: true,
          url: firmwareUrl,
          version: latestTag,
          timestamp: Date.now()
        });

        btn.textContent = 'Waiting for device...';
        status.textContent = 'Command sent! Waiting for ESP32 to respond...';
        status.style.color = 'var(--warn)';
        progressWrap.style.display = 'block';
        progressBar.style.width = '0%';
        progressPct.textContent = '0%';

        // Step 3: Listen to ESP32 progress via Firebase
        let successHandled = false;
        const unsubOta = onValue(ref(db, 'hydroponic/status/device'), (snap) => {
          const d = snap.val();
          if (!d) return;
          const otaStatus = d.ota_status || '';
          const pct = d.ota_progress || 0;

          if (successHandled) return;

          if (otaStatus === 'downloading' && pct < 100) {
            progressBar.style.width = pct + '%';
            progressPct.textContent = pct + '%';
            status.textContent = `Downloading firmware... ${pct}%`;
            status.style.color = 'var(--warn)';
          } else if (otaStatus === 'failed') {
            status.textContent = '✗ Update gagal. Cek Serial Monitor.';
            status.style.color = 'var(--danger)';
            btn.disabled = false;
            btn.textContent = 'Install Update & Restart';
            unsubOta();
          } else if (pct === 100 || (otaStatus === '' && pct === 0 && progressBar.style.width !== '0%')) {
            // pct===100: download complete before reboot
            // otaStatus==='' && pct===0: device already rebooted and cleared the field (success)
            successHandled = true;
            progressBar.style.width = '100%';
            progressPct.textContent = '100%';
            btn.disabled = true;
            btn.textContent = 'Update Berhasil ✓';
            status.textContent = '✓ Update berhasil! Device sedang restart... (Auto reload dalam 15 detik)';
            status.style.color = 'var(--ok)';
            unsubOta();
            setTimeout(() => {
              window.location.reload();
            }, 15000);
          }
        });

      } catch (err) {
        status.textContent = `Error: ${err.message}`;
        status.style.color = 'var(--danger)';
        btn.disabled = false;
        btn.textContent = 'Install Update & Restart';
      }
    }

    // Expose functions to global scope so onclick HTML attributes work inside ES module
    window.openOtaModal = openOtaModal;
    window.closeOtaModal = closeOtaModal;
    window.updateDualSlider = updateDualSlider;
    window.saveTarget = saveTarget;
    window.startOtaUpdate = startOtaUpdate;
    function requireConfirmation(message, onConfirm, onCancel) {
      const modal = document.getElementById('confirmModal');
      const msgEl = document.getElementById('confirmMessage');
      const yesBtn = document.getElementById('confirmYesBtn');
      const noBtn = document.getElementById('confirmNoBtn');
      
      msgEl.textContent = message;
      modal.style.display = 'flex';
      
      // Clear previous event listeners by replacing elements
      const newYes = yesBtn.cloneNode(true);
      const newNo = noBtn.cloneNode(true);
      yesBtn.parentNode.replaceChild(newYes, yesBtn);
      noBtn.parentNode.replaceChild(newNo, noBtn);
      
      newYes.onclick = () => {
        modal.style.display = 'none';
        if (onConfirm) onConfirm();
      };
      
      newNo.onclick = () => {
        modal.style.display = 'none';
        if (onCancel) onCancel();
      };
    }

    window.toggleAutoDose = async function(checkbox) {
      if (!db) return;
      
      requireConfirmation(
        checkbox.checked ? "Turn ON Auto Dosing?" : "Turn OFF Auto Dosing?",
        async () => {
          // ON CONFIRM
          const statusText = document.getElementById('autoDoseStatusText');
          isUpdatingAutoDose = true;
          statusText.textContent = "Updating...";
          statusText.style.color = "var(--muted)";
          checkbox.disabled = true;
          
          try {
            await set(ref(db, 'hydroponic/commands/dosing_enabled'), checkbox.checked);
            statusText.textContent = checkbox.checked ? "ON (Automated)" : "OFF (Manual)";
            statusText.style.color = checkbox.checked ? "var(--ok)" : "var(--warn)";
            checkbox.disabled = false;
          } catch (error) {
            statusText.textContent = 'Error updating';
            statusText.style.color = 'var(--danger)';
            checkbox.checked = !checkbox.checked; // Revert
            checkbox.disabled = false;
          } finally {
            isUpdatingAutoDose = false;
          }
        },
        () => {
          // ON CANCEL
          checkbox.checked = !checkbox.checked; // Revert visually
        }
      );
    };
    window.triggerManualPump = async function(command, btnElement) {
      if (!db) return;
      
      const pumpName = btnElement.textContent;
      requireConfirmation(
        `Are you sure you want to dose ${pumpName} manually for 1 step?`,
        async () => {
          const originalText = btnElement.textContent;
          btnElement.textContent = "Sending...";
          btnElement.disabled = true;
          
          try {
            await set(ref(db, 'hydroponic/commands/manual_trigger/command'), command);
            await set(ref(db, 'hydroponic/commands/manual_trigger/pending'), true);
            
            btnElement.textContent = "Sent!";
            setTimeout(() => {
              btnElement.textContent = originalText;
              btnElement.disabled = false;
            }, 2000);
          } catch (error) {
            console.error("Manual trigger error:", error);
            btnElement.textContent = "Failed";
            setTimeout(() => {
              btnElement.textContent = originalText;
              btnElement.disabled = false;
            }, 2000);
          }
        }
      );
    };

    // Initialize sliders on page load
    updateDualSlider(null, 'ph');
    updateDualSlider(null, 'ppm');
    
    // Set dynamic copyright year
    document.getElementById('copyrightYear').textContent = new Date().getFullYear();

    // Render Plants Table
    function renderPlantsTable() {
      const tbody = document.getElementById("plantsTableBody");
      if (!tbody) return;
      
      const searchInput = document.getElementById("plantSearchInput")?.value.toLowerCase() || "";
      const catFilter = document.getElementById("categoryFilter")?.value || "All";
      const sortVal = document.getElementById("plantSort")?.value || "name_asc";
      
      let filteredData = plantData.filter(plant => {
        const matchesSearch = plant.name.toLowerCase().includes(searchInput);
        const matchesCat = catFilter === "All" || plant.category === catFilter;
        return matchesSearch && matchesCat;
      });
      
      filteredData.sort((a, b) => {
        if (sortVal === "name_asc") return a.name.localeCompare(b.name);
        if (sortVal === "name_desc") return b.name.localeCompare(a.name);
        if (sortVal === "cat_asc") {
          const catCmp = a.category.localeCompare(b.category);
          return catCmp !== 0 ? catCmp : a.name.localeCompare(b.name);
        }
        return 0;
      });
      
      let html = "";
      if (filteredData.length === 0) {
        html = `<tr><td colspan="5" style="text-align: center; color: var(--muted); padding: 24px;">No plants found.</td></tr>`;
      } else {
        filteredData.forEach(plant => {
          html += `
            <tr>
              <td data-label="Plant Type">${plant.name}</td>
              <td data-label="Category">${plant.category}</td>
              <td data-label="Ideal pH" class="mono">${plant.phMin} - ${plant.phMax}</td>
              <td data-label="Target PPM" class="mono">${plant.ppmMin} - ${plant.ppmMax}</td>
              <td data-label="Action" style="text-align: left;">
                <button class="btn" style="padding: 6px 12px; font-size: 0.8rem; background: var(--accent); box-shadow: none;" onclick="applyPlantTarget(${plant.id})">Apply Target</button>
              </td>
            </tr>
          `;
        });
      }
      tbody.innerHTML = html;
    }
    
    document.getElementById("plantSearchInput")?.addEventListener("input", renderPlantsTable);
    document.getElementById("categoryFilter")?.addEventListener("change", renderPlantsTable);
    document.getElementById("plantSort")?.addEventListener("change", renderPlantsTable);
    
    window.applyPlantTarget = (id) => {
      const plant = plantData.find(p => p.id === id);
      if (!plant) return;
      
      requireConfirmation(
        `Apply target for ${plant.name} (pH: ${plant.phMin}-${plant.phMax}, PPM: ${plant.ppmMin}-${plant.ppmMax})?`,
        () => {
          document.getElementById('phMinSlider').value = plant.phMin;
          document.getElementById('phMaxSlider').value = plant.phMax;
          document.getElementById('ppmMinSlider').value = plant.ppmMin;
          document.getElementById('ppmMaxSlider').value = plant.ppmMax;
          
          updateDualSlider(document.getElementById('phMinSlider'), 'ph');
          updateDualSlider(document.getElementById('ppmMinSlider'), 'ppm');
          
          window.saveTarget('ph');
          window.saveTarget('ppm');
        }
      );
    };

    renderPlantsTable();

    // Connect to Firebase and listen to changes
    if(db) {
      const statusRef = ref(db, 'hydroponic/status');
      onValue(statusRef, (snapshot) => {
        if(snapshot.exists()) {
          try {
            applyStatus(snapshot.val());
          } catch(e) {
            document.getElementById("heroStatus").textContent = "JS Error: " + e.message;
            console.error("applyStatus error:", e);
          }
        }
      });

      const historyRef = ref(db, 'hydroponic/history');
      onValue(historyRef, (snapshot) => {
        if(snapshot.exists()) applyHistory(snapshot.val());
      });
      
      const reportsRef = ref(db, 'hydroponic/reports');
      onValue(reportsRef, (snapshot) => {
        if(snapshot.exists()) {
            const reportsObj = snapshot.val();
            // Convert object to array and sort by latest
            const reportsArr = Object.values(reportsObj).reverse().slice(0, 5);
            applyReports(reportsArr);
        }
      });
    }
    
    setInterval(() => {
        if(latestStatus) applyStatus(latestStatus);
    }, 1000);

