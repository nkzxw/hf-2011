#include "xml.h"

namespace mist { namespace xml {

  xml_file_source::xml_file_source( const std::string& path )
    : src_( new io::file_source( path ) ),
    xml_data_( static_cast<std::size_t>( src_->size() ) ) {
      init();
  }

  void xml_file_source::init() {
    src_->read( &xml_data_[0], static_cast<std::size_t>( src_->size() ) );
    xml_data_.push_back( '\x00' );
    doc_.parse<0>( &xml_data_[0] );
  }

  xml_db_file_source::xml_db_file_source( const std::string& path )
    : xml_file_source( path ), next_index_(0) {
      read_xml_db();
  }

  bool xml_db_file_source::next_entry() {
    if ( next_index_ >= entries_.size() )
      return false;

    entry_ = entries_[ next_index_++ ];
    return true;
  }

  entry_t xml_db_file_source::entry() const {
    return entry_;
  }

  std::string xml_db_file_source::version() const {
    return version_;
  }

  inline unpacker_t xml_db_file_source::get_unpacker(
    rapidxml::xml_node<>* node ) {
    unpacker_t unpacker;
    rapidxml::xml_attribute<>* attr = node->first_attribute();
    if ( std::memcmp( attr->name(), "type", attr->name_size() ) != 0 )
      throw std::runtime_error( "invalid db file" );

    unpacker.type = normalize_string( attr->value() );
    unpacker.file = normalize_string( node->value() );

    return unpacker;
  }

  inline signature_t xml_db_file_source::get_signature(
    rapidxml::xml_node<>* node ) {
    signature_t signature;
    for ( rapidxml::xml_attribute<>* attr = node->first_attribute(); attr;
      attr = attr->next_attribute() ) {
        if ( std::memcmp( attr->name(), "type", attr->name_size() ) == 0 )
          signature.type = normalize_string( attr->value() );
        else if ( std::memcmp( attr->name(), "start", attr->name_size() ) == 0 )
          signature.start = normalize_string( attr->value() );
        else if (
          std::memcmp( attr->name(), "version", attr->name_size() ) == 0 )
          signature.version = normalize_string( attr->value() );
        else if (
          std::memcmp( attr->name(), "unpacker", attr->name_size() ) == 0  )
          signature.unpacker = normalize_string( attr->value() );
        else
          throw std::runtime_error( "invalid db file" );
    }
    signature.signature = node->value();

    return signature;
  }

  inline entry_t xml_db_file_source::get_entry( rapidxml::xml_node<>* node ) {
    entry_t entry;
    if ( std::memcmp( node->name(), "entry", node->name_size() ) != 0 )
      throw std::runtime_error( "invalid db file" );
    for ( rapidxml::xml_attribute<>* attr = node->first_attribute(); attr;
      attr = attr->next_attribute() ) {
        if ( std::memcmp( attr->name(), "name", attr->name_size() ) == 0 )
          entry.name = normalize_string( attr->value() );
        else if ( std::memcmp( attr->name(), "url", attr->name_size() ) == 0 )
          entry.url = normalize_string( attr->value() );
        else if (
          std::memcmp( attr->name(), "description", attr->name_size() ) == 0 )
          entry.description = normalize_string( attr->value() );
        else if (
          std::memcmp( attr->name(), "author", attr->name_size() ) == 0 )
          entry.author = normalize_string( attr->value() );
        else if (
          std::memcmp( attr->name(), "priority", attr->name_size() ) == 0 )
          entry.priority = normalize_string( attr->value() );
        else
          throw std::runtime_error( "invalid db file" );
    }

    for ( rapidxml::xml_node<>* next_node = node->first_node(); next_node;
      next_node = next_node->next_sibling() ) {
        if ( std::memcmp(
          next_node->name(), "signature", next_node->name_size() ) == 0 )
          entry.signatures.push_back( get_signature( next_node ) );
        else if ( std::memcmp(
          next_node->name(), "unpacker", next_node->name_size() ) == 0 )
          entry.unpackers.push_back( get_unpacker( next_node ) );
        else
          throw std::runtime_error( "invalid db file" );
    }

    return entry;
  }

  bool xml_db_file_source::priority_cmp( const entry_t& e1, const entry_t& e2 ) {
    int p1 = 1;
    if ( !e1.priority.empty() )
      p1 = static_cast<int>( e1.priority[0] - '0' );
    int p2 = 1;
    if ( !e2.priority.empty() )
      p2 = static_cast<int>( e2.priority[0] - '0' );

    if ( ( p1 < 1 ) || ( p2 < 1 ) || ( p1 > 3 ) || ( p2 > 3 ) )
      throw std::runtime_error(
      "invalid priority level specified, priority can be between 1 and 3" );

    return p1 > p2;
  }

  void xml_db_file_source::read_xml_db() {
    rapidxml::xml_node<>* mist_node = doc_.first_node( "mistdb" );
    if ( !mist_node )
      throw std::runtime_error( "invalid database file" );

    rapidxml::xml_attribute<>* attr = mist_node->first_attribute();
    if ( std::memcmp( attr->name(), "version", attr->name_size() ) != 0 )
      throw std::runtime_error( "db missing version info" );
    version_ = attr->value();

    for ( rapidxml::xml_node<>* child = mist_node->first_node();
      child; child = child->next_sibling() )
      entries_.push_back( get_entry( child ) );

    std::sort( entries_.begin(), entries_.end(), this->priority_cmp );
  }

  xml_update_file_source::xml_update_file_source( const std::string& path )
    : xml_file_source( path ) {
      read_xml_update();
      p_ = modules_.begin();
      e_ = modules_.end();
  }

  bool xml_update_file_source::next_entry() {
    if ( p_ != e_ ) {
      entry_ = *p_++;
      return true;
    }
    return false;
  }

  uint32_t xml_update_file_source::hash() const {
    return entry_.first;
  }

  std::string xml_update_file_source::name() const {
    return entry_.second.name;
  }

  std::string xml_update_file_source::type() const {
    return entry_.second.type;
  }

  std::string xml_update_file_source::url() const {
    return entry_.second.url;
  }

  void xml_update_file_source::read_xml_update() {
    rapidxml::xml_node<>* mist_node = doc_.first_node( "mistupdate" );
    if ( !mist_node )
      throw std::runtime_error( "invalid update file" );

    for ( rapidxml::xml_node<>* child = mist_node->first_node(); child;
      child = child->next_sibling() ) {
        uint32_t hash;
        module_entry_t entry;
        char buf[8];
        for ( rapidxml::xml_attribute<>* attr = child->first_attribute(); attr;
          attr = attr->next_attribute() ) {
            if ( std::memcmp( attr->name(), "name", attr->name_size() ) == 0 )
              entry.name = attr->value();
            else if (
              std::memcmp( attr->name(), "type", attr->name_size() ) == 0 )
              entry.type = attr->value();
            else if ( std::memcmp( attr->name(), "url", attr->name_size() ) == 0 )
              entry.url = attr->value();
            else if (
              std::memcmp( attr->name(), "hash", attr->name_size() ) == 0 ) {
                std::memcpy( buf, attr->value(), 8 );
                hash = read_hex<uint32_t>( buf );
            } else
              throw std::runtime_error( "invalid update file format" );
        }
        modules_.insert( std::make_pair( hash, entry ) );
    }
  }

} } // end namespace xml, mist
