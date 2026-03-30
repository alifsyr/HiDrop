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

    .grid {
      display: grid;
      grid-template-columns: 1.4fr 1fr;
      gap: 18px;
      margin-top: 18px;
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

    .kv {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 14px;
    }

    .kv-item {
      padding: 16px;
      border-radius: 18px;
      background: rgba(13, 141, 119, 0.06);
    }

    .kv-item .k {
      display: block;
      font-size: 0.85rem;
      color: var(--muted);
      margin-bottom: 6px;
    }

    .kv-item .v {
      font-size: 1.1rem;
      font-weight: 700;
    }

    .ranges {
      display: grid;
      gap: 12px;
    }

    .range-track {
      position: relative;
      height: 12px;
      border-radius: 999px;
      background: rgba(19, 52, 43, 0.08);
      overflow: hidden;
    }

    .range-fill,
    .range-marker {
      position: absolute;
      top: 0;
      bottom: 0;
      border-radius: 999px;
    }

    .range-fill {
      background: rgba(13, 141, 119, 0.18);
    }

    .range-marker {
      width: 14px;
      top: -3px;
      bottom: -3px;
      border: 3px solid white;
      background: var(--accent);
      box-shadow: 0 10px 22px rgba(13, 141, 119, 0.25);
      transform: translateX(-50%);
    }

    .badge {
      display: inline-flex;
      align-items: center;
      padding: 6px 12px;
      border-radius: 999px;
      font-size: 0.8rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .badge.ok { color: var(--ok); background: rgba(21, 128, 61, 0.1); }
    .badge.warn { color: var(--warn); background: rgba(183, 121, 31, 0.12); }
    .badge.danger { color: var(--danger); background: rgba(180, 35, 24, 0.1); }

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

    .footer-note {
      margin-top: 18px;
      color: var(--muted);
      font-size: 0.9rem;
    }

    .empty {
      color: var(--muted);
      font-style: italic;
    }

    @media (max-width: 860px) {
      .grid {
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

    <section class="grid">
      <article class="panel">
        <h2>Range Monitor</h2>
        <div class="ranges">
          <div>
            <div class="meta" style="display:flex;justify-content:space-between;gap:10px">
              <strong>pH Range</strong>
              <span id="phBand" class="badge warn">-</span>
            </div>
            <div class="range-track" aria-hidden="true" style="margin-top:10px">
              <div id="phFill" class="range-fill"></div>
              <div id="phMarker" class="range-marker"></div>
            </div>
            <div class="meta" id="phRangeText">-</div>
          </div>

          <div>
            <div class="meta" style="display:flex;justify-content:space-between;gap:10px">
              <strong>PPM Range</strong>
              <span id="ppmBand" class="badge warn">-</span>
            </div>
            <div class="range-track" aria-hidden="true" style="margin-top:10px">
              <div id="ppmFill" class="range-fill"></div>
              <div id="ppmMarker" class="range-marker"></div>
            </div>
            <div class="meta" id="ppmRangeText">-</div>
          </div>
        </div>

        <div class="footer-note">
          Dashboard refresh otomatis setiap beberapa detik. Untuk pemantauan lewat internet, publikasikan alamat dashboard ini melalui router, VPN, atau tunnel yang aman.
        </div>
      </article>

      <article class="panel">
        <h2>System Status</h2>
        <div class="kv">
          <div class="kv-item">
            <span class="k">Sensor Mode</span>
            <span class="v" id="sensorMode">-</span>
          </div>
          <div class="kv-item">
            <span class="k">Display Mode</span>
            <span class="v" id="displayMode">-</span>
          </div>
          <div class="kv-item">
            <span class="k">Calibration</span>
            <span class="v" id="calibrationMode">-</span>
          </div>
          <div class="kv-item">
            <span class="k">Local Time</span>
            <span class="v" id="localTime">-</span>
          </div>
          <div class="kv-item">
            <span class="k">Uptime</span>
            <span class="v mono" id="uptime">-</span>
          </div>
          <div class="kv-item">
            <span class="k">Device Reachability</span>
            <span class="v" id="reachability">-</span>
          </div>
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

    function formatNumber(value, digits) {
      if (typeof value !== "number" || Number.isNaN(value)) return "-";
      return value.toFixed(digits);
    }

    function setBadge(element, label) {
      element.textContent = label;
      element.className = "badge " + (label === "OK" ? "ok" : (label === "LOW" || label === "HIGH" ? "warn" : "danger"));
    }

    function setMarker(fillEl, markerEl, value, min, max, domainMin, domainMax) {
      const clamp = (input, low, high) => Math.min(high, Math.max(low, input));
      const span = Math.max(domainMax - domainMin, 0.0001);
      const fillStart = ((min - domainMin) / span) * 100;
      const fillEnd = ((max - domainMin) / span) * 100;
      const marker = ((value - domainMin) / span) * 100;
      fillEl.style.left = clamp(fillStart, 0, 100) + "%";
      fillEl.style.width = clamp(fillEnd - fillStart, 0, 100) + "%";
      markerEl.style.left = clamp(marker, 0, 100) + "%";
    }

    function formatDuration(totalSeconds) {
      const seconds = Math.max(0, Number(totalSeconds) || 0);
      const hours = Math.floor(seconds / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      const secs = Math.floor(seconds % 60);
      return `${hours}h ${minutes}m ${secs}s`;
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

    function applyStatus(data) {
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

      setBadge(document.getElementById("phBand"), data?.sensor?.ph_band || "UNKNOWN");
      setBadge(document.getElementById("ppmBand"), data?.sensor?.ppm_band || "UNKNOWN");
      document.getElementById("phRangeText").textContent = `Live ${formatNumber(data?.sensor?.ph, 2)} | Target ${formatNumber(data?.targets?.ph_min, 2)} - ${formatNumber(data?.targets?.ph_max, 2)}`;
      document.getElementById("ppmRangeText").textContent = `Live ${formatNumber(data?.sensor?.ppm, 0)} | Target ${formatNumber(data?.targets?.ppm_min, 0)} - ${formatNumber(data?.targets?.ppm_max, 0)}`;
      setMarker(
        document.getElementById("phFill"),
        document.getElementById("phMarker"),
        Number(data?.sensor?.ph || 0),
        Number(data?.targets?.ph_min || 0),
        Number(data?.targets?.ph_max || 0),
        0,
        14
      );
      setMarker(
        document.getElementById("ppmFill"),
        document.getElementById("ppmMarker"),
        Number(data?.sensor?.ppm || 0),
        Number(data?.targets?.ppm_min || 0),
        Number(data?.targets?.ppm_max || 0),
        0,
        Math.max(1500, Number(data?.targets?.ppm_max || 0) * 1.4)
      );

      document.getElementById("sensorMode").textContent = data?.sensor?.mode || "-";
      document.getElementById("displayMode").textContent = data?.dosing?.display_mode || "-";
      document.getElementById("calibrationMode").textContent = data?.sensor?.calibration_mode ? "Active" : "Off";
      document.getElementById("localTime").textContent = data?.device?.time_valid
        ? `${data?.device?.date || "-"} ${data?.device?.time || "-"}`
        : "Waiting for NTP sync";
      document.getElementById("uptime").textContent = formatDuration(data?.device?.uptime_seconds);
      document.getElementById("reachability").textContent = wifiOk ? "Local dashboard reachable" : "Offline";

      renderReports(data?.reports || []);
    }

    async function refresh() {
      try {
        const response = await fetch("/api/status", { cache: "no-store" });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        applyStatus(await response.json());
      } catch (error) {
        document.getElementById("heroStatus").textContent = `Failed to load live data: ${error.message}`;
        document.getElementById("wifiDot").classList.remove("ok");
      }
    }

    refresh();
    setInterval(refresh, refreshIntervalMs);
  </script>
</body>
</html>
)HTML";
}

WebDashboardServer::WebDashboardServer()
    : _server(AppConfig::WEB_SERVER_PORT),
      _recentReportCount(0),
      _recentReportHead(0),
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

String WebDashboardServer::buildHtmlPage() const {
    String page(kDashboardPage);
    page.replace("__DASHBOARD_TITLE__", AppConfig::WEB_DASHBOARD_TITLE);
    page.replace("__REFRESH_INTERVAL_MS__", String(AppConfig::WEB_DASHBOARD_REFRESH_INTERVAL_MS));
    return page;
}

String WebDashboardServer::buildStatusJson() const {
    String json;
    json.reserve(3072);

    json += "{";
    json += "\"device\":{";
    json += "\"wifi_connected\":";
    json += _snapshot.wifiConnected ? "true" : "false";
    json += ",\"ip_address\":\"" + escapeJson(String(_snapshot.ipAddress)) + "\"";
    json += ",\"time_valid\":";
    json += _snapshot.timeValid ? "true" : "false";
    json += ",\"date\":\"" + escapeJson(String(_snapshot.date)) + "\"";
    json += ",\"time\":\"" + escapeJson(String(_snapshot.time)) + "\"";
    json += ",\"uptime_seconds\":";
    json += String(_snapshot.uptimeSeconds);
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
    json += ",\"mode\":\"" + escapeJson(String(_snapshot.sensorMode)) + "\"";
    json += ",\"calibration_mode\":";
    json += _snapshot.calibrationMode ? "true" : "false";
    json += ",\"ph_band\":\"";
    json += sensorBandLabel(_snapshot.sensorData.phValue, _snapshot.targets.phMin, _snapshot.targets.phMax);
    json += "\"";
    json += ",\"ppm_band\":\"";
    json += sensorBandLabel(_snapshot.sensorData.tds, _snapshot.targets.ppmMin, _snapshot.targets.ppmMax);
    json += "\"";
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
    json += "},";

    json += "\"reports\":";
    json += buildRecentReportsJson();
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

const char *WebDashboardServer::sensorBandLabel(float value, float minValue, float maxValue) {
    if (value < minValue) {
        return "LOW";
    }

    if (value > maxValue) {
        return "HIGH";
    }

    return "OK";
}
