#pragma once
#include "types.h"
#include "file.h"
#include "consts.h"
#include <string>
#include <memory>

namespace mist { namespace hash {

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
  +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

  class hash_file {
  public:
    hash_file( const std::string& path );

    uint32_t hash() const;

  private:
    std::tr1::shared_ptr<io::file_source> src_;
    uint32_t hash_;

    // SuperFastHash by Paul Hsieh
    // http://www.azillionmonkeys.com/qed/hash.html
    inline void SuperFastHash (const char * data, int len );
  };

} } // end namespace hash, mist
