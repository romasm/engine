-- Tests for Vector3, Quaternion, and CMath.
-- Run via: curl -X POST http://127.0.0.1:7777/exec --data-binary @tests/test_math.lua
-- Verified against engine build: Development (core_dev.exe)

dofile("tests/framework.lua")
suite("math")

-- ── Vector3 construction ─────────────────────────────────────────────────────
local v = Vector3(1, 2, 3)
expect_eq("v3_x", v.x, 1)
expect_eq("v3_y", v.y, 2)
expect_eq("v3_z", v.z, 3)

-- ── Vector3 zero ─────────────────────────────────────────────────────────────
local zero = Vector3(0, 0, 0)
expect_eq("v3_zero_x", zero.x, 0)

-- ── Vector3 add ──────────────────────────────────────────────────────────────
local sum = Vector3(1, 0, 0) + Vector3(0, 1, 0)
expect_eq("v3_add_x", sum.x, 1)
expect_eq("v3_add_y", sum.y, 1)
expect_eq("v3_add_z", sum.z, 0)

-- ── Vector3 subtract ─────────────────────────────────────────────────────────
local diff = Vector3(3, 5, 7) - Vector3(1, 2, 3)
expect_eq("v3_sub_x", diff.x, 2)
expect_eq("v3_sub_y", diff.y, 3)
expect_eq("v3_sub_z", diff.z, 4)

-- ── Vector3 dot product ───────────────────────────────────────────────────────
local dot = Vector3(1, 0, 0):Dot(Vector3(0, 1, 0))
expect_eq("v3_dot_orthogonal", dot, 0)

local dot2 = Vector3(1, 0, 0):Dot(Vector3(1, 0, 0))
expect_eq("v3_dot_parallel", dot2, 1)

-- ── Vector3 cross product ─────────────────────────────────────────────────────
local cross = Vector3(1, 0, 0):Cross(Vector3(0, 1, 0))
expect_eq("v3_cross_z", cross.z, 1)
expect_eq("v3_cross_x", cross.x, 0)
expect_eq("v3_cross_y", cross.y, 0)

-- ── Vector3 length ────────────────────────────────────────────────────────────
local len = Vector3(3, 4, 0):Length()
expect_eq("v3_length_3_4_0", len, 5)

local len2 = Vector3(0, 0, 0):Length()
expect_eq("v3_length_zero", len2, 0)

-- ── Quaternion construction ───────────────────────────────────────────────────
local q = Quaternion(0, 0, 0, 1)
expect_eq("quat_x", q.x, 0)
expect_eq("quat_y", q.y, 0)
expect_eq("quat_z", q.z, 0)
expect_eq("quat_w", q.w, 1)

-- ── Quaternion length of identity ────────────────────────────────────────────
local qlen = Quaternion(0, 0, 0, 1):Length()
expect_near("quat_identity_length", qlen, 1.0)

-- ── CMath.IsNearlyEq ─────────────────────────────────────────────────────────
expect_true("cmath_nearly_eq_same",    CMath.IsNearlyEq(1.0, 1.0, 0.001))
expect_false("cmath_nearly_eq_diff",   CMath.IsNearlyEq(1.0, 2.0, 0.001))
expect_true("cmath_nearly_eq_within",  CMath.IsNearlyEq(1.0, 1.0005, 0.001))

-- ── CMath.Round ──────────────────────────────────────────────────────────────
expect_eq("cmath_round_up",   CMath.Round(1.6), 2)
expect_eq("cmath_round_down", CMath.Round(1.4), 1)

-- ── CMath.IsEven ─────────────────────────────────────────────────────────────
expect_true("cmath_iseven_4",  CMath.IsEven(4))
expect_false("cmath_iseven_3", CMath.IsEven(3))
expect_true("cmath_iseven_0",  CMath.IsEven(0))

suite_done()
