#include "types.h"
#include "opcodes.h"
#include "consts.h"
#include <memory>

namespace mist {

  template< class Source, class Parser >
  class interpreter {
  public:
    interpreter( const Source& src, Parser* parser,
      std::size_t buffer_size = default_buffer_size )
      : src_( src ), parser_( parser ) {}

    bool find_match( bool bail_on_first_fail = false ) {
      return interpret_and_match( bail_on_first_fail );
    }

    void set_start_pos( uint64_t start_pos ) {
      start_pos_ = start_pos;
    }

    void set_size( uint64_t size ) {
      size_ = size;
    }

  private:
    Source src_;
    Parser* parser_;
    uint64_t pos_;
    uint64_t start_pos_;
    uint64_t last_pos_;
    uint64_t size_;
    uint64_t optional_pos_;
    uint8_t byte_;
    bool is_seek_range_;
    bool is_seek_range_neg_;
    uint32_t seek_range_len_;
    uint32_t seek_range_pos_;
    uint64_t last_range_pos_;

    inline bool match_byte( uint8_t b ) const {
      return byte_ == b;
    }

    inline bool match_mask1( uint8_t m ) const {
      return ( byte_ & 0xF ) == m;
    }

    inline bool match_mask2( uint8_t m ) const {
      return ( ( byte_ >> 4 ) & 0xF ) == m;
    }

    inline bool match_range( uint32_t range ) const {
      if ( ( byte_ >= low_byte( range ) ) && ( byte_ <= high_byte( range ) ) )
        return true;
      return false;
    }

    inline bool skip( uint32_t len, bool negate = false ) {
      if ( !negate ) {
        --len;
        if ( ( pos_ + len ) > ( start_pos_ + size_ ) )
          return false;
        pos_ =
          src_.seek( static_cast<std::streamoff>( len + pos_ ), std::ios::beg );
        return true;
      } else {
        ++len;
        if ( ( pos_ >= len) && ( ( pos_ - len ) > 0 ) ) {
          pos_ =
            src_.seek(
            static_cast<std::streamoff>( pos_ - len ), std::ios::beg );
          return true;
        }
        return false;
      }
    }

    inline bool do_jump() {
      char addr[4];
      addr[0] = byte_;
      if ( src_.read( addr+1, 3 ) == -1 )
        return false;
      uint32_t va = rotate_left( decode_uint( addr ), 32 );
      uint32_t offset = src_.va_to_offset( va );
      if ( offset == -1 )
        return false;
      pos_ = offset;
      size_ = src_.current_section_offset();
      start_pos_ = 0;
      return true;
    }

    inline bool follow_delta( uint8_t type, bool negate ) {
      char addr[4];
      addr[0] = byte_;
      if ( type > 1 ) {
        if ( src_.read( addr+1, 3 ) == -1 )
          return false;
      } else {
        std::memset( addr+1, 0, 3 );
      }
      uint32_t offset = rotate_left( decode_uint( addr ), 32 );
      uint32_t va = src_.offset_to_va( static_cast<uint32_t>( pos_ - 1 ) );
      if ( va == -1 )
        return false;

      if ( type == 1 ) {
        if ( ( offset & 0x80 ) && !negate )
          return false;
      } else {
        if ( ( offset & 0x80000000 ) && !negate )
          return false;
      }

      if ( ( type == 1 ) && ( offset > 0x7F ) ) {
        offset = 0xFF - offset;
        offset = 0 - offset - type;
      }

      va += offset + type;

      offset = src_.va_to_offset( va );
      if ( offset == -1 )
        return false;
      pos_ = offset;
      size_ = src_.current_section_offset();
      start_pos_ = 0;
      return true;
    }

    inline bool eat_bytes( uint8_t byte ) {
      if ( byte_ != byte )
        return false;

      while ( next_byte() ) {
        if ( byte_ != byte )
          break;
      }

      return true;
    }

    bool interpret_and_match( bool bail_on_first_fail ) {
      bool match = false;
      pos_ = last_pos_ = start_pos_;
      bool is_optional = false;
      bool skip_optional = false;
      bool need_next_byte = true;
      is_seek_range_ = is_seek_range_neg_ = false;
      seek_range_len_ = 0;
      bool seek_range_finished = true;

      while ( parser_->next_instruction() ) {
        bool negate = false;
        instr_t i = parser_->instruction();

        if ( skip_optional )
          if ( i.op == OP_OPTIONAL_END ) {
            skip_optional = false;
            is_optional = false;
            pos_ = optional_pos_;
            continue;
          } else {
            continue;
          }

        if ( need_next_byte ) {
          if ( !next_byte() )
            break;
        } else {
          need_next_byte = true;
        }

        if ( is_seek_range_ ) {
          if ( ++seek_range_pos_ > seek_range_len_ ) {
            if ( !match )
              return false;
            i.op = OP_SEEK_RANGE_END;
            seek_range_finished = true;
          }
        } else {
          match = false;
        }

        switch ( i.op ) {
          case OP_MATCH_RANGE:
            match = match_range( i.args.arg32 );
            break;
          case OP_MATCH_BYTE:
            match = match_byte( i.args.arg8 );
            break;
          case OP_MATCH_BITMASK1:
            match = match_mask1( i.args.arg8 );
            break;
          case OP_MATCH_BITMASK2:
            match = match_mask2( i.args.arg8 );
            break;
          case OP_IGNORE:
            match = true;
            continue;
          case OP_SKIP_NEG:
            negate = true;
          case OP_SKIP:
            match = skip( i.args.arg32, negate );
            break;
          case OP_OPTIONAL_START:
            is_optional = true;
            optional_pos_ = pos_ - 1;
            need_next_byte = false;
            continue;
          case OP_OPTIONAL_END:
            is_optional = false;
            need_next_byte = false;
            match = true;
            continue;
          case OP_JUMP:
            match = do_jump();
            break;
          case OP_FOLLOW_DELTA_NEG:
            negate = true;
          case OP_FOLLOW_DELTA:
            match = follow_delta( i.args.arg8, negate );
            break;
          case OP_EAT_BYTES:
            match = eat_bytes( i.args.arg8 );
            if ( match )
              need_next_byte = false;
            break;
          case OP_SEEK_RANGE_NEG_START:
            negate = true;
          case OP_SEEK_RANGE_START:
            match = false;
            seek_range_len_ = i.args.arg32;
            seek_range_pos_ = 0;
            need_next_byte = false;
            is_seek_range_neg_ = negate;
            if ( negate ) {
              if ( seek_range_len_ > pos_ )
                continue;
              pos_ -= seek_range_len_;
            } else {
              if ( ( seek_range_len_ + pos_ ) > ( start_pos_ + size_ ) ) {
                seek_range_len_ =
                  static_cast<uint32_t>( ( start_pos_ + size_ ) - pos_ );
              }
            }
            last_range_pos_ = pos_;
            seek_range_finished = false;
            is_seek_range_ = true;
            parser_->set_rem_index();
            continue;
          case OP_SEEK_RANGE_END:
            need_next_byte = false;
            if ( match || seek_range_finished ) {
              is_seek_range_ = false;
              break;
            }
            rewind();
            match = false;
            continue;
        }
        if ( !match ) {
          if ( is_optional )
            skip_optional = true;
          else {
            if ( is_seek_range_ || !bail_on_first_fail )
              rewind();
            else
              return false;
          }
        }
      }
      if ( parser_->next_instruction() )
        match = false;
      return match;
    }

    bool next_byte() {
      if ( pos_ > ( start_pos_ + size_ - 1 ) )
        return false;
      src_.seek( static_cast<std::streamoff>( pos_ ), std::ios::beg );
      if ( src_.read( reinterpret_cast<char*>( &byte_ ), 1 ) != -1 ) {
        ++pos_;
        return true;
      }
      return false;
    }

    void rewind() {
      pos_ =
        src_.seek(
        static_cast<std::streamoff>(
        is_seek_range_ ? ++last_range_pos_ : ++last_pos_ ),
        std::ios::beg );
      if ( is_seek_range_ )
        parser_->restore_rem_index();
      else
        parser_->rewind_code();
    }
  };

} // end namespace mist
