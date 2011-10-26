#pragma once
#include "engine.h"
extern "C" {
#include "third_party/lua5.1/include/lua.h"
#include "third_party/lua5.1/include/lualib.h"
#include "third_party/lua5.1/include/lauxlib.h"
};

#if defined( _DEBUG )
#  pragma comment( lib, "third_party/lua5.1/lib/static/lua5.1_d.lib" )
#else
#  pragma comment( lib, "third_party/lua5.1/lib/lua5.1.lib" )
#endif

namespace mist { namespace engine {

  class lua_engine : public basic_engine {
  public:
    lua_engine( const std::string& unpacker );

  private:
    bool unpack_impl( const std::string& input, const std::string& output );
  };

} } // end namespace engine, mist
