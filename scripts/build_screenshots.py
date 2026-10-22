#!/usr/bin/env python3
"""Build a separate screenshot-only watchface that advances every 10 seconds."""
import argparse
import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / ".screenshot-build"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", action="store_true", help="Install via CloudPebble after building")
    args = parser.parse_args()
    if DEST.exists():
        shutil.rmtree(DEST)
    DEST.mkdir()
    shutil.copytree(ROOT / "src", DEST / "src")
    shutil.copytree(ROOT / "resources", DEST / "resources")
    shutil.copy2(ROOT / "wscript", DEST / "wscript")
    package = json.loads((ROOT / "package.json").read_text(encoding="utf-8"))
    package["pebble"]["displayName"] = "Extinct Birds Screenshots"
    # Separate app ID: never overwrite the daily production watchface.
    package["pebble"]["uuid"] = "e1d451da-1b54-4d3a-88bf-fd9cb2c932a1"
    (DEST / "package.json").write_text(json.dumps(package, indent=2) + "\n", encoding="utf-8")

    source = (DEST / "src/main.c").read_text(encoding="utf-8")
    old = "static void tick_handler(struct tm *tick,TimeUnits changed){update_display(tick);}"
    new = """static AppTimer *s_screenshot_timer;
static void screenshot_advance(void *context) {
  display_bird((s_bird_index + 7) % NUM_BIRD);
  s_screenshot_timer = app_timer_register(10000, screenshot_advance, NULL);
}
static void tick_handler(struct tm *tick,TimeUnits changed){update_display(tick);}"""
    if source.count(old) != 1:
        raise RuntimeError("Expected tick handler not found; check src/main.c")
    source = source.replace(old, new)
    # Prevent midnight's daily update from interfering with screenshot sequencing.
    source = source.replace(
        "if(day!=s_day) {s_day=day;display_bird(daily_bird(day));}",
        "if(day!=s_day) {s_day=day;if(s_bird_index<0)display_bird(daily_bird(day));}"
    )
    source = source.replace(
        "  tick_timer_service_subscribe(MINUTE_UNIT,tick_handler);",
        "  tick_timer_service_subscribe(MINUTE_UNIT,tick_handler);\n"
        "  s_screenshot_timer = app_timer_register(10000, screenshot_advance, NULL);"
    )
    source = source.replace(
        "  tick_timer_service_unsubscribe();",
        "  tick_timer_service_unsubscribe();\n"
        "  if(s_screenshot_timer) app_timer_cancel(s_screenshot_timer);"
    )
    (DEST / "src/main.c").write_text(source, encoding="utf-8")
    subprocess.run(["pebble", "build"], cwd=DEST, check=True)
    print("Screenshot build:", DEST / "build/extinct-birds-pebble-watchface.pbw")
    if args.install:
        subprocess.run(["pebble", "install", "--cloudpebble"], cwd=DEST, check=True)

if __name__ == "__main__":
    main()
