#pragma once
#include "engine.h"

namespace mist { namespace engine {

  class native_engine : public basic_engine {
    typedef bool
      ( *mist_unpack )( const char* input, const char* output, void* reserved );

  public:
    native_engine( const std::string& unpacker );

  private:
    bool unpack_impl( const std::string& input, const std::string& output );

  protected:
    mist_unpack unpack_func_;
  };

} } // end namespace engine, mist
