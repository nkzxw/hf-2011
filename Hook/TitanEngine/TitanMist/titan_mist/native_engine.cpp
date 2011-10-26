#include "dll.h"
#include "native_engine.h"
#include <memory>

namespace mist { namespace engine {

  native_engine::native_engine( const std::string& unpacker ) {
    unpacker_ = "unpackers\\native\\";
    unpacker_ += unpacker;
  }

  bool native_engine::unpack_impl(
    const std::string& input, const std::string& output ) {

    std::auto_ptr<dll> lib( new dll( unpacker_.c_str() ) );

    unpack_func_ =
      reinterpret_cast<mist_unpack>( lib->get_function( "unpack" ) );

    return (*unpack_func_)( input.c_str(), output.c_str(), 0 );
  }

} } // end namespace engine, mist
