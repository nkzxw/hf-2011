#pragma once
#include "types.h"
#include <ios>
#include <memory>

#define MIST_IO_USE_MAPPED_FILE

namespace mist { namespace io {

  namespace detail {

    class file_impl;

  }

  class file_source {
    typedef detail::file_impl impl_type;

  public:
    file_source() {}

    explicit file_source(
      const std::string& filename, std::ios_base::openmode mode=std::ios::in ) {
      this->open( filename, mode );
    }

    void open(
      const std::string& filename, std::ios_base::openmode mode=std::ios::in );

    bool is_open() const {
      return pimpl_.get() != 0;
    }

    int64_t size() const;

    std::streamsize read( char* s, std::streamsize n );

    std::streampos seek( std::streamoff off, std::ios_base::seekdir way );

    void close() {
      pimpl_.reset();
    }

  private:
    std::tr1::shared_ptr<impl_type> pimpl_;
  };

  class file_sink {
    typedef detail::file_impl impl_type;

  public:
    file_sink() {}

    explicit file_sink(
      const std::string& filename,
      std::ios_base::openmode mode=std::ios::out ) {
      this->open( filename, mode );
    }

    void open(
      const std::string& filename, std::ios_base::openmode mode=std::ios::out );

    bool is_open() const {
      return pimpl_.get() != 0;
    }

    std::streamsize write( const char* s, std::streamsize n );

    std::streampos seek( std::streamoff off, std::ios_base::seekdir way );

    void close() {
      pimpl_.reset();
    }

  private:
    std::tr1::shared_ptr<impl_type> pimpl_;
  };

  class file {
    typedef detail::file_impl impl_type;

  public:
    file() {}

    explicit file(
      const std::string& filename,
      std::ios_base::openmode mode=std::ios::in|std::ios::out ) {
      this->open( filename, mode );
    }

    void open(
      const std::string& filename,
      std::ios_base::openmode mode=std::ios::in|std::ios::out );

    bool is_open() const {
      return pimpl_.get() != 0;
    }

    std::streamsize read( char* s, std::streamsize n );
    std::streamsize write( const char* s, std::streamsize n );

    std::streampos seek( std::streamoff off, std::ios_base::seekdir way );

    void close() {
      pimpl_.reset();
    }

  private:
    std::tr1::shared_ptr<impl_type> pimpl_;
  };

} } // end namespace io, mist
