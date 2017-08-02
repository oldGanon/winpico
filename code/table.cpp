const char* AllFunction =
R"all_function(
function all(t) 
  local i = nil
  local function all_it(t,ignore)
    i,v = next(t,i)
    return v
  end
  return all_it, t, nil
end
)all_function";

const char* ForeachFunction =
R"foreach_function(
function foreach(t,f) 
  for v in all(t) do
    f(v)
  end
end
)foreach_function";

static int
Pico_add(lua_State *L)
{    
    i32 args = lua_gettop(L);
    if (args < 2)
        return 0;

    if (lua_type(L, 1) != LUA_TTABLE)
        return 0;

    i32 pos = luaL_len(L, 1) + 1;
    lua_rawseti(L, 1, pos);
    return 0;
}

static int
Pico_del(lua_State *L) {
    i32 args = lua_gettop(L);
    if (args < 2)
        return 0;

    if (lua_type(L, 1) != LUA_TTABLE)
        return 0;
    
    i32 size = luaL_len(L, 1);
    i32 pos = 1;
    for ( ; pos <= size; ++pos)
    {
        lua_pushinteger(L, pos);
        i64 top = lua_tointeger(L, 3);
        lua_gettable(L, 1);
        if (lua_compare(L, 2, 3, LUA_OPEQ))
        {
            lua_rawgeti(L, 1, pos);
            for ( ; pos < size; ++pos)
            {
                lua_rawgeti(L, 1, pos + 1);
                lua_rawseti(L, 1, pos);
            }
            lua_pushnil(L);
            lua_rawseti(L, 1, pos);
            return 0;
        }
        lua_pop(L, 1);
    }
    return 0;    
}
