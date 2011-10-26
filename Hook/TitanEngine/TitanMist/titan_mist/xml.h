#pragma once
#include "third_party/rapidxml-1.13/rapidxml.hpp"
#include "file.h"
#include "helpers.h"
#include "hex.h"
#include <string>
#include <vector>
#include <memory>

namespace mist { namespace xml {

  struct signature_t {
    std::string type;
    std::string start;
    std::string version;
    std::string unpacker;
    std::string signature;
  };

  struct unpacker_t {
    std::string type;
    std::string file;
  };

  struct entry_t {
    std::string name;
    std::string url;
    std::string description;
    std::string author;
    std::string priority;
    std::vector<signature_t> signatures;
    std::vector<unpacker_t> unpackers;
  };

  class xml_db_file_source;
  class xml_update_file_source;

  class xml_file_source {
    friend class xml_db_file_source;
    friend class xml_update_file_source;

  public:
    xml_file_source( const std::string& path );

  private:
    std::tr1::shared_ptr<io::file_source> src_;
    rapidxml::xml_document<> doc_;
    std::vector<char> xml_data_;

  protected:
    void init();
  };

  class xml_db_file_source : public xml_file_source {
  public:
    explicit xml_db_file_source( const std::string& path );

    bool next_entry();

    entry_t entry() const;

    std::string version() const;

  private:
    std::size_t next_index_;
    std::vector<entry_t> entries_;
    entry_t entry_;
    std::string version_;

    inline unpacker_t get_unpacker( rapidxml::xml_node<>* node );

    inline signature_t get_signature( rapidxml::xml_node<>* node );

    inline entry_t get_entry( rapidxml::xml_node<>* node );

    static bool priority_cmp( const entry_t& e1, const entry_t& e2 );

    void read_xml_db();
  };

  class xml_update_file_source : public xml_file_source {
    struct module_entry_t {
      std::string name;
      std::string type;
      std::string url;
    };

  public:
    explicit xml_update_file_source( const std::string& path );

    bool next_entry();

    uint32_t hash() const;

    std::string name() const;

    std::string type() const;

    std::string url() const;

  private:
    std::map<uint32_t, module_entry_t> modules_;
    std::map<uint32_t, module_entry_t>::const_iterator p_;
    std::map<uint32_t, module_entry_t>::const_iterator e_;
    std::pair<uint32_t, module_entry_t> entry_;

    void read_xml_update();
  };

} } // end namespace xml, mist
