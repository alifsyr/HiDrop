import re

with open('docs/index.html', 'r') as f:
    html = f.read()

# Fix timezone offline logic
old_logic = """      // Calculate age of data to determine if online
      const nowMs = Date.now();
      const lastUpdateMs = data.timestamp || 0;
      const isOnline = (nowMs - lastUpdateMs) < 60000; // considered offline if no update for 60s"""

new_logic = """      // Calculate age of data to determine if online
      const nowMs = Date.now();
      let isOnline = false;
      let lastUpdateMs = nowMs;
      
      if (data.device && data.device.time_valid) {
          // Use absolute time if ESP32 has valid RTC time
          const espTime = new Date(`${data.device.date.replace(/-/g, '/')} ${data.device.time}`).getTime();
          if (!isNaN(espTime)) {
              lastUpdateMs = espTime;
              isOnline = Math.abs(nowMs - espTime) < 65000; // 65 seconds tolerance
          }
      }
      
      // Fallback if no valid time or if we want extra safety: use uptime changes
      if (!isOnline && data.device && data.device.uptime_seconds) {
          if (!window._lastUptime || window._lastUptime !== data.device.uptime_seconds) {
              window._lastUptime = data.device.uptime_seconds;
              window._lastUpdateMs = nowMs;
          }
          lastUpdateMs = window._lastUpdateMs || nowMs;
          isOnline = (nowMs - lastUpdateMs) < 65000;
      }"""
html = html.replace(old_logic, new_logic)

# Make sure otaBadge is always visible
html = html.replace(
    "document.getElementById(\"otaBadge\").style.display = data?.device?.ota_available ? 'block' : 'none';",
    "// otaBadge is always visible so user can click it\n      if(data?.device?.ota_available) {\n        document.getElementById('otaBadge').style.color = 'var(--warn)';\n      } else {\n        document.getElementById('otaBadge').style.color = 'var(--text)';\n      }"
)

with open('docs/index.html', 'w') as f:
    f.write(html)
print("Done")
