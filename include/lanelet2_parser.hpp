#pragma once

#include <string>

#include "map_server.hpp"
#include "types.hpp"

namespace hdmap {

// Parser for Lanelet2 XML format
class Lanelet2Parser {
 public:
  Lanelet2Parser() = default;

  // Parse Lanelet2 XML and populate map server
  bool parse(const std::string& filepath, MapServer& mapServer);

  // Get parsing errors
  const std::string& getLastError() const {
    return lastError_;
  }

 private:
  std::string lastError_;

  // Helper parsing methods
  bool parseNodes(const std::string& content, std::unordered_map<uint64_t, Point2D>& nodes);
  bool parseLanelets(const std::string& content, const std::unordered_map<uint64_t, Point2D>& nodes,
                     MapServer& mapServer);
  bool parseRegulatoryElements(const std::string& content, MapServer& mapServer);
};

}  // namespace hdmap
