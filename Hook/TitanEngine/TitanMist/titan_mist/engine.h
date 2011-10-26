#pragma once
#include "disallow_copy_and_assign.h"
#include <string>

namespace mist { namespace engine {

  class basic_engine : disallow_copy_and_assign {
  public:
    virtual ~basic_engine() {}

    bool unpack( const std::string& input, const std::string& output ) {
      return this->unpack_impl( input, output );
    }

  private:
    virtual bool
      unpack_impl( const std::string& input, const std::string& output ) = 0;

  protected:
    std::string unpacker_;
  };

} } // end namespace engine, mist
