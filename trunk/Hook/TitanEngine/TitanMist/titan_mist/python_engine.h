#pragma once
#include "engine.h"
#include <Python.h>

namespace mist { namespace engine {

  class python_engine : public basic_engine {
  public:
    python_engine( const std::string& unpacker );

  private:
    bool unpack_impl( const std::string& input, const std::string& output );
  };

} }
