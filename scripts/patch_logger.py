import os

files = [
    "src/control/dosing_controller.cpp",
    "src/control/target_range_manager.cpp",
    "src/control/sensor_manager.cpp"
]

for f in files:
    with open(f, "r") as file:
        content = file.read()
    
    # Remove the bad define
    content = content.replace('#define Serial Logger\n', '')
    content = content.replace('#define Serial Logger', '')
    
    # Replace Serial. with Logger::
    content = content.replace('Serial.print', 'Logger::print')
    content = content.replace('Serial.printf', 'Logger::printf')
    
    with open(f, "w") as file:
        file.write(content)
        
    print(f"Re-patched {f}")
