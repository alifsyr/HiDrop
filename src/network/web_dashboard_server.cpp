#include "network/web_dashboard_server.h"

#include <WiFi.h>

#include "config/app_config.h"

namespace {
const char *kDashboardPage = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>__DASHBOARD_TITLE__</title>
  <style>
    :root {
      --bg: #edf7f4;
      --bg-strong: radial-gradient(circle at top left, #ffffff 0%, #edf7f4 38%, #d8ece5 100%);
      --panel: rgba(255, 255, 255, 0.86);
      --panel-border: rgba(21, 90, 72, 0.14);
      --text: #13342b;
      --muted: #5f786d;
      --accent: #0d8d77;
      --accent-soft: rgba(13, 141, 119, 0.12);
      --ok: #15803d;
      --warn: #b7791f;
      --danger: #b42318;
      --shadow: 0 24px 60px rgba(28, 65, 55, 0.12);
      --radius: 24px;
      --mono: "SFMono-Regular", "Consolas", "Liberation Mono", monospace;
      --sans: "Trebuchet MS", "Avenir Next", "Segoe UI", sans-serif;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      font-family: var(--sans);
      color: var(--text);
      background: var(--bg-strong);
    }

    .shell {
      width: min(1120px, calc(100vw - 24px));
      margin: 0 auto;
      padding: 28px 0 40px;
    }

    .hero {
      display: grid;
      gap: 18px;
      padding: 28px;
      border-radius: 32px;
      background:
        linear-gradient(140deg, rgba(255,255,255,0.95), rgba(220,242,235,0.9)),
        linear-gradient(135deg, rgba(13,141,119,0.2), transparent 55%);
      box-shadow: var(--shadow);
      border: 1px solid rgba(19, 52, 43, 0.08);
    }

    .hero-head {
      display: flex;
      align-items: start;
      justify-content: space-between;
      gap: 18px;
      flex-wrap: wrap;
    }

    .eyebrow {
      display: inline-flex;
      align-items: center;
      gap: 10px;
      padding: 8px 14px;
      border-radius: 999px;
      background: rgba(255,255,255,0.8);
      color: var(--muted);
      font-size: 0.9rem;
      letter-spacing: 0.03em;
    }

    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--warn);
      box-shadow: 0 0 0 8px rgba(183, 121, 31, 0.1);
      transition: background 160ms ease, box-shadow 160ms ease;
    }

    .dot.ok {
      background: var(--ok);
      box-shadow: 0 0 0 8px rgba(21, 128, 61, 0.12);
    }

    h1 {
      margin: 10px 0 6px;
      font-size: clamp(2rem, 5vw, 3.5rem);
      line-height: 0.95;
      letter-spacing: -0.04em;
    }

    .subhead {
      margin: 0;
      max-width: 60ch;
      color: var(--muted);
      font-size: 1rem;
      line-height: 1.6;
    }

    .hero-stats {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 14px;
    }

    .stat {
      padding: 18px;
      border-radius: 20px;
      background: rgba(255,255,255,0.74);
      border: 1px solid rgba(19, 52, 43, 0.07);
      backdrop-filter: blur(8px);
    }

    .label {
      margin: 0 0 10px;
      color: var(--muted);
      font-size: 0.84rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .value {
      margin: 0;
      font-size: clamp(1.65rem, 4vw, 2.5rem);
      line-height: 1;
      font-weight: 700;
    }

    .value small {
      font-size: 0.55em;
      color: var(--muted);
      font-weight: 600;
      margin-left: 4px;
    }

    .meta {
      margin-top: 10px;
      color: var(--muted);
      font-size: 0.92rem;
    }

    .panel {
      padding: 24px;
      border-radius: var(--radius);
      background: var(--panel);
      border: 1px solid var(--panel-border);
      box-shadow: var(--shadow);
      backdrop-filter: blur(8px);
    }

    .panel h2 {
      margin: 0 0 16px;
      font-size: 1.15rem;
      letter-spacing: -0.03em;
    }

    .section-head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 16px;
      flex-wrap: wrap;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 0.95rem;
    }

    th, td {
      text-align: left;
      padding: 12px 10px;
      border-bottom: 1px solid rgba(19, 52, 43, 0.08);
    }

    th {
      color: var(--muted);
      font-size: 0.82rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .mono {
      font-family: var(--mono);
      font-size: 0.92rem;
    }

    .empty {
      color: var(--muted);
      font-style: italic;
    }

    .chart-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 18px;
      margin-top: 18px;
    }

    .chart-card {
      display: grid;
      gap: 14px;
    }

    .chart-frame {
      min-height: 260px;
      padding: 16px;
      border-radius: 20px;
      background:
        linear-gradient(180deg, rgba(255, 255, 255, 0.94), rgba(237, 247, 244, 0.82));
      border: 1px solid rgba(19, 52, 43, 0.08);
    }

    .chart-svg {
      width: 100%;
      height: 240px;
      display: block;
    }

    .chart-meta {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      flex-wrap: wrap;
      color: var(--muted);
      font-size: 0.9rem;
    }

    .chart-stat {
      font-weight: 700;
      color: var(--text);
    }

    @media (max-width: 860px) {
      .chart-grid {
        grid-template-columns: 1fr;
      }

      .shell {
        width: min(100vw - 18px, 1120px);
        padding-top: 18px;
      }

      .hero,
      .panel {
        padding: 20px;
      }
    }
  </style>
</head>
<body>
  <main class="shell">
    <section class="hero">
      <div class="hero-head">
        <div>
          <div class="eyebrow">
            <span id="wifiDot" class="dot"></span>
            <span id="heroStatus">Connecting to device...</span>
          </div>
          <h1>__DASHBOARD_TITLE__</h1>
          <p class="subhead">
            Dashboard live untuk pH, PPM, suhu air, status auto dosing, dan histori singkat aktivitas terakhir dari ESP32.
          </p>
        </div>
        <div class="stat">
          <p class="label">Last Refresh</p>
          <p id="refreshedAt" class="value" style="font-size:1.4rem">Waiting...</p>
          <div class="meta mono" id="ipAddress">IP: -</div>
        </div>
      </div>

      <div class="hero-stats">
        <article class="stat">
          <p class="label">pH Air</p>
          <p id="phValue" class="value">-</p>
          <div class="meta" id="phMeta">Target: -</div>
        </article>
        <article class="stat">
          <p class="label">PPM</p>
          <p id="ppmValue" class="value">-</p>
          <div class="meta" id="ppmMeta">Target: -</div>
        </article>
        <article class="stat">
          <p class="label">Temperature</p>
          <p id="tempValue" class="value">-</p>
          <div class="meta" id="tempMeta">Water temperature</div>
        </article>
        <article class="stat">
          <p class="label">Auto Dosing</p>
          <p id="dosingState" class="value" style="font-size:1.25rem; line-height:1.2">-</p>
          <div class="meta" id="dosingMeta">Mode: -</div>
        </article>
      </div>
    </section>

    <section class="chart-grid">
      <article class="panel chart-card">
        <div class="section-head">
          <h2>pH Trend</h2>
          <span class="meta">Recent sensor history</span>
        </div>
        <div class="chart-frame">
          <svg id="phChart" class="chart-svg" viewBox="0 0 640 240" role="img" aria-label="pH history chart"></svg>
        </div>
        <div class="chart-meta">
          <span id="phChartRange">Waiting for data...</span>
          <span id="phChartLatest" class="chart-stat">-</span>
        </div>
      </article>

      <article class="panel chart-card">
        <div class="section-head">
          <h2>PPM Trend</h2>
          <span class="meta">Recent sensor history</span>
        </div>
        <div class="chart-frame">
          <svg id="ppmChart" class="chart-svg" viewBox="0 0 640 240" role="img" aria-label="PPM history chart"></svg>
        </div>
        <div class="chart-meta">
          <span id="ppmChartRange">Waiting for data...</span>
          <span id="ppmChartLatest" class="chart-stat">-</span>
        </div>
      </article>
    </section>

    <section class="panel" style="margin-top:18px">
      <h2>Recent Dosing Reports</h2>
      <div id="reportContainer">
        <p class="empty">Belum ada event dosing yang selesai.</p>
      </div>
    </section>
  </main>

  <script>
    const refreshIntervalMs = __REFRESH_INTERVAL_MS__;
    const historyRefreshIntervalMs = __HISTORY_REFRESH_INTERVAL_MS__;
    const reportsRefreshIntervalMs = __REPORTS_REFRESH_INTERVAL_MS__;
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

      const margin = { top: 16, right: 18, bottom: 34, left: 48 };
      const width = 640;
      const height = 240;
      const chartWidth = width - margin.left - margin.right;
      const chartHeight = height - margin.top - margin.bottom;

      let minValue = Math.min(...numericValues, Number(targetMin));
      let maxValue = Math.max(...numericValues, Number(targetMax));
      if (!Number.isFinite(minValue)) minValue = hardMin;
      if (!Number.isFinite(maxValue)) maxValue = hardMax;

      const spread = Math.max(maxValue - minValue, digits === 2 ? 0.4 : 50);
      minValue -= spread * 0.18;
      maxValue += spread * 0.18;
      minValue = Math.max(hardMin, minValue);
      maxValue = Math.min(hardMax, maxValue);
      if (maxValue <= minValue) {
        maxValue = minValue + (digits === 2 ? 1 : 100);
      }

      const scaleX = (index) => margin.left + (index / (numericValues.length - 1)) * chartWidth;
      const scaleY = (value) => margin.top + ((maxValue - value) / (maxValue - minValue)) * chartHeight;
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
          <text x="${margin.left - 10}" y="${(y + 4).toFixed(2)}" text-anchor="end" fill="#5f786d" font-size="11">${value.toFixed(digits)}</text>
        `;
      }).join("");

      const windowMs = Math.max(0, (numericValues.length - 1) * Math.max(0, Number(sampleIntervalMs) || 0));
      const firstLabel = windowMs > 0 ? `-${formatDurationLabel(windowMs)}` : "Start";
      const lastValue = numericValues[numericValues.length - 1];

      svg.innerHTML = `
        <rect x="0" y="0" width="${width}" height="${height}" rx="18" fill="rgba(13, 141, 119, 0.02)"></rect>
        ${grid}
        <rect
          x="${margin.left}"
          y="${Math.min(bandTop, bandBottom).toFixed(2)}"
          width="${chartWidth}"
          height="${Math.abs(bandBottom - bandTop).toFixed(2)}"
          fill="rgba(13, 141, 119, 0.10)">
        </rect>
        <path d="${areaPath}" fill="${color}" opacity="0.12"></path>
        <path d="${path}" fill="none" stroke="${color}" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"></path>
        ${chartPoints.map((point, index) => `
          <circle
            cx="${point.x.toFixed(2)}"
            cy="${point.y.toFixed(2)}"
            r="${index === chartPoints.length - 1 ? 5.5 : 3}"
            fill="${index === chartPoints.length - 1 ? color : "#ffffff"}"
            stroke="${color}"
            stroke-width="2">
          </circle>
        `).join("")}
        <text x="${margin.left}" y="${height - 10}" fill="#5f786d" font-size="11">${firstLabel}</text>
        <text x="${(margin.left + chartWidth).toFixed(2)}" y="${height - 10}" text-anchor="end" fill="#5f786d" font-size="11">${lastLabel || "Now"}</text>
      `;

      rangeLabel.textContent = `Window ${formatDurationLabel(windowMs)} | Target ${formatNumber(targetMin, digits)} - ${formatNumber(targetMax, digits)} ${unit}`;
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
          <td class="mono">${report.date} ${report.time}</td>
          <td>${formatNumber(report.ppm_start, 0)} -> ${formatNumber(report.ppm_end, 0)}</td>
          <td>${formatNumber(report.ph_start, 2)} -> ${formatNumber(report.ph_end, 2)}</td>
          <td>${formatNumber(report.na_ml, 2)} / ${formatNumber(report.nb_ml, 2)}</td>
          <td>${formatNumber(report.ph_down_ml, 2)} / ${formatNumber(report.ph_up_ml, 2)}</td>
          <td>${report.manual_dilution_required ? "Manual dilution" : "Completed"}</td>
        </tr>
      `).join("");

      container.innerHTML = `
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
      const wifiOk = Boolean(data?.device?.wifi_connected);
      document.getElementById("heroStatus").textContent = wifiOk
        ? "Device online and serving live telemetry"
        : "Wi-Fi belum tersambung, menunggu koneksi";
      document.getElementById("wifiDot").classList.toggle("ok", wifiOk);
      document.getElementById("refreshedAt").textContent = new Date().toLocaleTimeString();
      document.getElementById("ipAddress").textContent = `IP: ${data?.device?.ip_address || "-"}`;

      document.getElementById("phValue").innerHTML = `${formatNumber(data?.sensor?.ph, 2)}<small>pH</small>`;
      document.getElementById("ppmValue").innerHTML = `${formatNumber(data?.sensor?.ppm, 0)}<small>ppm</small>`;
      document.getElementById("tempValue").innerHTML = `${formatNumber(data?.sensor?.temperature_c, 2)}<small>&deg;C</small>`;
      document.getElementById("dosingState").textContent = data?.dosing?.state || "-";

      document.getElementById("phMeta").textContent = `Target: ${formatNumber(data?.targets?.ph_min, 2)} - ${formatNumber(data?.targets?.ph_max, 2)}`;
      document.getElementById("ppmMeta").textContent = `Target: ${formatNumber(data?.targets?.ppm_min, 0)} - ${formatNumber(data?.targets?.ppm_max, 0)}`;
      document.getElementById("tempMeta").textContent = `Voltage pH probe: ${formatNumber(data?.sensor?.ph_voltage, 3)} V`;
      document.getElementById("dosingMeta").textContent = `Mode: ${data?.dosing?.display_mode || "-"} | Busy: ${data?.dosing?.busy ? "YES" : "NO"}`;
      renderHistory();
    }

    function applyHistory(data) {
      latestHistory = data;
      renderHistory();
    }

    async function refreshStatus() {
      try {
        const response = await fetch("/api/status", { cache: "no-store" });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        applyStatus(await response.json());
      } catch (error) {
        document.getElementById("heroStatus").textContent = `Failed to load live data: ${error.message}`;
        document.getElementById("wifiDot").classList.remove("ok");
      }
    }

    async function refreshHistory() {
      try {
        const response = await fetch("/api/history", { cache: "no-store" });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        applyHistory(await response.json());
      } catch (error) {
        createChartEmptyState("phChart", `Failed to load chart data: ${error.message}`);
        createChartEmptyState("ppmChart", `Failed to load chart data: ${error.message}`);
        document.getElementById("phChartRange").textContent = "History unavailable";
        document.getElementById("ppmChartRange").textContent = "History unavailable";
        document.getElementById("phChartLatest").textContent = "-";
        document.getElementById("ppmChartLatest").textContent = "-";
      }
    }

    async function refreshReports() {
      try {
        const response = await fetch("/api/reports", { cache: "no-store" });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        applyReports(await response.json());
      } catch (_error) {
      }
    }

    refreshStatus();
    refreshHistory();
    refreshReports();
    setInterval(refreshStatus, refreshIntervalMs);
    setInterval(refreshHistory, historyRefreshIntervalMs);
    setInterval(refreshReports, reportsRefreshIntervalMs);
  </script>
</body>
</html>
)HTML";
}

WebDashboardServer::WebDashboardServer()
    : _server(AppConfig::WEB_SERVER_PORT),
      _recentReportCount(0),
      _recentReportHead(0),
      _historySampleCount(0),
      _historySampleHead(0),
      _lastHistorySampleMs(0),
      _wasWifiConnected(false) {}

void WebDashboardServer::begin() {
    if (!AppConfig::WEB_DASHBOARD_ENABLED) {
        return;
    }

    registerRoutes();
    _server.begin();
    Serial.print("Web dashboard listening on port ");
    Serial.println(AppConfig::WEB_SERVER_PORT);
}

void WebDashboardServer::update(
    const SensorData &sensorData,
    const TargetRanges &targetRanges,
    const char *sensorMode,
    bool calibrationMode,
    DisplayMode displayMode,
    bool dosingBusy,
    const char *dosingState,
    bool wifiConnected,
    const struct tm *localTime,
    bool timeValid
) {
    if (!AppConfig::WEB_DASHBOARD_ENABLED) {
        return;
    }

    _snapshot.sensorData = sensorData;
    _snapshot.targets = targetRanges;
    _snapshot.sensorMode = sensorMode != nullptr ? sensorMode : "MONITOR";
    _snapshot.calibrationMode = calibrationMode;
    _snapshot.displayMode = displayMode;
    _snapshot.dosingBusy = dosingBusy;
    _snapshot.dosingState = dosingState != nullptr ? dosingState : "Monitoring";
    _snapshot.wifiConnected = wifiConnected;
    _snapshot.timeValid = timeValid;
    _snapshot.uptimeSeconds = millis() / 1000UL;

    safeCopy(
        _snapshot.ipAddress,
        sizeof(_snapshot.ipAddress),
        wifiConnected ? WiFi.localIP().toString().c_str() : "0.0.0.0"
    );

    if (timeValid && localTime != nullptr) {
        strftime(_snapshot.date, sizeof(_snapshot.date), "%Y-%m-%d", localTime);
        strftime(_snapshot.time, sizeof(_snapshot.time), "%H:%M:%S", localTime);
    } else {
        safeCopy(_snapshot.date, sizeof(_snapshot.date), "N/A");
        safeCopy(_snapshot.time, sizeof(_snapshot.time), "N/A");
    }

    const unsigned long now = millis();
    if (_historySampleCount == 0 ||
        (now - _lastHistorySampleMs) >= AppConfig::WEB_DASHBOARD_HISTORY_SAMPLE_INTERVAL_MS) {
        addHistorySample(sensorData);
        _lastHistorySampleMs = now;
    }

    if (wifiConnected && !_wasWifiConnected) {
        Serial.print("Dashboard ready at http://");
        Serial.println(_snapshot.ipAddress);
    }

    _wasWifiConnected = wifiConnected;
}

void WebDashboardServer::handleClient() {
    if (!AppConfig::WEB_DASHBOARD_ENABLED) {
        return;
    }

    _server.handleClient();
}

void WebDashboardServer::addCompletedReport(const DosingReport &report) {
    if (!AppConfig::WEB_DASHBOARD_ENABLED) {
        return;
    }

    _recentReports[_recentReportHead] = report;
    _recentReportHead = (_recentReportHead + 1) % kRecentReportsSize;

    if (_recentReportCount < kRecentReportsSize) {
        _recentReportCount++;
    }
}

void WebDashboardServer::registerRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/api/history", HTTP_GET, [this]() { handleHistory(); });
    _server.on("/api/reports", HTTP_GET, [this]() { handleReports(); });
    _server.onNotFound([this]() {
        _server.send(404, "application/json", "{\"error\":\"not_found\"}");
    });
}

void WebDashboardServer::handleRoot() {
    _server.send(200, "text/html; charset=utf-8", buildHtmlPage());
}

void WebDashboardServer::handleStatus() {
    _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    _server.send(200, "application/json; charset=utf-8", buildStatusJson());
}

void WebDashboardServer::handleHistory() {
    _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    _server.send(200, "application/json; charset=utf-8", buildHistoryJson());
}

void WebDashboardServer::handleReports() {
    _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    _server.send(200, "application/json; charset=utf-8", buildRecentReportsJson());
}

String WebDashboardServer::buildHtmlPage() const {
    String page(kDashboardPage);
    page.replace("__DASHBOARD_TITLE__", AppConfig::WEB_DASHBOARD_TITLE);
    page.replace("__REFRESH_INTERVAL_MS__", String(AppConfig::WEB_DASHBOARD_REFRESH_INTERVAL_MS));
    page.replace(
        "__HISTORY_REFRESH_INTERVAL_MS__",
        String(AppConfig::WEB_DASHBOARD_HISTORY_SAMPLE_INTERVAL_MS)
    );
    page.replace(
        "__REPORTS_REFRESH_INTERVAL_MS__",
        String(AppConfig::WEB_DASHBOARD_REPORTS_REFRESH_INTERVAL_MS)
    );
    return page;
}

String WebDashboardServer::buildStatusJson() const {
    String json;
    json.reserve(768);

    json += "{";
    json += "\"device\":{";
    json += "\"wifi_connected\":";
    json += _snapshot.wifiConnected ? "true" : "false";
    json += ",\"ip_address\":\"" + escapeJson(String(_snapshot.ipAddress)) + "\"";
    json += ",\"time_valid\":";
    json += _snapshot.timeValid ? "true" : "false";
    json += ",\"time\":\"" + escapeJson(String(_snapshot.time)) + "\"";
    json += "},";

    json += "\"sensor\":{";
    json += "\"temperature_c\":";
    json += String(_snapshot.sensorData.temperatureC, 2);
    json += ",\"ppm\":";
    json += String(_snapshot.sensorData.tds, 0);
    json += ",\"ph_voltage\":";
    json += String(_snapshot.sensorData.phVoltage, 3);
    json += ",\"ph\":";
    json += String(_snapshot.sensorData.phValue, 2);
    json += "},";

    json += "\"targets\":{";
    json += "\"ph_min\":";
    json += String(_snapshot.targets.phMin, 2);
    json += ",\"ph_max\":";
    json += String(_snapshot.targets.phMax, 2);
    json += ",\"ppm_min\":";
    json += String(_snapshot.targets.ppmMin, 0);
    json += ",\"ppm_max\":";
    json += String(_snapshot.targets.ppmMax, 0);
    json += "},";

    json += "\"dosing\":{";
    json += "\"busy\":";
    json += _snapshot.dosingBusy ? "true" : "false";
    json += ",\"state\":\"" + escapeJson(String(_snapshot.dosingState)) + "\"";
    json += ",\"display_mode\":\"";
    json += displayModeLabel(_snapshot.displayMode);
    json += "\"";
    json += "}";
    json += "}";

    return json;
}

String WebDashboardServer::buildRecentReportsJson() const {
    String json;
    json.reserve(1024);
    json += "[";

    for (size_t offset = 0; offset < _recentReportCount; ++offset) {
        const size_t index =
            (_recentReportHead + kRecentReportsSize - 1 - offset) % kRecentReportsSize;
        const DosingReport &report = _recentReports[index];

        if (offset > 0) {
            json += ",";
        }

        json += "{";
        json += "\"date\":\"" + escapeJson(report.date) + "\"";
        json += ",\"time\":\"" + escapeJson(report.time) + "\"";
        json += ",\"temperature_c\":";
        json += String(report.temperatureC, 2);
        json += ",\"ppm_start\":";
        json += String(report.ppmStart, 0);
        json += ",\"ppm_end\":";
        json += String(report.ppmEnd, 0);
        json += ",\"na_ml\":";
        json += String(report.nutrientAMl, 2);
        json += ",\"nb_ml\":";
        json += String(report.nutrientBMl, 2);
        json += ",\"ph_start\":";
        json += String(report.phStart, 2);
        json += ",\"ph_end\":";
        json += String(report.phEnd, 2);
        json += ",\"ph_down_ml\":";
        json += String(report.phDownMl, 2);
        json += ",\"ph_up_ml\":";
        json += String(report.phUpMl, 2);
        json += ",\"manual_dilution_required\":";
        json += report.manualDilutionRequired ? "true" : "false";
        json += "}";
    }

    json += "]";
    return json;
}

String WebDashboardServer::buildHistoryJson() const {
    String json;
    json.reserve(1024);
    json += "{";
    json += "\"sample_interval_ms\":";
    json += String(AppConfig::WEB_DASHBOARD_HISTORY_SAMPLE_INTERVAL_MS);
    json += ",\"ph_x100\":[";

    const size_t oldestIndex =
        (_historySampleHead + kHistorySamplesSize - _historySampleCount) % kHistorySamplesSize;

    for (size_t offset = 0; offset < _historySampleCount; ++offset) {
        const size_t index = (oldestIndex + offset) % kHistorySamplesSize;
        const HistorySample &sample = _historySamples[index];

        if (offset > 0) {
            json += ",";
        }

        json += String(sample.phX100);
    }

    json += "],\"ppm\":[";

    for (size_t offset = 0; offset < _historySampleCount; ++offset) {
        const size_t index = (oldestIndex + offset) % kHistorySamplesSize;
        const HistorySample &sample = _historySamples[index];

        if (offset > 0) {
            json += ",";
        }

        json += String(sample.ppm);
    }

    json += "]}";
    return json;
}

void WebDashboardServer::addHistorySample(const SensorData &sensorData) {
    HistorySample &sample = _historySamples[_historySampleHead];
    sample.phX100 = encodePhX100(sensorData.phValue);
    sample.ppm = encodePpm(sensorData.tds);

    _historySampleHead = (_historySampleHead + 1) % kHistorySamplesSize;

    if (_historySampleCount < kHistorySamplesSize) {
        _historySampleCount++;
    }
}

void WebDashboardServer::safeCopy(char *destination, size_t destinationSize, const char *source) {
    if (destination == nullptr || destinationSize == 0) {
        return;
    }

    const char *safeSource = source != nullptr ? source : "";
    snprintf(destination, destinationSize, "%s", safeSource);
}

String WebDashboardServer::escapeJson(const String &value) {
    String escaped;
    escaped.reserve(value.length() + 8);

    for (size_t index = 0; index < value.length(); ++index) {
        const char character = value.charAt(index);

        switch (character) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += character;
                break;
        }
    }

    return escaped;
}

uint16_t WebDashboardServer::encodePhX100(float phValue) {
    if (phValue <= 0.0f) {
        return 0;
    }

    if (phValue >= 14.0f) {
        return 1400;
    }

    return static_cast<uint16_t>((phValue * 100.0f) + 0.5f);
}

uint16_t WebDashboardServer::encodePpm(float ppmValue) {
    if (ppmValue <= 0.0f) {
        return 0;
    }

    if (ppmValue >= 5000.0f) {
        return 5000;
    }

    return static_cast<uint16_t>(ppmValue + 0.5f);
}

const char *WebDashboardServer::displayModeLabel(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::PH_DOWN_DOSE:
            return "PH_DOWN_DOSE";

        case DisplayMode::PH_DOWN_WAIT:
            return "PH_DOWN_WAIT";

        case DisplayMode::PH_UP_DOSE:
            return "PH_UP_DOSE";

        case DisplayMode::PH_UP_WAIT:
            return "PH_UP_WAIT";

        case DisplayMode::NUTRI_AB:
            return "NUTRI_AB";

        case DisplayMode::NUTRI_AB_WAIT:
            return "NUTRI_AB_WAIT";

        case DisplayMode::NORMAL:
        default:
            return "NORMAL";
    }
}
