#!/usr/bin/env python3

import os
import re
import shutil
import subprocess
import tempfile
import time


PROMPT = "$ "
# Wait for energyinfo output completion to capture idle sleep count metrics
DONE = "schedbench energyinfo done"


def wait_for_output(proc, pattern, timeout):
    deadline = time.time() + timeout
    output = ""

    while time.time() < deadline:
      chunk = os.read(proc.stdout.fileno(), 4096).decode("utf-8", "replace")
      output += chunk
      if pattern in output:
        return output

    raise RuntimeError(f"timed out waiting for {pattern!r}")


def build():
    subprocess.run(["make", "kernel/kernel"], check=True)
    subprocess.run(["make", "fs.img"], check=True)


def run_schedbench():
    qemu = None
    output = ""

    with tempfile.TemporaryDirectory() as tempdir:
        fs_copy = os.path.join(tempdir, "fs.img")
        shutil.copyfile("fs.img", fs_copy)

        cmd = [
            "qemu-system-riscv64",
            "-machine", "virt",
            "-bios", "none",
            "-kernel", "kernel/kernel",
            "-m", "128M",
            "-smp", "1",
            "-nographic",
            "-global", "virtio-mmio.force-legacy=false",
            "-drive", f"file={fs_copy},if=none,format=raw,id=x0",
            "-device", "virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0",
        ]
        qemu = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        try:
            output += wait_for_output(qemu, PROMPT, 15)
            qemu.stdin.write(b"schedbench\n")
            qemu.stdin.flush()
            output += wait_for_output(qemu, DONE, 30)
        finally:
            if qemu.poll() is None:
                qemu.stdin.write(b"\x01x")
                qemu.stdin.flush()
                qemu.wait(timeout=5)

    return output


def assert_behavior(output):
    order = {}
    for match in re.finditer(r"schedbench reap order=(\d+) job=(\w+)", output):
        order[match.group(2)] = int(match.group(1))

    required = {"shortjob", "mediumjob", "longjob", "burstjob"}
    missing = required - order.keys()
    if missing:
        raise RuntimeError(f"missing completion lines for: {', '.join(sorted(missing))}")

    if not (order["shortjob"] < order["longjob"] < order["burstjob"]):
        raise RuntimeError(f"unexpected completion order: {order}")
    if not (order["mediumjob"] < order["longjob"]):
        raise RuntimeError(f"mediumjob should finish before longjob: {order}")


def main():
    build()
    output = run_schedbench()
    print(output)
    assert_behavior(output)
    print("schedbench test passed")


if __name__ == "__main__":
    main()
