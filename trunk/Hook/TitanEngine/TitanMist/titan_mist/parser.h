#include "types.h"
#include "hex.h"
#include "opcodes.h"
#include "helpers.h"
#include <vector>
#include <iostream>

namespace mist {

  template< class Source >
  class parser {
    typedef typename Source::value_type value_type;

    enum token_t {
      TOK_BIT,
      TOK_QMARK,
      TOK_EXMARK,
      TOK_LPAREN,
      TOK_RPAREN,
      TOK_LBRACKET,
      TOK_RBRACKET,
      TOK_LCURLY,
      TOK_RCURLY,
      TOK_PLUS,
      TOK_MINUS,
      TOK_STAR,
      TOK_NL,
      TOK_WS,
      TOK_EOF
    };

  public:
    parser() {}

    explicit parser( const Source& src ) : src_( src ) {
      p_ = src_.begin();
      e_ = src_.end();

      parse_impl();
    }

    void rewind_code() {
      instr_index_ = 0;
    }

    void set_rem_index() {
      rem_instr_index_ = instr_index_;
    }

    void restore_rem_index() {
      instr_index_ = rem_instr_index_;
    }

    bool next_instruction() {
      if ( instr_index_ >= code_.size() )
        return false;

      instruction_ = code_[ instr_index_++ ];
      return true;
    }

    instr_t instruction() const {
      return instruction_;
    }

    void parse( const Source& src ) {
      src_ = src;
      p_ = src_.begin();
      e_ = src_.end();
      last_value_ = 0;
      optional_count_ = 0;
      code_.clear();
      parse_impl();
    }

    void dump() {
      std::vector<instr_t>::const_iterator p = code_.begin(), e = code_.end();
      while ( p != e ) {
        instr_t i = *p;
        switch ( i.op ) {
          case OP_MATCH_RANGE:
            std::cout << "RANGE\t" << std::hex <<
              static_cast<uint32_t>( low_byte( i.args.arg32 ) ) << " to " <<
              static_cast<uint32_t>( high_byte( i.args.arg32 ) ) << "\n";
            break;

          case OP_MATCH_BYTE:
            std::cout << "BYTE\t" << std::hex <<
              static_cast<uint32_t>( i.args.arg8 ) << "\n";
            break;

          case OP_MATCH_BITMASK1:
            std::cout << "BITMASK1\t" << std::hex <<
              static_cast<uint32_t>( i.args.arg8 ) << "\n";
            break;

          case OP_MATCH_BITMASK2:
            std::cout << "BITMASK2\t" << std::hex <<
              static_cast<uint32_t>( i.args.arg8 ) << "\n";
            break;

          case OP_SKIP:
            std::cout << "SKIP\t" << std::hex <<
              static_cast<uint32_t>( i.args.arg32 ) << "\n";
            break;

          case OP_IGNORE:
            std::cout << "IGNORE\n";
        }
        ++p;
      }
    }

  private:
    Source src_;
    typename Source::const_iterator p_;
    typename Source::const_iterator e_;
    value_type last_value_;
    std::vector<instr_t> code_;
    std::size_t instr_index_;
    instr_t instruction_;
    int optional_count_;
    std::size_t rem_instr_index_;

    void parse_impl() {
      token_t token;
      instr_index_ = 0;
      while ( ( token = next_token() ) != TOK_EOF ) {
        switch ( token ) {
          case TOK_QMARK:
            code_.push_back( parse_ignore() );
            break;

          case TOK_EXMARK:
            code_.push_back( parse_follow_delta() );
            break;

          case TOK_LBRACKET:
            code_.push_back( parse_range() );
            break;

          case TOK_PLUS:
            code_.push_back( parse_skip() );
            break;

          case TOK_MINUS:
            code_.push_back( parse_skip( true ) );
            break;

          case TOK_STAR:
            code_.push_back( parse_eat_bytes() );
            break;

          case TOK_LPAREN:
            code_.push_back( parse_optional( true ) );
            break;

          case TOK_RPAREN:
            code_.push_back( parse_optional( false ) );
            break;

          case TOK_RCURLY:
            code_.push_back( parse_seek_range_end() );
            break;

          case TOK_NL:
          case TOK_WS:
            break;

          case TOK_BIT:
            code_.push_back( parse_byte() );
            break;

          default:
            throw std::runtime_error( "syntax error" );
        }
      }
      if ( optional_count_ )
        throw std::runtime_error( "syntax error" );
    }

    instr_t parse_eat_bytes() {
      instr_t i;

      if ( next_token() != TOK_LPAREN )
        throw std::runtime_error( "syntax error" );

      char buf[2];
      if ( next_token() != TOK_BIT )
        throw std::runtime_error( "syntax error" );
      buf[0] = last_value_;
      if ( next_token() != TOK_BIT )
        throw std::runtime_error( "syntax error" );
      buf[1] = last_value_;

      if ( next_token() != TOK_RPAREN )
        throw std::runtime_error( "syntax error" );

      i.op = OP_EAT_BYTES;
      i.args.arg8 = read_hex<char>( buf );

      return i;
    }

    instr_t parse_follow_delta() {
      instr_t i;

      if ( next_token() != TOK_LPAREN )
        throw std::runtime_error( "syntax error" );

      bool negate = false;
      bool get_next = true;
      token_t token = next_token();
      if ( token == TOK_MINUS )
        negate = true;
      else if ( token == TOK_BIT )
        get_next = false;

      if ( get_next )
        if ( next_token() != TOK_BIT )
          throw std::runtime_error( "syntax error" );

      uint8_t delta_type = static_cast<uint8_t>( last_value_ - '0' );
      if ( next_token() != TOK_RPAREN )
        throw std::runtime_error( "syntax error" );

      switch ( delta_type ) {
        case 2:
          delta_type = 1;
          break;
        case 5:
        case 6:
          delta_type = 4;
          break;
        default:
          throw std::runtime_error( "syntax error" );
      }

      if ( negate )
        i.op = OP_FOLLOW_DELTA_NEG;
      else
        i.op = OP_FOLLOW_DELTA;

      i.args.arg8 = delta_type;

      return i;
    }

    instr_t parse_optional( bool start ) {
      instr_t i;

      if ( start ) {
        i.op = OP_OPTIONAL_START;
        ++optional_count_;
      } else {
        i.op = OP_OPTIONAL_END;
        --optional_count_;
      }

      return i;
    }

    instr_t parse_range() {
      instr_t i;

      char buf1[2];
      char buf2[2];

      char* buf = buf1;
      int limit = sizeof(buf1)*2;
      while ( true ) {
        if ( next_token() != TOK_BIT )
          throw std::runtime_error( "syntax error" );
        
        *buf++ = last_value_;
        if ( next_token() != TOK_BIT )
          throw std::runtime_error( "syntax error" );

        *buf = last_value_;

        limit -= sizeof(buf1);
        if ( !limit )
          break;

        if ( next_token() != TOK_MINUS )
          throw std::runtime_error( "syntax error" );

        buf = buf2;
      }

      if ( next_token() != TOK_RBRACKET )
        throw std::runtime_error( "syntax error" );

      i.op = OP_MATCH_RANGE;
      i.args.arg32 =
        ( read_hex<char>( buf2 ) << 8 ) | ( read_hex<char>( buf1 ) );

      return i;
    }

    instr_t parse_seek_range_end() {
      instr_t i;

      i.op = OP_SEEK_RANGE_END;

      return i;
    }

    instr_t parse_seek_range( bool is_minus ) {
      instr_t i;

      char buf[4];
      std::memset( buf, 0, sizeof(buf) );

      if ( next_token() != TOK_LBRACKET )
        throw std::runtime_error( "syntax error" );

      int bit_count = 0;
      token_t t;
      while ( ( t = next_token() ) != TOK_RBRACKET ) {
        if ( t != TOK_BIT )
          throw std::runtime_error( "syntax error" );
        if ( bit_count >= sizeof(buf) )
          throw std::runtime_error( "syntax error" );
        buf[ bit_count ] = last_value_;
        ++bit_count;
      }

      if ( bit_count < sizeof(buf) ) {
        char* buf_ptr = buf + ( sizeof(buf)-bit_count );
        std::memcpy( buf_ptr, buf, bit_count );
        std::memset( buf, 0, sizeof(buf)-bit_count );
      }

      i.args.arg32 = read_hex<uint32_t>( buf );
      if ( is_minus )
        i.op = OP_SEEK_RANGE_NEG_START;
      else
        i.op = OP_SEEK_RANGE_START;

      if ( next_token() != TOK_LCURLY )
        throw std::runtime_error( "syntax error" );

      return i;
    }

    instr_t parse_skip( bool is_minus = false ) {
      instr_t i;

      if ( peek_next_token() == TOK_LBRACKET )
        return parse_seek_range( is_minus );

      char buf[4];
      std::memset( buf, 0, sizeof(buf) );
      if ( next_token() != TOK_LPAREN )
        throw std::runtime_error( "syntax error" );

      if ( peek_next_token() == TOK_QMARK ) {
        next_token();
        if ( next_token() != TOK_RPAREN )
          throw std::runtime_error( "syntax error" );
        i.op = OP_JUMP;
        return i;
      }
      
      int bit_count = 0;
      token_t t;
      while ( ( t = next_token() ) != TOK_RPAREN ) {
        if ( t != TOK_BIT )
          throw std::runtime_error( "syntax error" );
        if ( bit_count >= sizeof(buf) )
          throw std::runtime_error( "syntax error" );
        buf[ bit_count ] = last_value_;
        ++bit_count;
      }

      if ( bit_count < sizeof(buf) ) {
        char* buf_ptr = buf + ( sizeof(buf)-bit_count );
        std::memcpy( buf_ptr, buf, bit_count );
        std::memset( buf, 0, sizeof(buf)-bit_count );
      }

      i.args.arg32 = read_hex<uint32_t>( buf );
      if ( !is_minus )
        i.op = OP_SKIP;
      else
        i.op = OP_SKIP_NEG;

      return i;
    }

    instr_t parse_mask() {
      instr_t i;

      char buf[2];
      buf[0] = 0;
      token_t next_tok = peek_next_token();
      if ( next_tok == TOK_BIT ) {
        next_token();
        buf[1] = last_value_;
        i.op = OP_MATCH_BITMASK1;
      } else if ( next_tok == TOK_QMARK ) {
        buf[1] = last_value_;
        i.op = OP_MATCH_BITMASK2;
        next_token();
      } else {
        throw std::runtime_error( "syntax error" );
      }
      
      i.args.arg8 = read_hex<char>( buf );

      return i;
    }

    instr_t parse_ignore() {
      instr_t i;

      token_t next_tok = peek_next_token();
      if ( next_tok == TOK_BIT ) {
        return parse_mask();
      } else if ( next_tok != TOK_QMARK ) {
        throw std::runtime_error( "syntax error" );
      }

      next_token();
      i.op = OP_IGNORE;

      return i;
    }

    instr_t parse_byte() {
      instr_t i;

      char buf[2];
      token_t next_tok = peek_next_token();
      if ( next_tok == TOK_QMARK ) {
        return parse_mask();
      } else if ( next_tok != TOK_BIT ) {
        throw std::runtime_error( "syntax error" );
      }

      buf[0] = last_value_;
      if ( next_token() != TOK_BIT )
        throw std::runtime_error( "syntax error" );
      buf[1] = last_value_;

      i.op = OP_MATCH_BYTE;
      i.args.arg8 = read_hex<char>( buf );
    
      return i;
    }

    token_t peek_next_token() {
      return next_token( true );
    }

    token_t next_token( bool peek = false ) {
      if ( p_ == e_ )
        return TOK_EOF;

      if ( !peek )
        last_value_ = *p_;

      switch ( peek ? *p_ : *p_++ ) {
        case '?':
          return TOK_QMARK;
        case '!':
          return TOK_EXMARK;
        case '[':
          return TOK_LBRACKET;
        case ']':
          return TOK_RBRACKET;
        case '{':
          return TOK_LCURLY;
        case '}':
          return TOK_RCURLY;
        case '+':
          return TOK_PLUS;
        case '-':
          return TOK_MINUS;
        case '*':
          return TOK_STAR;
        case '(':
          return TOK_LPAREN;
        case ')':
          return TOK_RPAREN;
        case '\n':
          return TOK_NL;
        case ' ':
          return TOK_WS;
        case '\t':
          return TOK_WS;
        case '\r':
          return TOK_NL;
        default:
          if ( hex_table[ peek ? *p_ : last_value_ ] == 0xFF )
            throw std::runtime_error( "syntax error" );
          return TOK_BIT;
      }

      MIST_UNREACHABLE_RETURN(0)
    }

  };

} // end namespace mist
