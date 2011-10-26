#include "python_engine.h"

namespace mist { namespace engine {

  python_engine::python_engine( const std::string& unpacker ) {
    if ( unpacker.substr( unpacker.size() - 3, 3 ) == ".py" )
      unpacker_ = unpacker.substr( 0, unpacker.size() - 3 );
    else
      unpacker_ = unpacker;
  }

  bool python_engine::unpack_impl(
    const std::string& input, const std::string& output ) {

    Py_Initialize();

    // set python modules path
    PyObject* path = PySys_GetObject( "path" );
    PyObject* modules_path = PyString_FromString( "./unpackers/python" );
    PyList_Insert( path, 0, modules_path );
    Py_DECREF( modules_path );

    // load unpacker module
    PyObject* module = PyImport_ImportModule( unpacker_.c_str() );
    if ( !module ) {
      PyErr_Print();
      PyErr_Clear();
      std::string err = "Failed to initialize module: ";
      err += unpacker_;
      throw std::runtime_error( err );
    }

    // get dict from module and check if it contains unpacking function name
    PyObject* dict = PyModule_GetDict( module );
    PyObject* unpack_func = PyDict_GetItemString( dict, "unpack" );
    if ( !unpack_func || !PyCallable_Check( unpack_func ) ) {
      if ( PyErr_Occurred() ) {
        PyErr_Print();
        PyErr_Clear();
      }
      throw
        std::runtime_error( "\"unpack\" - unpacking function not found!" );
    }

    // put args we want to pass into a tuple
    PyObject* args = PyTuple_New(3);
    PyTuple_SetItem( args, 0, PyString_FromString( input.c_str() ) );
    PyTuple_SetItem( args, 1, PyString_FromString( output.c_str() ) );
    PyTuple_SetItem( args, 2, PyInt_FromSize_t(0) );

    PyObject* ret = PyObject_CallObject( unpack_func, args );

    bool res;
    if ( PyBool_Check( ret ) )
      res = ( ret == Py_True );
    else
      throw std::runtime_error( "invalid result type" );

    Py_XDECREF( ret );
    Py_DECREF( args );

    return res;
  }

} } // end namespace engine, mist
