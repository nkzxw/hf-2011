#pragma once
#include <string>
#include <exception>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace mist { namespace filesystem {

  class dir {
  public:
    dir( const std::string& path )
      : path_( path ), handle_( INVALID_HANDLE_VALUE ) {
        path_ += "\\*";
        init();
    }

    ~dir() {
      if ( handle_ != INVALID_HANDLE_VALUE )
        ::FindClose( handle_ );
    }

    bool next_entry() {
      if ( handle_ == INVALID_HANDLE_VALUE )
        throw std::runtime_error( "bad dir handle" );
      if ( have_entry_ ) {
        have_entry_ = false;
      } else {
        if ( !::FindNextFileA( handle_, &find_data_ ) ) {
          FindClose( handle_ );
          handle_ = INVALID_HANDLE_VALUE;
          return false;
        }
      }
      entry_name_ = find_data_.cFileName;
      return true;
    }

    std::string name() const {
      return entry_name_;
    }

    bool is_directory() const {
      return ( find_data_.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY );
    }

  private:
    ::WIN32_FIND_DATAA find_data_;
    ::HANDLE handle_;
    std::string path_;
    std::string entry_name_;
    bool have_entry_;

    void init() {
      handle_ = ::FindFirstFileA( path_.c_str(), &find_data_ );
      if ( handle_ == INVALID_HANDLE_VALUE )
        throw std::runtime_error( "dir failure" );
      have_entry_ = true;
    }
  };

} } // end namespace filesystem, mist
