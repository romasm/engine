-- Tests for engine time functions: Get_time() and Get_dt().
-- Run via: curl -X POST http://127.0.0.1:7777/exec --data-binary @tests/test_time.lua
-- Verified against engine build: Development (core_dev.exe)

dofile("tests/framework.lua")
suite("time")

-- ── Get_time returns a number ─────────────────────────────────────────────────
local time = Get_time()
expect_type("get_time_type", time, "number")

-- ── Get_time is positive (engine has been running) ────────────────────────────
expect_gt("get_time_positive", time, 0)

-- ── Get_dt returns a number ───────────────────────────────────────────────────
local dt = Get_dt()
expect_type("get_dt_type", dt, "number")

-- ── Get_dt is positive (called mid-frame, should be > 0) ─────────────────────
expect_gt("get_dt_positive", dt, 0)

-- ── Two consecutive Get_time calls are monotonically non-decreasing ───────────
local t1 = Get_time()
local t2 = Get_time()
expect_gte("get_time_monotonic", t2, t1)

-- ── Get_dt is a reasonable frame time (ms; 60fps cap → ~16.7ms; allow up to 1000ms) ──
expect_true("get_dt_reasonable", dt > 0 and dt < 1000.0)

suite_done()
