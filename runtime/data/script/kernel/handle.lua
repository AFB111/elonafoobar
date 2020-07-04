--- Lua-side storage for safe references to C++ objects.
--
-- Whenever a new character or item is initialized, a corresponding
-- handle will be created here to track the object's lifetime in an
-- isolated Lua environment managed by a C++ handle manager. If the
-- object is no longer valid for use (a character died, or an item was
-- destroyed), the C++ side will set the Lua side's handle to be
-- invalid. An error will be thrown on trying to access or write to
-- anything on an invalid handle. Since objects are identified by
-- UUIDs, it is possible to serialize references to C++ objects
-- relatively easily, allowing for serializing the state of any mods
-- that are in use along with the base save data. The usage of UUIDs
-- also allows checking equality and validity of objects, even long
-- after the C++ object the handle references has been removed.
--
-- Borrowed from https://eliasdaler.github.io/game-object-references/

local Handle = {}

-- Stores a map of handle.__uuid -> C++ object reference. These should not be
-- directly accessable outside this chunk.
-- Indexed by [class_name][uuid].
local refs = {}

-- Stores a map of integer ID -> handle.
-- Indexed by [class_name][id].
-- NOTE: If integer indices as IDs are completely removed, then this
-- may have to be indexed by another value.
local handles_by_index = {}

-- Cache for function closures resolved when indexing a handle.
-- Creating a new closure for every method lookup is expensive.
-- Indexed by [class_name][method_name].
local memoized_funcs = {}


local function handle_error(handle, key)
   if _IS_TEST then
      return
   end

   if ELONA and ELONA.require then
      local GUI = ELONA.require("core.GUI")
      GUI.txt_color(3)
      GUI.txt("Error: handle is not valid! ")
      if key ~= nil then
         GUI.txt("Indexing: " .. tostring(key) .. " ")
      end
      GUI.txt("This means the character/item got removed. ")
      GUI.txt_color(0)
   end

   if handle then
      print("Error: handle is not valid! " .. handle.__kind .. ":" .. handle.__uuid)
   else
      print("Error: handle is not valid! " .. tostring(handle))
   end
   print(debug.traceback())

   error("Error: handle is not valid!", 2)
end


function Handle.is_valid(handle)
   return handle ~= nil and refs[handle.__kind][handle.__uuid] ~= nil
end


--- Create a metatable to be set on all handle tables of a given kind
--- ("LuaCharacter", "LuaItem") which will check for validity on
--- variable/method access.
local function generate_metatable(kind)
   -- The userdata table is bound by sol2 as a global.
   local userdata_table = _ENV[kind]

   local mt = {}
   memoized_funcs[kind] = {}
   mt.__index = function(handle, key)
      if key == "is_valid" then
         -- workaround to avoid serializing is_valid function, since
         -- serpent will refuse to load it safely
         return Handle.is_valid
      end

      if not Handle.is_valid(handle)then
         handle_error(handle, key)
      end

      -- Try to get a property out of the C++ reference.
      local val = refs[kind][handle.__uuid][key]

      if val ~= nil then
         -- If the found property is a plain value, return it.
         if type(val) ~= "function" then
            return val
         end
      end

      -- If that fails, try calling a function by the name given.
      local f = memoized_funcs[kind][key]
      if not f then
         -- Look up the function on the usertype table generated by
         -- sol2.
         f = function(h, ...)
            if type(h) ~= "table" or not h.__handle then
               error("Please call this function using colon syntax (':').")
            end
            return userdata_table[key](refs[kind][h.__uuid], ...)
         end

         -- Cache it so we don't incur the overhead of creating a
         -- closure on every lookup.
         memoized_funcs[kind][key] = f
      end
      return f
   end
   mt.__newindex = function(handle, key, value)
      if not Handle.is_valid(handle) then
         handle_error(handle, key)
      end

      refs[kind][handle.__uuid][key] = value
   end
   mt.__eq = function(lhs, rhs)
      return lhs.__kind == rhs.__kind and lhs.__uuid == rhs.__uuid
   end

   refs[kind] = {}
   handles_by_index[kind] = {}

   return mt
end

local metatables = {}
metatables.LuaCharacter = generate_metatable("LuaCharacter")
metatables.LuaItem = generate_metatable("LuaItem")

--- Given a valid handle and kind, retrieves the underlying C++
--- userdata reference.
function Handle.get_ref(handle, kind)
   if not Handle.is_valid(handle) then
      handle_error(handle)
      return nil
   end

   if not handle.__kind == kind then
      print(debug.traceback())
      error("Error: handle is of wrong type: wanted " .. kind .. ", got " .. handle.__kind)
      return nil
   end

   return refs[kind][handle.__uuid]
end

function Handle.set_ref(handle, ref)
   refs[handle.__kind][handle.__uuid] = ref
end

--- Gets a metatable for the lua type specified ("LuaItem",
--- "LuaCharacter")
function Handle.get_metatable(kind)
   return metatables[kind]
end

--- Given an integer index of a C++ object and kind, retrieves the
--- handle that references it.
function Handle.get_handle(index, kind)
   local handle = handles_by_index[kind][index]

   if handle and handle.__kind ~= kind then
      print(debug.traceback())
      error("Error: handle is of wrong type: wanted " .. kind .. ", got " .. handle.__kind)
      return nil
   end

   return handle
end

--- Creates a new handle by using a C++ reference's integer index. The
--- handle's index must not be occupied by another handle, to prevent
--- overwrites.
function Handle.create_handle(cpp_ref, kind, uuid)
   if handles_by_index[kind][cpp_ref.index] ~= nil then
      print(handles_by_index[kind][cpp_ref.index].__uuid)
      error("Handle already exists: " .. kind .. ":" .. cpp_ref.index, 2)
      return nil
   end

   -- print("CREATE " .. kind .. " " .. cpp_ref.index .. " " .. uuid)

   local handle = {
      __uuid = uuid,
      __kind = kind,
      __handle = true
   }

   setmetatable(handle, metatables[handle.__kind])
   refs[handle.__kind][handle.__uuid] = cpp_ref
   handles_by_index[handle.__kind][cpp_ref.index] = handle

   return handle
end


--- Removes an existing handle by using a C++ reference's integer
--- index. It is acceptable if the handle doesn't already exist.
function Handle.remove_handle(cpp_ref, kind)
   local handle = handles_by_index[kind][cpp_ref.index]

   if handle == nil then
      return
   end

   -- print("REMOVE " .. cpp_ref.index .. " " .. handle.__uuid)

   assert(handle.__kind == kind)

   refs[handle.__kind][handle.__uuid] = nil
   handles_by_index[handle.__kind][cpp_ref.index] = nil
end


--- Moves a handle from one integer index to another if it exists. If the handle
--- exists, the target slot must not be occupied. If not, the destination slot
--- will be set to empty as well.
function Handle.relocate_handle(cpp_ref, dest_cpp_ref, new_index, kind)
   local handle = handles_by_index[kind][cpp_ref.index]

   if Handle.is_valid(handle) then
      handles_by_index[kind][new_index] = handle
      Handle.set_ref(handle, dest_cpp_ref)
   else
      -- When the handle is not valid, set the destination slot to be
      -- invalid as well, to reflect relocating an empty handle.

      -- This can happen when a temporary character is created as part
      -- of change creature magic, as the temporary's state will be
      -- empty at the time of relocation.
      handles_by_index[kind][new_index] = nil
   end

   -- Clear the slot the handle was moved from.
   handles_by_index[kind][cpp_ref.index] = nil
end

--- Exchanges the positions of two handles and updates their __index
--- fields with the new values. Both handles must be valid.
function Handle.swap_handles(cpp_ref_a, cpp_ref_b, kind)
   -- Do nothing.
end


function Handle.assert_valid(cpp_ref, kind)
   local handle = handles_by_index[kind][cpp_ref.index]

   assert(Handle.is_valid(handle))
end


function Handle.assert_invalid(cpp_ref, kind)
   local handle = handles_by_index[kind][cpp_ref.index]

   assert(not Handle.is_valid(handle))
end


-- Functions for deserialization. The steps are as follows.
-- 1. Deserialize mod data that contains the table of handles.
-- 2. Clear out existing handles/references in "handles_by_index" and
--    "refs" using "clear_handle_range".
-- 3. Place handles into the "handles_by_index" table using
--    "merge_handles".
-- 4. In C++, for each object loaded, add its reference to the "refs"
--    table using by looking up a newly inserted handle using the C++
--    object's integer index in "handles_by_index".
--    (handle_manager::resolve_handle)

function Handle.clear_handle_range(kind, index_start, index_end)
   for index=index_start, index_end-1 do
      local handle = handles_by_index[kind][index]
      if handle ~= nil then
         refs[kind][handle.__uuid] = nil
         handles_by_index[kind][index] = nil
      end
   end
end

function Handle.merge_handles(kind, obj_ids)
   for index, obj_id in pairs(obj_ids) do
      if obj_id ~= nil then
         if handles_by_index[kind][index] ~= nil then
            error("Attempt to overwrite handle " .. kind .. ":" .. index, 2)
         end
         local handle = {
            __uuid = obj_id,
            __kind = kind,
            __handle = true,
         }
         setmetatable(handle, metatables[kind])
         handles_by_index[kind][index] = handle
      end
   end
end


function Handle.clear()
   for kind, _ in pairs(refs) do
      refs[kind] = {}
   end

   for kind, _ in pairs(handles_by_index) do
      handles_by_index[kind] = {}
   end
end


local function iter(a, i)
   if i >= a.to then
      return nil
   end
   local v = a.handles[i]
   -- Skip over indices that point to invalid handles.
   while not (v and Handle.is_valid(v)) do
      i = i + 1
      v = a.handles[i]
      if i >= a.to then
         return nil
      end
   end
   i = i + 1
   return i, v
end

function Handle.iter(kind, from, to)
   if from > to then
      return nil
   end
   return iter, {handles=handles_by_index[kind], to=to}, from
end

-- These functions exist in the separate handle environment because I
-- couldn't quite figure out how to make a valid custom C++/Lua
-- iterator with Sol that returns Lua table references as values.

-- Chara.iter(from, to)
Handle.iter_charas = function(from, to) return Handle.iter("LuaCharacter", from, to) end

-- Item.iter(from, to)
Handle.iter_items = function(from, to) return Handle.iter("LuaItem", from, to) end


return Handle
