import re

with open('dashboard_preview.html', 'r') as f:
    preview = f.read()

with open('docs/index.html', 'r') as f:
    live = f.read()

# We want the HTML and CSS of preview, but the JS of live.
# Actually, the JS of live needs to be modified to also call applyReports and update dual slider.
# It's easier to just write the final HTML as a string in the script.

final_html = preview

# Replace the __DASHBOARD_TITLE__ with 'Hydroponic Monitor (Cloud)'
final_html = final_html.replace('__DASHBOARD_TITLE__', 'Hydroponic Monitor (Cloud)')

# Extract the Firebase initialization and logic from live
firebase_script = """
  <script type="module">
    import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js";
    import { getDatabase, ref, onValue, set, get, child } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-database.js";

    const firebaseConfig = {
      apiKey: "AIzaSyBndjmNk-7gose5h_pM1bYX1rMI6Z-CgHE",
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
"""

# Now replace the <script> block in preview
script_start = final_html.find('<script>')
script_end = final_html.find('</script>', script_start) + len('</script>')

# Get the script content of preview, remove the fetch stuff
preview_script = final_html[script_start+8:script_end-9]

# Remove the __REFRESH_INTERVAL_MS__ stuff and fetch functions
preview_script = re.sub(r'const refreshIntervalMs = __REFRESH_INTERVAL_MS__;.*let latestHistory = null;', 'let latestStatus = null;\n    let latestHistory = null;', preview_script, flags=re.DOTALL)

# Remove refreshStatus, refreshHistory, refreshReports fetch functions
preview_script = re.sub(r'async function refreshStatus\(\).*?async function refreshReports\(\) \{.*?\n    \}', '', preview_script, flags=re.DOTALL)

# Replace the saveTarget fetch with Firebase set
save_target_fb = """
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
"""
preview_script = re.sub(r'async function saveTarget\(prefix\).*?setTimeout\(\(\) => \{ status.style.display = \'none\'; \}, 2000\);\n      \}\n    \}', save_target_fb, preview_script, flags=re.DOTALL)

# Remove OTA fetches (just make them dummy for now)
check_ota_fb = """
    async function checkOtaUpdate() {
      const btn = document.getElementById('otaCheckBtn');
      const status = document.getElementById('otaStatusText');
      btn.disabled = true;
      btn.textContent = 'Checking...';
      status.textContent = 'OTA update via Firebase is not yet supported.';
      status.style.color = 'var(--warn)';
      setTimeout(() => {
        btn.disabled = false;
        btn.textContent = 'Check Update Now';
      }, 3000);
    }
    
    async function startOtaUpdate() {
        // Not supported yet
    }
"""
preview_script = re.sub(r'async function checkOtaUpdate\(\).*?async function startOtaUpdate\(\).*? btn.textContent = \'Install Update & Restart\';\n      \}\n    \}', check_ota_fb, preview_script, flags=re.DOTALL)

# Remove the setIntervals at the end
preview_script = re.sub(r'refreshStatus\(\).*setInterval\(refreshReports, reportsRefreshIntervalMs\);', '', preview_script, flags=re.DOTALL)

# Modify applyStatus to check timestamp for offline status
apply_status_mod = """
      const nowMs = Date.now();
      const lastUpdateMs = data.timestamp || 0;
      const isOnline = (nowMs - lastUpdateMs) < 60000;
      document.getElementById("heroStatus").textContent = isOnline
        ? "Device online and serving live telemetry"
        : "Device offline (showing last known state)";
      document.getElementById("wifiDot").classList.toggle("ok", isOnline);
      document.getElementById("refreshedAt").textContent = new Date(lastUpdateMs || nowMs).toLocaleTimeString();
"""
preview_script = preview_script.replace('const wifiOk = Boolean(data?.device?.wifi_connected);\n      document.getElementById("heroStatus").textContent = wifiOk\n        ? "Device online and serving live telemetry"\n        : "Wi-Fi belum tersambung, menunggu koneksi";\n      document.getElementById("wifiDot").classList.toggle("ok", wifiOk);\n      document.getElementById("refreshedAt").textContent = new Date().toLocaleTimeString();', apply_status_mod)

# Add Firebase listeners at the end
firebase_listeners = """
    // Connect to Firebase and listen to changes
    if(db) {
      const statusRef = ref(db, 'hydroponic/status');
      onValue(statusRef, (snapshot) => {
        if(snapshot.exists()) applyStatus(snapshot.val());
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
    }, 5000);
"""

final_script = firebase_script + preview_script + firebase_listeners + "\n  </script>"

final_html = final_html[:script_start] + final_script + final_html[script_end:]

with open('docs/index.html', 'w') as f:
    f.write(final_html)

print("Merged successfully!")
