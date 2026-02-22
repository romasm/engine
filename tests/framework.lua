-- Test framework: included at the top of every test file via dofile or copy-paste.
-- Each test prints [PASS] test_name or [FAIL] test_name: reason.
-- At the end the suite prints SUITE_RESULT: passed=N failed=M suite=<name>.

local _passed = 0
local _failed = 0
local _suite  = "unknown"

local function fail(name, reason)
    _failed = _failed + 1
    print("[FAIL] " .. name .. ": " .. tostring(reason))
end

local function pass(name)
    _passed = _passed + 1
    print("[PASS] " .. name)
end

function suite(name)
    _suite = name
end

function expect_eq(name, actual, expected)
    if actual == expected then
        pass(name)
    else
        fail(name, "expected " .. tostring(expected) .. " got " .. tostring(actual))
    end
end

function expect_true(name, value)
    if value then
        pass(name)
    else
        fail(name, "expected true, got " .. tostring(value))
    end
end

function expect_false(name, value)
    if not value then
        pass(name)
    else
        fail(name, "expected false, got " .. tostring(value))
    end
end

function expect_near(name, actual, expected, tolerance)
    tolerance = tolerance or 0.0001
    if math.abs(actual - expected) <= tolerance then
        pass(name)
    else
        fail(name, "expected ~" .. tostring(expected) .. " got " .. tostring(actual))
    end
end

function expect_type(name, value, expected_type)
    local actual_type = type(value)
    if actual_type == expected_type then
        pass(name)
    else
        fail(name, "expected type " .. expected_type .. " got " .. actual_type)
    end
end

function expect_gt(name, actual, threshold)
    if actual > threshold then
        pass(name)
    else
        fail(name, "expected > " .. tostring(threshold) .. " got " .. tostring(actual))
    end
end

function expect_gte(name, actual, threshold)
    if actual >= threshold then
        pass(name)
    else
        fail(name, "expected >= " .. tostring(threshold) .. " got " .. tostring(actual))
    end
end

function suite_done()
    print("SUITE_RESULT: passed=" .. _passed .. " failed=" .. _failed .. " suite=" .. _suite)
end
