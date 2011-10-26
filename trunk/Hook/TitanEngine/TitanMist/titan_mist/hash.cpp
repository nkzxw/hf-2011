#include "hash.h"
#include <iostream>
namespace mist { namespace hash {

  hash_file::hash_file( const std::string& path )
    : src_( new io::file_source( path ) ), hash_(0)  {
      std::auto_ptr<char> data(
        new char[ static_cast<std::size_t>( src_->size() ) ] );
      if ( src_->read(
        data.get(), static_cast<std::streamsize>( src_->size() ) ) != -1 )
        SuperFastHash( data.get(), static_cast<int>( src_->size() ) );
      else
        throw std::runtime_error( "file hash failed" );
  }

  uint32_t hash_file::hash() const {
    return hash_;
  }

  inline void hash_file::SuperFastHash (const char * data, int len ) {
      uint32_t tmp;
      int rem;

      if (len <= 0 || data == NULL)
        throw std::runtime_error( "invalid hash data" );

      rem = len & 3;
      len >>= 2;

      /* Main loop */
      for (;len > 0; len--) {
        hash_  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash_;
        hash_   = (hash_ << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash_  += hash_ >> 11;
      }

      /* Handle end cases */
      switch (rem) {
          case 3: hash_ += get16bits (data);
            hash_ ^= hash_ << 16;
            hash_ ^= data[sizeof (uint16_t)] << 18;
            hash_ += hash_ >> 11;
            break;
          case 2: hash_ += get16bits (data);
            hash_ ^= hash_ << 11;
            hash_ += hash_ >> 17;
            break;
          case 1: hash_ += *data;
            hash_ ^= hash_ << 10;
            hash_ += hash_ >> 1;
      }

      /* Force "avalanching" of final 127 bits */
      hash_ ^= hash_ << 3;
      hash_ += hash_ >> 5;
      hash_ ^= hash_ << 4;
      hash_ += hash_ >> 17;
      hash_ ^= hash_ << 25;
      hash_ += hash_ >> 6;
    }

} } // end namespace hash, mist
