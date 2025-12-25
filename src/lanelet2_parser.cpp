#include "lanelet2_parser.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace hdmap {

bool Lanelet2Parser::parse(const std::string& filepath, MapServer& mapServer) {
  const std::ifstream file{filepath};
  if (!file.is_open()) {
    lastError_ = "Cannot open file: " + filepath;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string content{buffer.str()};

  // Parse nodes (points)
  std::unordered_map<uint64_t, Point2D> nodes;
  if (!parseNodes(content, nodes)) {
    return false;
  }

  // Parse lanelets (lanes)
  if (!parseLanelets(content, nodes, mapServer)) {
    return false;
  }

  // Parse regulatory elements (traffic lights, signs)
  if (!parseRegulatoryElements(content, mapServer)) {
    return false;
  }

  return true;
}

bool Lanelet2Parser::parseNodes(const std::string& content, std::unordered_map<uint64_t, Point2D>& nodes) {
  // Simplified parser - looks for node tags
  // Format: <node id="X" lat="Y" lon="Z"/>

  size_t pos = 0;
  while ((pos = content.find("<node ", pos)) != std::string::npos) {
    const size_t endPos{content.find("/>", pos)};
    if (endPos == std::string::npos) break;

    const std::string nodeStr{content.substr(pos, endPos - pos)};

    // Extract id
    size_t idPos = nodeStr.find("id=\"");
    if (idPos == std::string::npos) {
      pos = endPos;
      continue;
    }
    idPos += 4;
    const size_t idEnd{nodeStr.find("\"", idPos)};
    const uint64_t id{std::stoull(nodeStr.substr(idPos, idEnd - idPos))};

    // Extract lat (y)
    size_t latPos = nodeStr.find("lat=\"");
    if (latPos == std::string::npos) {
      pos = endPos;
      continue;
    }
    latPos += 5;
    const size_t latEnd{nodeStr.find("\"", latPos)};
    const double lat{std::stod(nodeStr.substr(latPos, latEnd - latPos))};

    // Extract lon (x)
    size_t lonPos = nodeStr.find("lon=\"");
    if (lonPos == std::string::npos) {
      pos = endPos;
      continue;
    }
    lonPos += 5;
    const size_t lonEnd{nodeStr.find("\"", lonPos)};
    const double lon{std::stod(nodeStr.substr(lonPos, lonEnd - lonPos))};

    nodes[id] = Point2D(lon, lat);
    pos = endPos;
  }

  return !nodes.empty();
}

bool Lanelet2Parser::parseLanelets(const std::string& content, const std::unordered_map<uint64_t, Point2D>& nodes,
                                   MapServer& mapServer) {
  // Simplified lanelet parsing
  // Format: <way id="X" ...> with member refs to nodes

  size_t pos = 0;
  while ((pos = content.find("<way ", pos)) != std::string::npos) {
    const size_t endPos{content.find("</way>", pos)};
    if (endPos == std::string::npos) break;

    const std::string wayStr{content.substr(pos, endPos - pos)};

    // Extract id
    size_t idPos = wayStr.find("id=\"");
    if (idPos == std::string::npos) {
      pos = endPos;
      continue;
    }
    idPos += 4;
    const size_t idEnd{wayStr.find("\"", idPos)};
    const uint64_t wayId{std::stoull(wayStr.substr(idPos, idEnd - idPos))};

    // Check if this is a centerline (has subtype tag)
    const bool isCenterline{wayStr.find("subtype") != std::string::npos};

    if (isCenterline) {
      auto lane = std::make_shared<Lane>();
      lane->id = wayId;
      lane->type = LaneType::DRIVING;
      lane->speedLimit = 13.89;  // 50 km/h default

      // Extract node references
      size_t ndPos = 0;
      while ((ndPos = wayStr.find("<nd ref=\"", ndPos)) != std::string::npos) {
        ndPos += 9;
        const size_t ndEnd{wayStr.find("\"", ndPos)};
        const uint64_t nodeId{std::stoull(wayStr.substr(ndPos, ndEnd - ndPos))};

        auto it = nodes.find(nodeId);
        if (it != nodes.end()) {
          lane->centerline.push_back(it->second);
        }
        ndPos = ndEnd;
      }

      if (!lane->centerline.empty()) {
        mapServer.getLanesMutable()[lane->id] = lane;
      }
    }

    pos = endPos;
  }

  return true;
}

bool Lanelet2Parser::parseRegulatoryElements(const std::string& content, MapServer& mapServer) {
  // Simplified parsing of traffic lights and signs
  // Format: <relation id="X" ...> with type="regulatory_element"

  size_t pos = 0;
  while ((pos = content.find("<relation ", pos)) != std::string::npos) {
    const size_t endPos{content.find("</relation>", pos)};
    if (endPos == std::string::npos) break;

    const std::string relStr{content.substr(pos, endPos - pos)};

    // Check if regulatory element
    if (relStr.find("type=\"regulatory_element\"") == std::string::npos) {
      pos = endPos;
      continue;
    }

    // Extract id
    size_t idPos = relStr.find("id=\"");
    if (idPos == std::string::npos) {
      pos = endPos;
      continue;
    }
    idPos += 4;
    const size_t idEnd{relStr.find("\"", idPos)};
    const uint64_t relId{std::stoull(relStr.substr(idPos, idEnd - idPos))};

    // Check subtype
    if (relStr.find("subtype=\"traffic_light\"") != std::string::npos) {
      auto light = std::make_shared<TrafficLight>();
      light->id = relId;
      light->position = Point2D(0, 0);  // Would extract from refers_to
      light->state = TrafficLightState::UNKNOWN;
      light->height = 5.0;
      mapServer.getTrafficLightsMutable()[light->id] = light;
    } else if (relStr.find("subtype=\"traffic_sign\"") != std::string::npos) {
      auto sign = std::make_shared<TrafficSign>();
      sign->id = relId;
      sign->position = Point2D(0, 0);
      sign->type = TrafficSignType::OTHER;
      sign->height = 3.0;
      mapServer.getTrafficSignsMutable()[sign->id] = sign;
    }

    pos = endPos;
  }

  return true;
}

}  // namespace hdmap
