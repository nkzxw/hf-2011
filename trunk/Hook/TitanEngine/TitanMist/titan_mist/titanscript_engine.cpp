#include "titan_engine/SDK.h"
#include "titanscript_engine.h"
#include <iostream>

namespace mist { namespace engine {

  titanscript_engine::titanscript_engine( const std::string& unpacker ) {
    unpacker_ = "unpackers\\titanscript\\";
    unpacker_ += unpacker;
  }

  bool titanscript_engine::unpack_impl(
    const std::string& input, const std::string& output ) {

    if ( !ExtensionManagerIsPluginLoaded( "TitanScript" ) ||
      !ExtensionManagerIsPluginEnabled( "TitanScript" ) )
      throw std::runtime_error( "TitanScript failed to load!" );

    tScripterLoadFileA load_file = GetTSFunctionPointer( LoadFileA );
    tScripterExecuteWithTitanMistA exec =
      GetTSFunctionPointer( ExecuteWithTitanMistA );
    tScripterSetLogCallback set_log_callback =
      GetTSFunctionPointer( SetLogCallback );

    set_log_callback( &log_callback );
    load_file( unpacker_.c_str() );
    return exec( input.c_str(), output.c_str() );
  }

  void titanscript_engine::log_callback( const char* str, eLogType log_type ) {
      std::cout << str << "\n" << std::flush;
  }

} } // end namespace engine, mist
