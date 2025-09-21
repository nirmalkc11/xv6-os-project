#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# CSE3320 Project-1 Autograder
# - Runs `make qemu-nox`
# - Drives xv6 I/O with pexpect
# - Scores per spec: Section I (80) + Section II (20) = 100
#
# Usage:
#   python3 grade_proj1.py --timeout 120
#
import argparse, os, sys, time, re, json, pathlib
import pexpect

PROMPT_ORIG = r"\$ "       # original shell prompt
PROMPT_XVSH = r"xvsh> "    # xvsh prompt

def spawn_qemu(timeout):
    # If your Makefile uses a different target, you can change it to "make qemu" or set QEMUEXTRA in the environment
    return pexpect.spawn("/bin/bash", ["-lc", "make qemu-nox"], encoding="utf-8", timeout=timeout)

def expect_re(child, pattern, timeout, where=""):
    child.expect(pattern, timeout=timeout)

def sendline(child, s):
    child.sendline(s)

def now():
    return time.time()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--timeout", type=int, default=120, help="overall and per-step default timeout (s)")
    args = ap.parse_args()

    total = 0
    score = 0
    results = []
    def add(name, pts, passed, reason=""):
        nonlocal total, score, results
        total += pts
        if passed: score += pts
        results.append({"name": name, "points": pts if passed else 0, "max_points": pts, "passed": passed, "reason": reason})

    pathlib.Path("grade-report").mkdir(exist_ok=True)
    child = spawn_qemu(args.timeout)
    child.logfile = open("grade-report/qemu.log", "w", buffering=1)

    try:
        # Wait for the original shell prompt
        expect_re(child, PROMPT_ORIG, min(args.timeout, 60), "original prompt")

        # Enter xvsh
        sendline(child, "xvsh")
        ok_enter = True
        try:
            expect_re(child, PROMPT_XVSH, 10, "xvsh prompt")
        except Exception as e:
            ok_enter = False
        add("Enter xvsh and show xvsh> prompt", 5, ok_enter, "" if ok_enter else "no xvsh> prompt")
        if not ok_enter:
            with open("grade-report/summary.txt","w") as f:
                f.write("FAIL: xvsh did not start. Score = 0/100\n")
            print("==== Project-1 Score ====")
            print("[FAIL] Enter xvsh and show xvsh> prompt: 0/5")
            print("TOTAL: 0/100")
            sys.exit(1)

        # Basic command works (echo)
        ok_basic = False
        if ok_enter:
            sendline(child, "echo hello")
            try:
                child.expect(r"hello\r?\n", timeout=5)
                # Return to the prompt
                expect_re(child, PROMPT_XVSH, 5, "xvsh prompt after echo")
                ok_basic = True
            except Exception as e:
                ok_basic = False
        add("Basic command works (echo)", 10, ok_basic)

        # Background &: Use sleep-echo Hello &
        ok_bg = False
        if ok_enter:
            sendline(child, "sleep-echo Hello &")
            try:
                # 1) Background process message
                child.expect(r"\[pid\s+\d+\]\s+runs as a background process", timeout=4)
                # 2) Immediately return to the prompt (non-blocking)
                expect_re(child, PROMPT_XVSH, 4, "xvsh prompt immediate")
                # 3) Print Hello after a few seconds — allow trailing spaces, only \r, or no newline
                child.expect(r"Hello(?:[ \t]*\r?\n|[ \t]*\r|$)", timeout=10)
                # 4) Try to wait for the prompt again (some implementations may not immediately show it; not a failure condition)
                try:
                    expect_re(child, PROMPT_XVSH, 3, "prompt after background finished")
                except Exception:
                    pass
                ok_bg = True
            except Exception:
                ok_bg = False
        add("Background (&) runs and later prints Hello", 20, ok_bg)

        # Empty command line: pressing Enter should immediately show the prompt again
        ok_empty = False
        if ok_enter:
            sendline(child, "")
            try:
                expect_re(child, PROMPT_XVSH, 3, "empty line prompt")
                ok_empty = True
            except Exception:
                ok_empty = False
        add("Empty command line shows new prompt", 5, ok_empty)

        # Error command message: Cannot run this command a-wrong-cmd
        ok_bad = False
        if ok_enter:
            sendline(child, "a-wrong-cmd arg1 arg2")
            try:
                child.expect(r"Cannot run this command a-wrong-cmd\r?\n", timeout=5)
                expect_re(child, PROMPT_XVSH, 5, "prompt after bad cmd")
                ok_bad = True
            except Exception:
                ok_bad = False
        add("Wrong command message", 10, ok_bad)

        # Pipe: ls | wc should output three numbers
        ok_pipe = False
        if ok_enter:
            sendline(child, "ls | wc")
            try:
                # Match three numbers, allowing spaces or tabs in between
                child.expect(r"\d+\s+\d+\s+\d+", timeout=5)
                expect_re(child, PROMPT_XVSH, 5, "prompt after pipe")
                ok_pipe = True
            except Exception:
                ok_pipe = False
        add("Pipe (ls | wc) works (3 numbers)", 10, ok_pipe)

        # Redirection: ls > file.txt then ls to confirm the file exists
        ok_redir = False
        if ok_enter:
            sendline(child, "rm -f file.txt")   # Ensure clean state
            sendline(child, "ls > file.txt")
            # Run ls again to confirm file.txt appears
            sendline(child, "ls")
            try:
                child.expect(r"file\.txt", timeout=5)
                expect_re(child, PROMPT_XVSH, 5, "prompt after redir")
                ok_redir = True
            except Exception:
                ok_redir = False
        add("Redirection (ls > file.txt creates file)", 5, ok_redir)

        # Background &: Use sleep-echo Hello &
        ok_bg = False
        if ok_enter:
            sendline(child, "sleep-echo Hello &")
            try:
                # 1) Background process message must appear
                child.expect(r"\[pid\s+\d+\]\s+runs as a background process", timeout=8)

                # 2) Should immediately return to the prompt (non-blocking)
                expect_re(child, PROMPT_XVSH, 4, "xvsh prompt immediate")

                # 3) Print Hello after a few seconds:
                #    - Allow 'xvsh> ' prefix (if background finishes right after the prompt)
                #    - Allow trailing spaces
                #    - Allow only '\r', or '\r?\n', or no line ending at all
                hello_re = r"(?:\r?\n|\r)?(?:xvsh>\s*)?Hello[ \t]*(?:\r?\n|\r|$)"
                child.expect(hello_re, timeout=20)

                # 4) Some implementations may not immediately show the prompt again, not strictly required here
                try:
                    expect_re(child, PROMPT_XVSH, 3, "prompt after background finished")
                except Exception:
                    pass

                ok_bg = True
            except Exception:
                # For debugging, save the tail of the output
                try:
                    with open("grade-report/bg_debug_tail.txt", "w") as dbg:
                        dbg.write(child.before[-800:])
                except Exception:
                    pass
                ok_bg = False

        add("Background (&) runs and later prints Hello", 20, ok_bg)        
        

        # Section II: uprog shut (20) — needs to run under the original $ prompt
        ok_shutdown = False
        # Optionally re-enter xvsh from the original $, or directly run uprog shut under $
        try:
            sendline(child, "uprog_shut")
            # Expect QEMU to exit quickly (EOF)
            child.expect(pexpect.EOF, timeout=10)
            ok_shutdown = True
        except Exception:
            ok_shutdown = False
        add("uprog_shut powers off QEMU (EOF observed)", 20, ok_shutdown)

    finally:
        try:
            child.terminate(force=True)
        except Exception:
            pass

    # Output and archive
    with open("grade-report/summary.txt", "w") as f:
        for r in results:
            f.write(f"{r['name']}: {r['points']}/{r['max_points']} ({'OK' if r['passed'] else 'FAIL'})\n")
        f.write(f"\nTOTAL: {score}/{total}\n")

    print("\n==== Project-1 Score ====")
    for r in results:
        print(f"[{'OK' if r['passed'] else 'FAIL'}] {r['name']}: {r['points']}/{r['max_points']}")
    # Cap the total score
    final_score = score
    if final_score > 100:
        final_score = 100
    
    with open("grade-report/summary.txt","w") as f:
        for r in results:
            f.write(f"{r['name']}: {r['points']}/{r['max_points']} ({'OK' if r['passed'] else 'FAIL'})\n")
        f.write(f"\nTOTAL: {final_score}/100\n")
    
    print("\n==== Project-1 Score ====")
    for r in results:
        print(f"[{'OK' if r['passed'] else 'FAIL'}] {r['name']}: {r['points']}/{r['max_points']}")
    print(f"TOTAL: {final_score}/100")
    
    # exit code = 0 only if full score
    sys.exit(0 if final_score == 100 else 1)

if __name__ == "__main__":
    main()
