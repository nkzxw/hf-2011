#pragma once
#include "file.h"
#include "ios.h"
#include "types.h"
#include "helpers.h"
#include "disallow_copy_and_assign.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace mist { namespace io {
  
  namespace detail {

    static const int64_t MIST_PAGE_SIZE = 1LL << 13;
    static const int64_t MIST_MAP_SIZE = 64LL << 21;

    inline bool test_flag( unsigned long a, unsigned long i ) {
      return ( a & i ) != 0;
    }

    class file_impl : disallow_copy_and_assign {
    public:
      file_impl( const std::string& filename, std::ios_base::openmode mode ) {
        unsigned long access_flags = NULL;
        if ( test_flag( mode, std::ios::in ) )
          access_flags |= GENERIC_READ;
        if ( test_flag( mode, std::ios::out ) )
          access_flags |= GENERIC_WRITE;

        unsigned long create_flags = NULL;
        if ( test_flag( mode, std::ios::app ) )
          create_flags = OPEN_ALWAYS;
        else if ( test_flag( mode, std::ios::in ) &&
          !test_flag( mode, std::ios::trunc ) )
          create_flags = OPEN_EXISTING;
        else
          create_flags = CREATE_ALWAYS;

        file_handle_ = ::CreateFileA( filename.c_str(), access_flags,
          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, create_flags,
          FILE_ATTRIBUTE_NORMAL, NULL );

        if ( file_handle_ == INVALID_HANDLE_VALUE )
          throw MIST_IO_FAILURE( "opening file failed" );

        ::LARGE_INTEGER size_buffer;
        if ( !::GetFileSizeEx( file_handle_, &size_buffer ) ) {
          ::CloseHandle( file_handle_ );
          throw MIST_IO_FAILURE( "GetFileSizeEx failed" );
        }
        file_size_ = static_cast<uint64_t>( size_buffer.QuadPart );

#if defined(MIST_IO_USE_MAPPED_FILE)
        offset_ = 0;
        map_size_ = MIST_MAP_SIZE;
        page_size_ = file_size_;
        int64_t diff = map_size_ % MIST_PAGE_SIZE;
        if ( diff )
          map_size_ += MIST_PAGE_SIZE - diff;
        ::DWORD mprot = PAGE_READONLY;
        ::DWORD vmode = FILE_MAP_READ;
        if ( test_flag( mode, std::ios::out ) ) {
          mprot = PAGE_READWRITE;
          vmode = FILE_MAP_WRITE;
        } else if ( map_size_ > file_size_ ) {
          map_size_ = file_size_;
        }
        size_buffer.QuadPart = map_size_;
        void* map = NULL;
        if ( map_size_ ) {
          map_handle_ = ::CreateFileMappingA( file_handle_, NULL, mprot,
            size_buffer.HighPart, size_buffer.LowPart, NULL );
          if ( !map_handle_ || map_handle_ == INVALID_HANDLE_VALUE ) {
            ::CloseHandle( file_handle_ );
            throw MIST_IO_FAILURE( "CreateFileMappingA failed" );
          }
          map = ::MapViewOfFile( map_handle_, vmode, 0, 0, 0 );
          if ( !map ) {
            ::CloseHandle( map_handle_ );
            ::CloseHandle( file_handle_ );
            throw MIST_IO_FAILURE( "MapViewOfFile failed" );
          }
          if ( page_size_ < map_size_ )
            page_size_ = map_size_;
        }
        map_ = reinterpret_cast<char*>( map );
#endif

        try {
          if ( test_flag( mode, std::ios::app ) )
            this->seek( 0, std::ios::end );
        } catch ( ... ) {
#if defined(MIST_IO_USE_MAPPED_FILE)
          ::UnmapViewOfFile( reinterpret_cast<void*>( map_ ) );
          ::CloseHandle( map_handle_ );
#endif
          ::CloseHandle( file_handle_ );
          throw;
        }
      }

      ~file_impl() {
#if defined(MIST_IO_USE_MAPPED_FILE)
        ::UnmapViewOfFile( reinterpret_cast<void*>( map_ ) );
        ::CloseHandle( map_handle_ );
#endif
        ::CloseHandle( file_handle_ );
      }

      std::streamsize read( char* s, std::streamsize n ) {
        ::DWORD amt;
        ::DWORD buf_size;

        std::streamsize total = 0;
#if defined(MIST_IO_USE_MAPPED_FILE)
        int64_t end = offset_ + n;
        if ( end <= map_size_ ) {
          std::memcpy( s, map_ + offset_, n );
          offset_ += n;
          return n;
        }
        if ( offset_ < map_size_ ) {
          std::size_t left = static_cast<std::size_t>( map_size_ - offset_ );
          std::memcpy( s, map_ + offset_, left );
          offset_ += left;
          s += left;
          n -= left;
          total += left;
        }
#endif
        
        while ( n > 0 ) {
          buf_size = n;
          if ( ::ReadFile( file_handle_, s, buf_size, &amt, 0 ) == FALSE )
            throw MIST_IO_FAILURE( "read failed" );
          if ( !amt )
            break;
          total += amt;
          s += amt;
          n -= amt;
        }

        return total ? total : -1;

      }

      std::streamsize write( const char* s, std::streamsize n ) {
        ::DWORD amt;
        ::DWORD buf_size;

        std::streamsize total = 0;
#if defined(MIST_IO_USE_MAPPED_FILE)
        int64_t end = offset_ + n;
        if ( end <= map_size_ ) {
          std::memcpy( map_ + offset_, s, n );
          return n;
        }
        if ( offset_ < map_size_ ) {
          std::size_t left = static_cast<std::size_t>( map_size_ - offset_ );
          offset_ += left;
          s += left;
          n -= left;
          total += left;
        }
#endif

        while ( n > 0 ) {
          buf_size = n;
          if ( ::WriteFile( file_handle_, s, buf_size, &amt, NULL ) == FALSE )
            throw MIST_IO_FAILURE( "write failed" );
          total += amt;
          s += amt;
          n -= amt;
        }

        return total;
      }

      std::streampos seek( std::streamoff off, std::ios_base::seekdir way ) {
#if defined(MIST_IO_USE_MAPPED_FILE)
        int64_t pos;
        switch ( way ) {
          case std::ios::cur:
            pos = offset_ + off;
            if ( pos <= map_size_ ) {
              offset_ = pos;
              return std::streamoff( offset_ );
            }
            break;
          case std::ios::beg:
            if ( off <= map_size_ ) {
              offset_ = off;
              return std::streamoff( offset_ );
            }
            break;
          case std::ios::end:
            pos = file_size_ + off;
            if ( pos <= map_size_ ) {
              offset_ = pos;
              return std::streamoff( offset_ );
            }
            break;
        }
#endif
        ::LONG low = static_cast<::LONG>( low_qword( off ) );
        ::LONG high = static_cast<::LONG>( high_qword( off ) );
        low = ::SetFilePointer( file_handle_, low, &high,
          way == std::ios::beg ? FILE_BEGIN :
          way == std::ios::cur ? FILE_CURRENT : FILE_END );

        if ( ( low == INVALID_SET_FILE_POINTER ) &&
          ( ::GetLastError() != NO_ERROR ) )
          throw MIST_IO_FAILURE( "seek failed" );

        return static_cast<std::streampos>(
          low | ( static_cast<uint64_t>( static_cast<::LONG>(high) ) << 32 ) );
      }

      int64_t size() const {
        return file_size_;
      }

    private:
      ::HANDLE file_handle_;
      int64_t file_size_;

#if defined(MIST_IO_USE_MAPPED_FILE)
      ::HANDLE map_handle_;
      int64_t map_size_;
      int64_t page_size_;
      int64_t offset_;
      char* map_;
#endif
    };

  } // end namespace detail

  void file_source::open(
    const std::string& filename, std::ios_base::openmode mode ) {
    pimpl_.reset( new impl_type( filename, mode|std::ios::in ) );
  }

  std::streamsize file_source::read( char* s, std::streamsize n ) {
    return pimpl_->read( s, n );
  }

  std::streampos file_source::seek(
    std::streamoff off, std::ios_base::seekdir way ) {
    return pimpl_->seek( off, way );
  }

  int64_t file_source::size() const {
    return pimpl_->size();
  }

  void file_sink::open(
    const std::string& filename, std::ios_base::openmode mode ) {
    pimpl_.reset( new impl_type( filename, mode|std::ios::out ) );
  }

  std::streamsize file_sink::write( const char* s, std::streamsize n ) {
    return pimpl_->write( s, n );
  }

  std::streampos file_sink::seek(
    std::streamoff off, std::ios_base::seekdir way ) {
    return pimpl_->seek( off, way );
  }

  void file::open( const std::string& filename, std::ios_base::openmode mode ) {
    pimpl_.reset( new impl_type( filename, mode|std::ios::in|std::ios::out ) );
  }

  std::streamsize file::read( char* s, std::streamsize n ) {
    return pimpl_->read( s, n );
  }

  std::streamsize file::write( const char* s, std::streamsize n ) {
    return pimpl_->write( s, n );
  }

  std::streampos file::seek( std::streamoff off, std::ios_base::seekdir way ) {
    return pimpl_->seek( off, way );
  }

} } // end namespaces io, mist
