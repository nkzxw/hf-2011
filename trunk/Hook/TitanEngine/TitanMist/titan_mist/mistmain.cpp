#include "file.h"
#include "hex.h"
#include "parser.h"
#include "interpreter.h"
#include "pe.h"
#include "xml.h"
#include "update.h"

#include "python_engine.h"
#include "lua_engine.h"
#include "native_engine.h"
#include "titanscript_engine.h"

#include "titan_engine/SDK.h"

#include <ShellAPI.h>
#include <iostream>
#include <algorithm>
#include <set>

namespace mist {

  enum module_t {
    MODULE_NONE,
    MODULE_TITANSCRIPT,
    MODULE_LUA,
    MODULE_PYTHON,
    MODULE_NATIVE
  };

  std::map<std::string, module_t> g_modules = map_list_of
    ( std::string( "python" ), MODULE_PYTHON )
    ( std::string( "lua" ), MODULE_LUA )
    ( std::string( "native" ), MODULE_NATIVE )
    ( std::string( "titanscript" ), MODULE_TITANSCRIPT )
    ( std::string( "ts" ), MODULE_TITANSCRIPT );

  bool priority_cmp(
    const mist::xml::unpacker_t& u1, const mist::xml::unpacker_t& u2 ) {
      return g_modules[ u1.type ] > g_modules[ u2.type ];
  }

  int main( int argc, char** argv ) {
    std::cout << 
      " ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n"
      " ³         TitanMist by ReversingLabs Corp.             ³\n"
      " ³                                                      ³Û\n"
      " ³ version  :            1.0                            ³Û\n"
      " ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙÛ\n"
      "   ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß\n\n";

    char* update = get_switch( argv, argv + argc, "-update" );
    if ( update ) {
      std::auto_ptr<mist::update::updater> updater(
        new mist::update::updater() );
      updater->update();
      return 0;
    }

    char* input_file = get_opt( argv, argv + argc, "-i" );
    if ( !input_file ) {
      std::cerr << "usage: titan_mist -i input [options]\n"
        "\n"
        "  [-o output]     user defined output file\n"
        "  [-d database]   use custom database\n"
        "  [-m]            just match signature, don't unpack\n"
        "  [-mg]           just match signature and go to wiki url\n"
        "  [-update]       update database and modules\n"
        "  [-f]            try to unpack with all available unpackers\n"
        "  [-n]            enable nexus plugin before unpacking\n"
        "  [-t type]       preferred unpacker type\n"
        "                  types:\n"
        "                  native, python, lua, titanscript/ts\n\n";
      return 1;
    }
    char* just_match = get_switch( argv, argv + argc, "-m" );
    char* just_match_and_go = get_switch( argv, argv + argc, "-mg" );
    bool do_just_match = ( just_match != 0 ) || ( just_match_and_go != 0 );
    bool go_to_wiki = ( just_match_and_go != 0 );
    char* output_file = NULL;
    if ( !do_just_match )
      output_file = get_opt( argv, argv + argc, "-o" );
    char* db_file = get_opt( argv, argv + argc, "-d" );
    char* force_unpack = get_switch( argv, argv + argc, "-f" );

    std::string path;
    if ( !do_just_match && !output_file ) {
      path = get_basename( input_file );
      path += ".unpacked";
      path += get_extension( input_file );
      output_file = const_cast<char*>( path.c_str() );
    }

    mist::xml::xml_db_file_source xml( db_file ? db_file : "db.xml" );
    mist::pe::pe_file_source file( input_file );
    mist::parser<std::string> parser;
    mist::interpreter<mist::pe::pe_file_source,mist::parser<std::string> >
      interpreter( file, &parser );

    bool match_found = false;
    mist::xml::entry_t entry;
    std::string default_unpacker;
    while ( xml.next_entry() ) {
      entry = xml.entry();
      std::vector<mist::xml::signature_t>::const_iterator
        sig_p = entry.signatures.begin(),
        sig_e = entry.signatures.end();
      while ( sig_p != sig_e ) {
        mist::xml::signature_t signature = *sig_p;
        parser.parse( signature.signature );
        bool bail_on_fail = false;
        if ( signature.start == "ep" ) {
          interpreter.set_start_pos( file.entry_point() );
          interpreter.set_size( file.ep_section_size() );
          bail_on_fail = true;
        } else if ( signature.start == "all" || signature.start == "begin" ) {
          interpreter.set_start_pos(0);
          interpreter.set_size( file.size() );
          if ( signature.start == "begin" )
            bail_on_fail = true;
        } else if ( signature.start == "overlay" ) {
          interpreter.set_start_pos( file.overlay_start() );
          interpreter.set_size( file.overlay_size() );
          bail_on_fail = true;
        } else
          throw std::runtime_error( "invalid signature start" );
        
        if ( !signature.type.empty() )
            bail_on_fail = ( signature.type == "match" );

        if ( interpreter.find_match( bail_on_fail ) ) {
          std::cout <<
            "Û Match found!\n" <<
            "³ Name:\t\t" << entry.name << "\n" <<
            "³ Version:\t" << signature.version << "\n" <<
            "³ Author:\t" << entry.author << "\n" <<
            "³ Wiki url:\t" << entry.url << "\n" <<
            "³ Description:\n\t" << entry.description << "\n\n";
          match_found = true;
          default_unpacker = signature.unpacker;
          break;
        }
        ++sig_p;
      }
      if ( match_found ) {
        if ( go_to_wiki ) {
          std::cout << "Û Launching wiki url... " << entry.url << "\n";
          ::ShellExecuteA(
            NULL, "open", entry.url.c_str(), NULL, NULL, SW_SHOWNORMAL );
        }
        break;
      }
    }

    if ( match_found && !do_just_match ) {
      mist::xml::unpacker_t unpacker;

      char* unpacker_type = get_opt( argv, argv + argc, "-t" );

      if ( !unpacker_type && !default_unpacker.empty() )
        unpacker_type = const_cast<char*>( default_unpacker.c_str() );

      module_t module_type = MODULE_NONE;
      if ( unpacker_type ) {
        if ( !map_contains( std::string( unpacker_type ), g_modules ) )
          throw std::runtime_error( "invalid unpacking module type" );
        module_type = g_modules[ std::string( unpacker_type ) ];
      }

      if ( entry.unpackers.empty() )
        throw std::runtime_error( "no unpacker found for detected signature!" );

      std::vector<mist::xml::unpacker_t>::const_iterator
        p = entry.unpackers.begin(), e = entry.unpackers.end();
      while ( p != e ) {
        if ( !map_contains( (*p).type, g_modules ) )
          throw std::runtime_error( "bad db module type entry" );
        if ( ( module_type != MODULE_NONE ) &&
          ( g_modules[ (*p).type ] == module_type ) ) {
            unpacker = *p;
            break;
        }
        ++p;
      }

      if ( unpacker.file.empty() ) {
        std::sort(
          entry.unpackers.begin(), entry.unpackers.end(), priority_cmp );
        if ( module_type == MODULE_NONE ) {
          unpacker = entry.unpackers[0];
        } else {
          while ( true ) {
            std::string line;
            std::cout <<
              "Selected unpacking module type does not exist!\n" \
              "Select available module type (Q/q to quit):\n";
            for ( std::size_t i = 0; i < entry.unpackers.size(); ++i ) {
              std::cout << i + 1 << ". " << entry.unpackers[ i ].type << "\n";
            }
            std::cout << "Choice: ";
            std::getline( std::cin, line );
            if ( line == "q" || line == "Q" )
              return 0;

            if ( static_cast<std::size_t>( line[0] - '0' ) >
              entry.unpackers.size()  ) {
                std::cerr << "Invalid module number entered, try again.\n";
                continue;
            }

            unpacker =
              entry.unpackers[ static_cast<std::size_t>( line[0] - '0' ) - 1 ];
            break;
          }
        }
        module_type = g_modules[ unpacker.type ];
      }

      char* unload_nexus = get_switch( argv, argv + argc, "-n" );

      std::set<module_t> tried;
      bool failed = true;
      while ( failed ) {
        std::auto_ptr<mist::engine::basic_engine> engine;
        switch ( module_type ) {
        case MODULE_LUA:
          engine.reset( new mist::engine::lua_engine( unpacker.file ) );
          break;
        case MODULE_NATIVE:
          engine.reset(
            new mist::engine::native_engine( unpacker.file ) );
          break;
        case MODULE_PYTHON:
          engine.reset(
            new mist::engine::python_engine( unpacker.file ) );
          break;
        case MODULE_TITANSCRIPT:
          engine.reset(
            new mist::engine::titanscript_engine( unpacker.file ) );
          break;
        default:
          throw std::runtime_error( "invalid unpacking engine" );
          break;
        }

        if ( !unload_nexus )
          ExtensionManagerUnloadPlugin( "Nexus" );

        if ( engine->unpack( input_file, output_file ) )
          failed = false;
        else {
          bool no_unpackers_left = true;
          failed = true;
          if ( !force_unpack )
            break;
          tried.insert( module_type );
          std::vector<mist::xml::unpacker_t>::const_iterator
            ep = entry.unpackers.begin(), ee = entry.unpackers.end();          
          while ( ep != ee ) {
            if ( tried.find( g_modules[ (*ep).type ] ) == tried.end() ) {
              module_type = g_modules[ (*ep).type ];
              no_unpackers_left = false;
              unpacker = *ep;
              break;
            }
            ++ep;
          }
          if ( no_unpackers_left )
            break;
        }
      }
      if ( !failed )
        std::cout << "Û Unpacking succeeded!\n\n";
      else
        std::cout << "Û Unpacking failed!\n\n";
    } else if ( !match_found )
      std::cout << "Û Match not found!\n\n";

    return 0;
  }

} // end namespace mist
