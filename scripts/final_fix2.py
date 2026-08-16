with open('docs/index.html', 'r') as f:
    html = f.read()

html = html.replace(
    "// otaBadge is always visible so user can click it\n      if(data?.device?.ota_available) {\n        document.getElementById('otaBadge').style.color = 'var(--warn)';\n      } else {\n        document.getElementById('otaBadge').style.color = 'var(--text)';\n      }",
    "if(data?.device?.ota_available) {\n        document.getElementById('otaBadge').style.display = 'block';\n      } else {\n        document.getElementById('otaBadge').style.display = 'none';\n      }"
)

with open('docs/index.html', 'w') as f:
    f.write(html)
