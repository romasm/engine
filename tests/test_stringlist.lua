-- Tests for the StringList C++ userdata class exposed to Lua.
-- Run via: curl -X POST http://127.0.0.1:7777/exec --data-binary @tests/test_stringlist.lua
-- Verified against engine build: Development (core_dev.exe)

dofile("tests/framework.lua")
suite("stringlist")

-- ── Construction ─────────────────────────────────────────────────────────────
local list = StringList()
expect_type("sl_constructed", list, "userdata")

-- ── Empty size ────────────────────────────────────────────────────────────────
expect_eq("sl_empty_size", list:Size(), 0)

-- ── Add one element ───────────────────────────────────────────────────────────
list:Add("hello")
expect_eq("sl_size_after_one", list:Size(), 1)
expect_eq("sl_get_first", list:Get(0), "hello")

-- ── Add second element ────────────────────────────────────────────────────────
list:Add("world")
expect_eq("sl_size_after_two", list:Size(), 2)
expect_eq("sl_get_second", list:Get(1), "world")

-- ── First element unchanged after second add ──────────────────────────────────
expect_eq("sl_first_unchanged", list:Get(0), "hello")

-- ── Second independent list doesn't share state ───────────────────────────────
local list2 = StringList()
list2:Add("independent")
expect_eq("sl_list2_size", list2:Size(), 1)
expect_eq("sl_list1_size_unchanged", list:Size(), 2)

suite_done()
