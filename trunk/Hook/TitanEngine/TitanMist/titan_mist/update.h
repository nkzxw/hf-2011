#pragma once
#include "hash.h"
#include "xml.h"
#include "filesystem.h"
#include <UrlMon.h>
#pragma comment( lib, "urlmon.lib" )
#include <memory>
#include <string>
#include <vector>

namespace mist { namespace update {

  class updater {
  public:
    updater();

    void update();

  private:
    std::tr1::shared_ptr<xml::xml_db_file_source> xml_;
    std::map<uint32_t, std::string> files_;

    bool is_db_latest();

    void download_file( const std::string& url, const std::string& path );
  };

} } // end namespace update, mist
