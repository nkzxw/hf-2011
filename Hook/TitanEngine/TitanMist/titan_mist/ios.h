#pragma once

#include <ios>
#include <string>
#include <exception>

namespace mist { namespace io { namespace detail {

    //class failure : std::exception {
    //public:
    //  explicit failure( const std::string& what ) : what_( what ) {}
    //  const char* what() const {
    //    return what_.c_str();
    //  }
    //private:
    //  std::string what_;
    //};

#define MIST_IO_FAILURE std::runtime_error

} } } // end namespaces detail, io, mist