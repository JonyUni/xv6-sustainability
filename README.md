# xv6 Sustainability Project

This repository contains a modified xv6 kernel for an operating-systems project centered on environmental sustainability. The current implemented feature is Feature 1: Energy-Aware Scheduler.

## Git and GitHub on WSL Ubuntu

These steps assume you are in **Ubuntu on WSL** (the same environment where you build and run xv6).

### Install Git

```bash
sudo apt update
sudo apt install -y git
git --version
```

### Configure your identity

Use the same name and email you use on GitHub (commits are labeled with these values).

```bash
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
```

Optional: set your default branch name to match this repo.

```bash
git config --global init.defaultBranch main
```

### Connect WSL to GitHub

You need a [GitHub](https://github.com) account. Then choose **SSH** (recommended) or **HTTPS**.

**SSH**

1. Generate a key (press Enter to accept the default path; set a passphrase if you want).

   ```bash
   ssh-keygen -t ed25519 -C "you@example.com"
   ```

2. Start the agent and add the key (typical on Ubuntu/WSL):

   ```bash
   eval "$(ssh-agent -s)"
   ssh-add ~/.ssh/id_ed25519
   ```

3. Copy the **public** key and add it in GitHub under **Settings → SSH and GPG keys → New SSH key**:

   ```bash
   cat ~/.ssh/id_ed25519.pub
   ```

4. Test:

   ```bash
   ssh -T git@github.com
   ```

**HTTPS**

Clone and pull work without extra setup. For `git push`, GitHub requires a **personal access token** instead of your account password. Create one under **Settings → Developer settings → Personal access tokens**, then use it when Git prompts for a password, or configure a credential helper so you are not prompted every time.

### Clone this repository (first time)

Pick a parent directory where you keep code (for example your home directory or `~/projects`), then:

```bash
cd ~
git clone https://github.com/JonyUni/xv6-sustainability.git
cd xv6-sustainability
```

If you use SSH with GitHub:

```bash
git clone git@github.com:JonyUni/xv6-sustainability.git
cd xv6-sustainability
```

### Pull the latest changes

When you already have a clone and want to update it to match GitHub:

```bash
cd xv6-sustainability
git pull origin main
```

If your default branch is not `main`, replace `main` with the branch name shown on GitHub (for example `git branch -r` to see remotes).

Resolve any merge conflicts if you have local commits that diverge from `origin/main`.

## Repository Layout

- `xv6/`: xv6 kernel, user programs, build system, and test scripts
- `OS Project.md`: project brief and feature outline

## Feature 1: Energy-Aware Scheduler

### Goal

This feature changes xv6 scheduling decisions to better match an energy-saving policy:

- prefer short-running processes
- penalize CPU-heavy processes
- allow small uninterrupted bursts so short jobs can finish sooner
- reduce unnecessary scheduler churn compared with one-tick round-robin behavior

This is a simulated energy-aware policy. It does not measure real hardware power.

### How It Was Implemented

The scheduler changes are in [proc.c](/home/jonas/xv6-sustainability/xv6/kernel/proc.c), [proc.h](/home/jonas/xv6-sustainability/xv6/kernel/proc.h), and [trap.c](/home/jonas/xv6-sustainability/xv6/kernel/trap.c).

Per-process scheduling state was added to `struct proc`:

- `run_ticks`: total CPU time the process has consumed
- `burst_ticks`: ticks consumed in the current uninterrupted burst
- `recent_burst_ticks`: length of the most recent completed CPU burst
- `times_scheduled`: number of times the process was dispatched

Timer interrupts now update process runtime accounting through `proc_tick_and_should_yield()`. Instead of forcing a yield on every timer tick, the scheduler allows a process to continue for up to 3 ticks before preempting it. This creates a small burst window that helps short jobs finish quickly.

The process-selection policy is no longer pure round robin. The scheduler scans all `RUNNABLE` processes and chooses the one with the lowest score:

```c
score = run_ticks + 2 * recent_burst_ticks
```

This means:

- lower total runtime is favored
- recent CPU hogging is penalized
- ties fall back to lower `run_ticks`, then lower `pid`

### Scheduler Stats Interface

To make the feature testable from user space, a syscall named `getschedstats()` was added. It returns:

- process id
- total runtime ticks
- most recent burst length
- number of times scheduled

The shared type is defined in [schedstats.h](/home/jonas/xv6-sustainability/xv6/kernel/schedstats.h).

## Testing Feature 1

### What Is Tested

The test checks whether the scheduler behaves consistently with the green policy by running multiple workloads at the same time and verifying that shorter CPU-bound work completes before longer CPU-bound work.

The benchmark uses four user programs:

- `shortjob`
- `mediumjob`
- `longjob`
- `burstjob`

These are launched together by `schedbench`, which synchronizes their start time and prints:

- start tick
- finish tick
- wall-clock ticks
- `run_ticks`
- `times_scheduled`
- completion order

### Automated Test

Run the automated benchmark from the xv6 directory:

```bash
cd /home/jonas/xv6-sustainability/xv6
python3 test-schedbench.py
```

This script:

1. builds `kernel/kernel`
2. rebuilds `fs.img`
3. boots xv6 in QEMU
4. runs `schedbench`
5. checks that completion order matches the intended policy

The test currently passes when:

- `shortjob` finishes before `longjob`
- `mediumjob` finishes before `longjob`
- `longjob` finishes before `burstjob`

### Manual Test

You can also run the benchmark manually:

```bash
cd /home/jonas/xv6-sustainability/xv6
make qemu
```

Then inside xv6:

```text
schedbench
```

Expected behavior:

- `shortjob` should complete first
- `mediumjob` should complete before `longjob`
- `longjob` should take more `run_ticks` than shorter jobs
- `burstjob` should finish last because it alternates compute with sleeps

Example output from a passing run:

```text
schedbench reap order=1 job=shortjob pid=4 tick=45 status=0
schedbench reap order=2 job=mediumjob pid=5 tick=52 status=0
schedbench reap order=3 job=longjob pid=6 tick=56 status=0
schedbench reap order=4 job=burstjob pid=7 tick=69 status=0
```

Representative per-process stats from the same run:

```text
shortjob done ... wall=1 run_ticks=1 picks=6
mediumjob done ... wall=6 run_ticks=3 picks=7
longjob done ... wall=7 run_ticks=6 picks=8
```

These results show that shorter jobs are favored and CPU-heavier jobs accumulate more runtime before completion.

## Build Notes

Basic kernel build:

```bash
cd /home/jonas/xv6-sustainability/xv6
make kernel/kernel
```

Rebuild filesystem image with user programs:

```bash
cd /home/jonas/xv6-sustainability/xv6
make fs.img
```

## Current Limitations

- This feature is currently the default scheduler behavior; the project does not yet include the later eco/performance mode toggle from Feature 4.
- The energy model is simulated through scheduler behavior and runtime statistics, not real power measurements.
- The benchmark is designed to validate policy behavior, not to prove actual hardware watt savings.

## Relevant Files

- [proc.c](/home/jonas/xv6-sustainability/xv6/kernel/proc.c)
- [proc.h](/home/jonas/xv6-sustainability/xv6/kernel/proc.h)
- [trap.c](/home/jonas/xv6-sustainability/xv6/kernel/trap.c)
- [sysproc.c](/home/jonas/xv6-sustainability/xv6/kernel/sysproc.c)
- [schedstats.h](/home/jonas/xv6-sustainability/xv6/kernel/schedstats.h)
- [schedbench.c](/home/jonas/xv6-sustainability/xv6/user/schedbench.c)
- [test-schedbench.py](/home/jonas/xv6-sustainability/xv6/test-schedbench.py)
