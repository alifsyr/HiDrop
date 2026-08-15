import os
from pathlib import Path
from typing import Any


env: Any = None

try:
    from SCons.Script import Import  # type: ignore
except ImportError:
    def Import(_name):
        raise RuntimeError("scripts/load_env.py must be executed by PlatformIO/SCons.")


Import("env")

if env is None:
    raise RuntimeError("PlatformIO did not provide the build environment.")


PROJECT_DIR = Path(env["PROJECT_DIR"])
ENV_FILE_CANDIDATES = [PROJECT_DIR / ".env.local", PROJECT_DIR / ".env"]


def parse_dotenv(path):
    values = {}

    if not path.exists():
        return values

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()

        if not line or line.startswith("#") or "=" not in line:
            continue

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()

        if value.startswith(("\"", "'")) and value.endswith(("\"", "'")) and len(value) >= 2:
            value = value[1:-1]

        values[key] = value

    return values


def load_env_values():
    values = {}
    for candidate in ENV_FILE_CANDIDATES:
        values.update(parse_dotenv(candidate))
    return values


def get_value(name, fallback=""):
    return os.environ.get(name, FILE_ENV.get(name, fallback))


def as_bool_literal(value):
    normalized = str(value).strip().lower()
    return "true" if normalized in ("1", "true", "yes", "on") else "false"


def quote_cpp_string(value):
    escaped = str(value).replace("\\", "\\\\").replace('"', '\\"')
    return '\\"{}\\"'.format(escaped)


FILE_ENV = load_env_values()

env.Append(
    CPPDEFINES=[
        ("APP_WIFI_SSID", quote_cpp_string(get_value("WIFI_SSID"))),
        ("APP_WIFI_PASSWORD", quote_cpp_string(get_value("WIFI_PASSWORD"))),
        ("APP_GOOGLE_SHEETS_WEB_APP_URL", quote_cpp_string(get_value("GOOGLE_SHEETS_WEB_APP_URL"))),
        ("APP_GOOGLE_SHEETS_SHARED_SECRET", quote_cpp_string(get_value("GOOGLE_SHEETS_SHARED_SECRET"))),
        ("APP_GOOGLE_SHEETS_LOGGING_ENABLED", as_bool_literal(get_value("GOOGLE_SHEETS_LOGGING_ENABLED", "false"))),
    ]
)
