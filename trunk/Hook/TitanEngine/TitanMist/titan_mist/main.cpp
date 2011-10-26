#include "mist_decl.h"
#include <exception>
#include <iostream>

namespace mist {
  int main( int argc, char** argv );
}

int main( int argc, char** argv ) {
  try {
    return mist::main( argc, argv );
  } catch ( std::exception& e ) {
    std::cerr << "TitanMist encountered an error:\n" << e.what() << "\n";
    return 1;
  }
}
