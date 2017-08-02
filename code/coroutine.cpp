//
// COROUTINE
//

static int
Pico_coresume(lua_State *L)
{
    lua_State *co = lua_tothread(L, 1);
    
    if (!co)
    {
        lua_pushboolean(L, false); // first argument not a coroutine
        return 1;
    }
    
    if (lua_status(co) == LUA_OK && lua_gettop(co) == 0)
    {
        lua_pushboolean(L, false); // dead
        return 1;
    }
    
    int status = lua_resume(co, L, 0);
    if (status == LUA_OK || status == LUA_YIELD)
    {
        lua_pushboolean(L, true);
        return 1;
    }
    else
    {
        lua_pushboolean(L, false);
        return 1;
    }
}

static int
Pico_cocreate(lua_State *L)
{
    lua_State *NL;
    luaL_checktype(L, 1, LUA_TFUNCTION);
    NL = lua_newthread(L);
    lua_pushvalue(L, 1);
    lua_xmove(L, NL, 1);
    return 1;
}

static int
Pico_yield(lua_State *L)
{
    return lua_yield(L, 0);
}

static int
Pico_costatus(lua_State *L)
{
    lua_State *co = lua_tothread(L, 1);

    if (!co)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    
    if (L == co)
    {
        lua_pushboolean(L, true); // running
        return 1;
    }

    switch (lua_status(co))
    {
        case LUA_YIELD:
        {
            lua_pushboolean(L, true); // suspended
        } break;

        case LUA_OK:
        {
            lua_Debug ar;
            if (lua_getstack(co, 0, &ar) > 0)
                lua_pushboolean(L, true); // running
            else if (lua_gettop(co) == 0)
                lua_pushboolean(L, false); // dead
            else
                lua_pushboolean(L, true); // suspended
        } break;
        
        default:
        {
            lua_pushboolean(L, false); // dead
        } break;
    }
    
    return 1;
}
