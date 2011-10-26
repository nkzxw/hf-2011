#pragma once
#include "engine.h"
#include "titan_script/TitanScript.h"

namespace mist { namespace engine {

  class titanscript_engine : public basic_engine {
  public:
    titanscript_engine( const std::string& unpacker );

  private:
    bool unpack_impl( const std::string& input, const std::string& output );

    static void __stdcall log_callback( const char* str, eLogType log_type );
  };

} } // end namespace engine, mist
