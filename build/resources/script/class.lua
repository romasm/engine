-- recipe taken from http://lua-users.org/wiki/SimpleLuaClasses and
-- mixin code added, jiggled about a little bit.
-- class.lua
-- Compatible with Lua 5.1 (not 5.0).

--deep copy an object BUT NOT ITs METATABLE
function deep_copy(object)
    local lookup_table = {}
    local function _copy(object)
        if type(object) ~= "table" then
            return object
        elseif lookup_table[object] then
            return lookup_table[object]
        end
        local new_table = {}
        lookup_table[object] = new_table
        for index, value in pairs(object) do
            new_table[_copy(index)] = _copy(value)
        end
        return setmetatable(new_table, getmetatable(object))
    end
    return _copy(object)
end

local function _is_instance()
   return true
end

local function _isnot_instance()
   return false
end


-- clear a class object that can be inherited from
-- base is a base class.
function class(base)

   local c = {}    -- a new class instance

   if type(base) == 'table' then
    -- our new class is a shallow copy of the base class!
      for i,v in pairs(base) do
         c[i] = v
      end
      c._base = base
   end

   -- the class will be the metatable for all its objects,
   -- and they will look up their methods in it.
   c.__index = c

   -- expose a constructor which can be called by <classname>(<args>)
   local mt = {}

   -- this is the constructor for the class
   mt.__call = function(class_tbl, ...)

      local obj = {}
      setmetatable(obj,c)
      obj.is_instance = _is_instance
      if class_tbl.init then
         class_tbl.init(obj,...)
      else
         -- make sure that any stuff from the base class is initialized!
         if class_tbl._base and class_tbl._base.init then
            class_tbl._base.init(obj, ...)
         end
      end
      return obj
   end

   -- utility function to see of an object belongs to a class
   c.is_a = function(self, klass)

      local m = getmetatable(self)
      while m do
         if m == klass then return true end
         m = m._base
      end
      return false
   end

   -- mixin is added to the class. klass is a class that supports
   -- the mixin methods:
   --    klass._mixinmethods, a list of methods to copy to self
   --    klass._mixindata a table with name and source keys
   --       name is the name of the data self and source is an
   --       object that will be deep copied in to self so that
   --       self[name] = object
   c.mixin = function(self, klass)

      assert(type(klass) == "table", "mixin class should be a table, is a " .. type(klass))
      assert(klass._mixinmethods, "mixin class needs a _mixinmethods list of methods to mix")

      --copy the methods across to the object
      for _, methodname in pairs(klass._mixinmethods) do
         assert(self[methodname] == nil, "mixin name " .. methodname .. " already used")
         self[methodname] = klass[methodname]
      end

      -- copy the mixin data across
      for k, v in pairs(klass._mixindata) do
         self[k] = deep_copy(v)
      end

   end

   c.is_instance = _isnot_instance

   setmetatable(c, mt)
   return c
end
