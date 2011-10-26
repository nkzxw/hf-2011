#pragma once
#include "types.h"
#include <map>
#include <algorithm>

#define MIST_UNREACHABLE_RETURN(x)

template< typename T >
inline mist::uint8_t low_byte( T x ) {
  return static_cast<mist::uint8_t>( x & 0xFF );
}

template< typename T >
inline mist::uint8_t high_byte( T x ) {
  return static_cast<mist::uint8_t>( ( x >> 8 ) & 0xFF );
}

template< typename T >
inline mist::uint32_t low_qword( T x ) {
  return static_cast<mist::uint32_t>( x );
}

template< typename T >
inline mist::uint32_t high_qword( T x ) {
  return static_cast<mist::uint32_t>(
    ( static_cast<mist::uint64_t>( x ) >> 32 ) & 0xFFFFFFFF );
}

inline char* get_opt( char** begin, char** end, const std::string& opt ) {
  char** p = std::find( begin, end, opt );
  if ( p != end && ++p != end )
    return *p;
  return 0;
}

inline char* get_switch( char** begin, char** end, const std::string& opt ) {
  char** p = std::find( begin, end, opt );
  if ( p != end )
    return *p;
  return 0;
}

inline std::string get_extension( const std::string& s ) {
  std::string::size_type p = s.rfind( '.' );
  if ( p != std::string::npos )
    return s.substr( p );
  else
    return std::string();
}

inline std::string get_basename( const std::string& s ) {
  std::string::size_type p = s.rfind( '.' );
  return s.substr( 0, p );
}

inline std::string get_filename( const std::string& s ) {
  std::string::size_type p = s.rfind( '\\' );
  return s.substr( p+1, s.size() - p );
}

inline std::string normalize_string(
  const std::string& src, const std::string& c = " \r\n\t" ) {
  typedef std::string::size_type size_type;
  size_type p2 = src.find_last_not_of( c );
  if ( p2 == std::string::npos )
    return std::string();
  size_type p1 = src.find_first_not_of( c );
  if ( p1 == std::string::npos )
    p1 = 0;
  return src.substr( p1, ( p2-p1 ) + 1 );
}

template< typename Key, typename T >
class list_of {
public:
  list_of( const Key& key, const T& value ) {
    list_[ key ] = value;
  }

  list_of<Key, T>& operator()( const Key& key, const T& value ) {
    list_[ key ] = value;
    return *this;
  }

  operator std::map<Key, T>() {
    return list_;
  }
private:
  std::map<Key, T> list_;
};

template< typename Key, typename T >
list_of<Key, T> map_list_of( Key k, T v ) {
  return list_of<Key, T>( k, v );
}

template< typename Key, typename T >
bool map_contains( Key key, T map ) {
  T::iterator lb = map.lower_bound( key );
  if ( lb != map.end() && !( map.key_comp()( key, lb->first ) ) )
    return true;
  return false;
}

inline mist::uint32_t rotate_left( mist::uint32_t n, mist::uint32_t s ) {
  return ::_rotl( n, static_cast<int>( s ) );
}

inline mist::uint32_t decode_uint( const char* s ) {
  return *reinterpret_cast<const mist::uint32_t*>( s );
}