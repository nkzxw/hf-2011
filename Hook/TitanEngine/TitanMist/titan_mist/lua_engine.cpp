#include "lua_engine.h"

namespace mist { namespace engine {

  lua_engine::lua_engine( const std::string& unpacker ) {
      unpacker_ = "unpackers\\lua\\";
      unpacker_ += unpacker;
  }

  bool lua_engine::unpack_impl(
    const std::string& input, const std::string& output ) {

    lua_State* L = lua_open();
    luaL_openlibs( L );
    if ( luaL_loadfile( L, unpacker_.c_str() ) || lua_pcall( L, 0, 0, 0 ) )
      throw std::runtime_error( lua_tostring( L, -1 ) );

    lua_getglobal( L, "unpack" );
    if ( !lua_isfunction( L, -1 ) ) {
      lua_pop( L, 1 );
      throw std::runtime_error( "failed to find 'unpack' function" );
    }

    lua_pushstring( L, input.c_str() );
    lua_pushstring( L, output.c_str() );
    lua_pushnil( L );

    if ( lua_pcall( L, 3, 1, 0 ) != 0 )
      throw std::runtime_error( lua_tostring( L, -1 ) );

    if ( !lua_isboolean( L, -1 ) )
      throw std::runtime_error( "invalid result type, expected boolean" );

    int res = lua_toboolean( L, -1 );

    lua_gc( L, LUA_GCCOLLECT, 0 );
    lua_close( L );

    return res ? true : false;
  }

} } // end namespace engine, mist
