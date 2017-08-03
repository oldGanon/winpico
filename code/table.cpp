const char* ForeachFunction =
R"foreach_function(
function foreach(t,f) 
  for v in all(t) do
    f(v)
  end
end
)foreach_function";

static int
Pico_allIterator(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_rawget(L, 1);
    if (!lua_compare(L, 2, 3, LUA_OPEQ))
    {
        return 1;
    }

    lua_pop(L, 2);
    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_next(L, 1))
    {
        lua_insert(L, 2);
        lua_replace(L, lua_upvalueindex(1));
        return 1;
    }
    
    return 0;
}

static int
Pico_all(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
        return 0;

    if (lua_type(L, 1) != LUA_TTABLE)
        return 0;

    lua_settop(L, 1);

    lua_pushnil(L);
    lua_pushcclosure(L, Pico_allIterator, 1);
    lua_insert(L, 1);
    lua_pushnil(L);

    return 3;
}

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
    for (i32 pos = 1; pos <= size; ++pos)
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
