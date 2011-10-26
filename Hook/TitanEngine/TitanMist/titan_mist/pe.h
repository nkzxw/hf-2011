#include "types.h"
#include "file.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>

namespace mist { namespace pe {

  class pe_file_source {
  public:
    pe_file_source( const std::string& path )
      : impl_( new io::file_source( path ) ), size_( impl_->size() ),
      nt_header_( new IMAGE_NT_HEADERS ) {
        overlay_start_ = 0;
        overlay_size_ = static_cast<uint32_t>( size_ );

        std::auto_ptr<char> header( new char[ sizeof(IMAGE_DOS_HEADER) ] );
        impl_->read( header.get(), sizeof(IMAGE_DOS_HEADER) );
        PIMAGE_DOS_HEADER mz_header =
          reinterpret_cast<PIMAGE_DOS_HEADER>( header.get() );
        if ( mz_header->e_magic != IMAGE_DOS_SIGNATURE )
          throw std::runtime_error( "bad pe file" );

        std::streampos nt_header_pos = impl_->seek(
          static_cast<std::streamoff>( mz_header->e_lfanew ),
          std::ios::beg );

        impl_->read( reinterpret_cast<char*>( nt_header_.get() ),
          sizeof(IMAGE_NT_HEADERS) );
        if ( nt_header_->Signature != IMAGE_NT_SIGNATURE )
          throw std::runtime_error( "bad pe file" );

        if ( nt_header_->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 )
          throw std::runtime_error( "unsupported pe file" );

        first_section_pos_ = impl_->seek(
          static_cast<std::streamoff>( static_cast<::LONG>( nt_header_pos ) +
          FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +
          nt_header_->FileHeader.SizeOfOptionalHeader ),
          std::ios::beg );

        ::DWORD section_num = nt_header_->FileHeader.NumberOfSections;

        IMAGE_SECTION_HEADER section;
        for ( ::DWORD i = 0; i < section_num; ++i ) {
          std::memset( &section, 0, sizeof(IMAGE_SECTION_HEADER) );
          impl_->read(
            reinterpret_cast<char*>( &section ), sizeof(IMAGE_SECTION_HEADER) );
          sections_.push_back( section );
        }

        get_entry_point();
        get_overlay();

        impl_->seek( 0, std::ios::beg );
    }

    std::streamsize read( char* s, std::streamsize n ) {
      return impl_->read( s, n );
    }

    std::streampos seek( std::streamoff off, std::ios_base::seekdir way ) {
      return impl_->seek( off, way );
    }

    uint32_t entry_point() const {
      return entry_point_;
    }

    uint32_t ep_section_size() const {
      return ep_section_size_;
    }

    uint32_t overlay_start() const {
      return overlay_start_;
    }

    uint32_t overlay_size() const {
      return overlay_size_;
    }

    uint64_t size() const {
      return size_;
    }

    uint32_t current_section_size() const {
      return current_section_size_;
    }

    uint32_t current_section_offset() const {
      return current_section_offset_;
    }

    uint32_t va_to_offset( uint32_t va ) {

      ::DWORD nt_size;
      if ( ( nt_header_->OptionalHeader.SizeOfImage %
        nt_header_->OptionalHeader.SectionAlignment ) != 0 ) {
          nt_size = ( (nt_header_->OptionalHeader.SizeOfImage /
            nt_header_->OptionalHeader.SectionAlignment) + 1 ) *
            nt_header_->OptionalHeader.SectionAlignment;
      }

      if ( va < nt_header_->OptionalHeader.ImageBase ||
        va > ( nt_header_->OptionalHeader.ImageBase + nt_size ) )
        return -1;

      va -= nt_header_->OptionalHeader.ImageBase;

      if ( va < nt_header_->OptionalHeader.SectionAlignment ) {
        current_section_size_ = nt_header_->OptionalHeader.FileAlignment;
        current_section_offset_ = nt_header_->OptionalHeader.SectionAlignment;
        return va;
      }

      for ( std::size_t i = 0; i < sections_.size(); ++i ) {
        PIMAGE_SECTION_HEADER section = &sections_[ i ];
        ::DWORD size;
        if ( ( section->Misc.VirtualSize %
          nt_header_->OptionalHeader.SectionAlignment ) != 0 ) {
            size = ( (section->Misc.VirtualSize /
              nt_header_->OptionalHeader.SectionAlignment) + 1 ) *
              nt_header_->OptionalHeader.SectionAlignment;
        } else {
          size = section->Misc.VirtualSize ?
            section->Misc.VirtualSize : section->SizeOfRawData;
        }
        if ( ( va >= section->VirtualAddress ) &&
          ( va < section->VirtualAddress + size ) ) {
            current_section_size_ =
              static_cast<uint32_t>( section->SizeOfRawData );
            current_section_offset_ =
              static_cast<uint32_t>(
              section->PointerToRawData + section->VirtualAddress );
            return static_cast<uint32_t>( section->PointerToRawData +
              ( va - section->VirtualAddress ) );
        }
      }
      return -1;
    }

    uint32_t offset_to_va( uint32_t offset ) {
      for ( std::size_t i = 0; i < sections_.size(); ++i ) {
        PIMAGE_SECTION_HEADER section = &sections_[ i ];
        if ( ( offset >= section->PointerToRawData ) &&
          ( offset < section->PointerToRawData + section->SizeOfRawData ) ) {
            offset -= section->PointerToRawData;
            offset += section->VirtualAddress;
            offset += nt_header_->OptionalHeader.ImageBase;
            current_section_size_ =
              static_cast<uint32_t>( section->SizeOfRawData );
            current_section_offset_ =
              static_cast<uint32_t>(
              section->PointerToRawData + section->VirtualAddress );
            return offset;
        }
      }
      if ( offset < nt_header_->OptionalHeader.SectionAlignment ) {
        current_section_size_ = nt_header_->OptionalHeader.FileAlignment;
        current_section_offset_ = nt_header_->OptionalHeader.SectionAlignment;
        return offset + nt_header_->OptionalHeader.ImageBase;
      }
      return -1;
    }
    
  private:
    std::tr1::shared_ptr<io::file_source> impl_;
    uint32_t entry_point_;
    uint32_t ep_section_size_;
    uint32_t overlay_start_;
    uint32_t overlay_size_;
    uint64_t size_;
    std::tr1::shared_ptr<IMAGE_NT_HEADERS> nt_header_;
    std::streampos first_section_pos_;
    uint32_t current_section_size_;
    uint32_t current_section_offset_;
    std::vector<IMAGE_SECTION_HEADER> sections_;

    void get_overlay() {
      ::DWORD section_raw_offset = 0;
      ::DWORD section_raw_size = 0;

      for ( std::size_t i = 0; i < sections_.size(); ++i ) {
        PIMAGE_SECTION_HEADER section = &sections_[ i ];
        if ( section->PointerToRawData >= section_raw_offset ) {
          if ( section->SizeOfRawData ||
            ( section_raw_offset != section->PointerToRawData ) )
            section_raw_size = section->SizeOfRawData;
          section_raw_offset = section->PointerToRawData;
        }
        if ( section_raw_offset + section_raw_size < size_ ) {
          overlay_start_ =
            static_cast<uint32_t>( section_raw_offset + section_raw_size );
          overlay_size_ =
            static_cast<uint32_t>(
            size_ - section_raw_offset - section_raw_size );
        }
      }
    }

    void get_entry_point() {
      ::DWORD ep_addr =
        nt_header_->OptionalHeader.AddressOfEntryPoint +
        nt_header_->OptionalHeader.ImageBase;
      entry_point_ = va_to_offset( ep_addr );
      if ( entry_point_ == -1 )
        throw std::runtime_error( "cannot find entry point" );
      ep_section_size_ = current_section_size_;
    }
  };

} } // end namespace pe, mist
