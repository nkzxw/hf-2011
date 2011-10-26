#pragma once

namespace mist {

  class disallow_copy_and_assign {
  protected:
    disallow_copy_and_assign() {}
    ~disallow_copy_and_assign() {}

  private:
    disallow_copy_and_assign( const disallow_copy_and_assign& );
    const disallow_copy_and_assign& operator=(
      const disallow_copy_and_assign& );
  };

} // end namespace mist
