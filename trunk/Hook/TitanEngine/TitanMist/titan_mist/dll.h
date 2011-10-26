#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>
#include <stdexcept>

#include "disallow_copy_and_assign.h"

namespace mist {

  class dll : disallow_copy_and_assign {
  public:
    explicit dll( const char* name ) : handle_( ::LoadLibraryA( name ) ) {
      if ( !handle_ ) {
        std::string err( "failed to load dll: " );
        err += name;
        throw std::runtime_error( err );
      }
    }

    ~dll() {
      if ( handle_ )
        ::FreeLibrary( handle_ );
    }

    ::FARPROC get_function( const char* name, const std::nothrow_t& ) {
      return ::GetProcAddress( handle_, name );
    }

    ::FARPROC get_function( const char* name ) {
      ::FARPROC ptr = this->get_function( name, std::nothrow );
      if ( !ptr ) {
        std::string err( "cannot find function: " );
        err += name;
        throw std::runtime_error( err );
      }
      return ptr;
    }

  protected:
    ::HMODULE handle_;
  };

} // end namespace mist
