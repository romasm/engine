-- Tests that critical engine globals are present and have the expected types.
-- Run via: curl -X POST http://127.0.0.1:7777/exec --data-binary @tests/test_engine_globals.lua
-- Verified against engine build: Development (core_dev.exe)

dofile("tests/framework.lua")
suite("engine_globals")

-- ── Core engine tables ────────────────────────────────────────────────────────
expect_type("global_Main",         Main,         "table")
expect_type("global_WorldMgr",     WorldMgr,     "table")
expect_type("global_SceneMgr",     SceneMgr,     "table")
expect_type("global_Config",       Config,       "table")
expect_type("global_Profiler",     Profiler,     "table")
expect_type("global_FileIO",       FileIO,       "table")

-- ── Math types ────────────────────────────────────────────────────────────────
expect_type("global_Vector3",      Vector3,      "table")
expect_type("global_Vector2",      Vector2,      "table")
expect_type("global_Vector4",      Vector4,      "table")
expect_type("global_Quaternion",   Quaternion,   "table")
expect_type("global_Matrix",       Matrix,       "table")
expect_type("global_CMath",        CMath,        "table")

-- ── ECS system tables ────────────────────────────────────────────────────────
expect_type("global_TransformSystem",   TransformSystem,   "table")
expect_type("global_StaticMeshSystem",  StaticMeshSystem,  "table")
expect_type("global_LightSystem",       LightSystem,       "table")
expect_type("global_PhysicsSystem",     PhysicsSystem,     "table")
expect_type("global_SkeletonSystem",    SkeletonSystem,    "table")
expect_type("global_CameraSystem",      CameraSystem,      "table")

-- ── Engine functions ──────────────────────────────────────────────────────────
expect_type("global_Get_time",     Get_time,     "function")
expect_type("global_Get_dt",       Get_dt,       "function")
expect_type("global_GetWorldMgr",  GetWorldMgr,  "function")

-- ── GUI types ─────────────────────────────────────────────────────────────────
expect_type("global_Gui",          Gui,          "table")
expect_type("global_GuiRoot",      GuiRoot,      "table")

-- ── Rendering types ──────────────────────────────────────────────────────────
expect_type("global_Material",     Material,     "table")
expect_type("global_ScenePipeline",ScenePipeline,"table")

-- ── Scripting helpers ─────────────────────────────────────────────────────────
expect_type("global_StringList",   StringList,   "table")
expect_type("global_class",        class,        "function")
expect_type("global_deep_copy",    deep_copy,    "function")

suite_done()
