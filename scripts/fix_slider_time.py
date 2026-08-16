import re

with open('docs/index.html', 'r') as f:
    html = f.read()

# Fix time parsing (replace '-' with '/' and remove 'T')
old_time = "const espTime = new Date(`${data.device.date}T${data.device.time}`).getTime();"
new_time = "const espTime = new Date(`${data.device.date.replace(/-/g, '/')} ${data.device.time}`).getTime();"
html = html.replace(old_time, new_time)

# Add updateDualSlider if missing
if 'function updateDualSlider' not in html:
    slider_func = """
    function updateDualSlider(triggerEl, prefix) {
      const minSlider = document.getElementById(prefix + 'MinSlider');
      const maxSlider = document.getElementById(prefix + 'MaxSlider');
      const minVal = document.getElementById(prefix + 'MinVal');
      const maxVal = document.getElementById(prefix + 'MaxVal');
      const progress = document.getElementById(prefix + 'Progress');

      if(!minSlider || !maxSlider) return;

      let min = parseFloat(minSlider.value);
      let max = parseFloat(maxSlider.value);
      
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

      const total = parseFloat(minSlider.max) - parseFloat(minSlider.min);
      const leftPercent = ((min - parseFloat(minSlider.min)) / total) * 100;
      const widthPercent = ((max - min) / total) * 100;

      if(progress) {
        progress.style.left = leftPercent + '%';
        progress.style.width = widthPercent + '%';
      }
    }
"""
    # Insert it right before "let isEditingTargets = false;"
    html = html.replace('let isEditingTargets = false;', slider_func + '\n    let isEditingTargets = false;')

with open('docs/index.html', 'w') as f:
    f.write(html)
print("Fixed!")
