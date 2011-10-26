#pragma once
#include "types.h"

namespace mist {

  static const unsigned char hex_table[] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  };

  static inline unsigned char char_to_hex( char c ) {
    unsigned char uc = static_cast<unsigned char>( c );
    if ( static_cast<std::size_t>( uc ) >= sizeof(hex_table) )
      throw std::runtime_error( "invalid pattern" );
    unsigned char val = hex_table[ uc ];
    if ( val == 0xFF )
      throw std::runtime_error( "invalid pattern" );
    return val;
  }

  inline uint8_t to_hex( const char (&s)[2] ) {
    return
      ( static_cast<uint8_t>( char_to_hex( s[0] ) ) <<  4 ) |
      ( static_cast<uint8_t>( char_to_hex( s[1] ) )       );
  }

  inline uint16_t to_hex( const char (&s)[4] ) {
    return
      ( static_cast<uint16_t>( char_to_hex( s[0] ) ) << 12 ) |
      ( static_cast<uint16_t>( char_to_hex( s[1] ) ) <<  8 ) |
      ( static_cast<uint16_t>( char_to_hex( s[2] ) ) <<  4 ) |
      ( static_cast<uint16_t>( char_to_hex( s[3] ) )       );
  }

  inline uint32_t to_hex( const char (&s)[8] ) {
    return
      ( static_cast<uint32_t>( char_to_hex( s[0] ) ) << 28 ) |
      ( static_cast<uint32_t>( char_to_hex( s[1] ) ) << 24 ) |
      ( static_cast<uint32_t>( char_to_hex( s[2] ) ) << 20 ) |
      ( static_cast<uint32_t>( char_to_hex( s[3] ) ) << 16 ) |
      ( static_cast<uint32_t>( char_to_hex( s[4] ) ) << 12 ) |
      ( static_cast<uint32_t>( char_to_hex( s[5] ) ) <<  8 ) |
      ( static_cast<uint32_t>( char_to_hex( s[6] ) ) <<  4 ) |
      ( static_cast<uint32_t>( char_to_hex( s[7]   ) )     );
  }

  inline uint64_t to_hex( const char (&s)[16] ) {
    return
      ( static_cast<uint64_t>( char_to_hex( s[ 0] ) ) << 60 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 1] ) ) << 56 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 2] ) ) << 52 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 3] ) ) << 48 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 4] ) ) << 44 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 5] ) ) << 40 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 6] ) ) << 36 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 7] ) ) << 32 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 8] ) ) << 28 ) |
      ( static_cast<uint64_t>( char_to_hex( s[ 9] ) ) << 24 ) |
      ( static_cast<uint64_t>( char_to_hex( s[10] ) ) << 20 ) |
      ( static_cast<uint64_t>( char_to_hex( s[11] ) ) << 16 ) |
      ( static_cast<uint64_t>( char_to_hex( s[12] ) ) << 12 ) |
      ( static_cast<uint64_t>( char_to_hex( s[13] ) ) <<  8 ) |
      ( static_cast<uint64_t>( char_to_hex( s[14] ) ) <<  4 ) |
      ( static_cast<uint64_t>( char_to_hex( s[15]   ) )     );
  }

  inline char from_hex( int n, bool is_upper ) {
    if ( is_upper )
      return "0123456789ABCDEF"[ n ];
    else
      return "0123456789abcdef"[ n ];
  }

  template< typename T >
  inline std::string write_hex( const T& x, bool is_upper ) {
    std::string s;
    for ( std::size_t i = 0; i < sizeof(x); ++i ) {
      uint8_t tmp =
        static_cast<uint8_t>( ( x >> ( ( sizeof(x)-1-i)*8 ) ) & 0xFFu );
      s += from_hex( tmp/16, is_upper );
      s += from_hex( tmp%16, is_upper );
    }
    return s;
  }

  template< class T, std::size_t Size >
  inline T read_hex( const char (&s)[ Size ] ) {
    return to_hex( s );
  }



} // end namespace mist
