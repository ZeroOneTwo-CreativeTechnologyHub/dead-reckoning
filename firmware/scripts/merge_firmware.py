Import("env")
import os
import subprocess

def merge_firmware(source, target, env):
    firmware_dir = env.subst("$BUILD_DIR")
    merged_path = os.path.join(env.subst("$PROJECT_DIR"), "..", "docs", "assets", "firmware.bin")
    
    bootloader = os.path.join(firmware_dir, "bootloader.bin")
    partitions = os.path.join(firmware_dir, "partitions.bin")
    boot_app  = os.path.join(firmware_dir, "boot_app0.bin")
    app       = os.path.join(firmware_dir, env.subst("${PROGNAME}.bin"))

    cmd = [
        "esptool.py",
        "--chip", "esp32s3",
        "merge_bin",
        "-o", merged_path,
        "--flash_mode", "dio",
        "--flash_freq", "80m",
        "--flash_size", "8MB",
        "0x0000", bootloader,
        "0x8000", partitions,
        "0xe000", boot_app,
        "0x10000", app,
    ]

    print("\n=== Merging firmware for ESP Web Tools ===")
    print(f"Output: {merged_path}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("esptool merge failed:", result.stderr)
    else:
        print("Merged firmware written successfully.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_firmware)
