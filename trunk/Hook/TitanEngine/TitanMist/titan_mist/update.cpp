#include "update.h"
#include <iostream>

namespace mist { namespace update {

  updater::updater() : xml_( new xml::xml_db_file_source( "db.xml" ) ) {}

  void updater::update() {
    std::cout << "Û Update starting...\n";
    if ( !is_db_latest() ) {
      std::cout << "³ New database found, downloading...\n";
      xml_.reset();
      download_file(
      "http://www.reversinglabs.com/mist/db.xml", "db.xml" );
    } else
      std::cout << "³ Database up to date.\n";

    std::auto_ptr<filesystem::dir> directory(
      new filesystem::dir( "unpackers" ) );
    std::vector<std::string> directories;
    while ( directory->next_entry() ) {
      if ( directory->is_directory() ) {
        if ( directory->name() != "." && directory->name() != ".." ) {
          std::string dir_name( "unpackers\\" );
          dir_name += directory->name();
          std::auto_ptr<filesystem::dir> d( new filesystem::dir( dir_name ) );
          while ( d->next_entry() ) {
            if ( !d->is_directory() ) {
              std::string tmp = dir_name;
              tmp += "\\";
              tmp += d->name();
              if ( get_extension( tmp ) == ".pyc" )
                continue;
              std::auto_ptr<hash::hash_file> hash(
                new hash::hash_file( tmp ) );
              files_.insert( std::make_pair( hash->hash(), tmp ) );
            }
          }
        }
      }
    }

    std::cout << "Û Getting new module updates...\n";
    download_file(
      "http://www.reversinglabs.com/mist/update.xml", "update.xml" );
    std::auto_ptr<xml::xml_update_file_source> update_file(
      new xml::xml_update_file_source( "update.xml" ) );

    while ( update_file->next_entry() ) {
      if ( !map_contains( update_file->hash(), files_ ) ) {
        std::string dl_path( "unpackers\\" );
        dl_path += update_file->type();
        dl_path += "\\";
        dl_path += update_file->name();
        std::cout << "³ Downloading module: " <<
          update_file->name() << "\n";
        download_file( update_file->url(), dl_path );
      }
    }

    std::cout << "Û Update complete!\n";
  }

  bool updater::is_db_latest() {
    std::cout << "³ Checking for database update...\n";
    download_file(
      "http://www.reversinglabs.com/mist/latest.ver", "latest.ver" );
    std::auto_ptr<io::file_source> file(
      new io::file_source( "latest.ver" ) );
    std::auto_ptr<char> buffer(
      new char[ static_cast<std::size_t>( file->size() ) ] );
    std::streamsize len =
      file->read( buffer.get(), static_cast<std::streamsize>( file->size() ) );
    std::string latest_ver( buffer.get(), len );

    return ( latest_ver == xml_->version() );
  }

  void updater::download_file( const std::string& url, const std::string& path ) {
    if ( ::URLDownloadToFileA(
      NULL, url.c_str(), path.c_str(), NULL, NULL ) != S_OK )
      throw std::runtime_error( "download failed" );
  }

} } // end namespace update, mist
