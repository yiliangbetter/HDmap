#include "lanelet2_parser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace hdmap {

bool Lanelet2Parser::parse(const std::string& filepath, MapServer& mapServer) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        lastError_ = "Cannot open file: " + filepath;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
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
        size_t endPos = content.find("/>", pos);
        if (endPos == std::string::npos) break;
        
        std::string nodeStr = content.substr(pos, endPos - pos);
        
        // Extract id
        size_t idPos = nodeStr.find("id=\"");
        if (idPos == std::string::npos) {
            pos = endPos;
            continue;
        }
        idPos += 4;
        size_t idEnd = nodeStr.find("\"", idPos);
        uint64_t id = std::stoull(nodeStr.substr(idPos, idEnd - idPos));
        
        // Extract lat (y)
        size_t latPos = nodeStr.find("lat=\"");
        if (latPos == std::string::npos) {
            pos = endPos;
            continue;
        }
        latPos += 5;
        size_t latEnd = nodeStr.find("\"", latPos);
        double lat = std::stod(nodeStr.substr(latPos, latEnd - latPos));
        
        // Extract lon (x)
        size_t lonPos = nodeStr.find("lon=\"");
        if (lonPos == std::string::npos) {
            pos = endPos;
            continue;
        }
        lonPos += 5;
        size_t lonEnd = nodeStr.find("\"", lonPos);
        double lon = std::stod(nodeStr.substr(lonPos, lonEnd - lonPos));
        
        nodes[id] = Point2D(lon, lat);
        pos = endPos;
    }
    
    return !nodes.empty();
}

bool Lanelet2Parser::parseLanelets(const std::string& content,
                                   const std::unordered_map<uint64_t, Point2D>& nodes,
                                   MapServer& mapServer) {
    // Simplified lanelet parsing
    // Format: <way id="X" ...> with member refs to nodes
    
    size_t pos = 0;
    while ((pos = content.find("<way ", pos)) != std::string::npos) {
        size_t endPos = content.find("</way>", pos);
        if (endPos == std::string::npos) break;
        
        std::string wayStr = content.substr(pos, endPos - pos);
        
        // Extract id
        size_t idPos = wayStr.find("id=\"");
        if (idPos == std::string::npos) {
            pos = endPos;
            continue;
        }
        idPos += 4;
        size_t idEnd = wayStr.find("\"", idPos);
        uint64_t wayId = std::stoull(wayStr.substr(idPos, idEnd - idPos));
        
        // Check if this is a centerline (has subtype tag)
        bool isCenterline = wayStr.find("subtype") != std::string::npos;
        
        if (isCenterline) {
            auto lane = std::make_shared<Lane>();
            lane->id_ = wayId;
            lane->type_ = LaneType::DRIVING;
            lane->speedLimit_ = 13.89;  // 50 km/h default
            
            // Extract node references
            size_t ndPos = 0;
            while ((ndPos = wayStr.find("<nd ref=\"", ndPos)) != std::string::npos) {
                ndPos += 9;
                size_t ndEnd = wayStr.find("\"", ndPos);
                uint64_t nodeId = std::stoull(wayStr.substr(ndPos, ndEnd - ndPos));
                
                auto it = nodes.find(nodeId);
                if (it != nodes.end()) {
                    lane->centerline_.push_back(it->second);
                }
                ndPos = ndEnd;
            }
            
            if (!lane->centerline_.empty()) {
                mapServer.getLanes()[lane->id_] = lane;
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
        size_t endPos = content.find("</relation>", pos);
        if (endPos == std::string::npos) break;
        
        std::string relStr = content.substr(pos, endPos - pos);
        
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
        size_t idEnd = relStr.find("\"", idPos);
        uint64_t relId = std::stoull(relStr.substr(idPos, idEnd - idPos));
        
        // Check subtype
        if (relStr.find("subtype=\"traffic_light\"") != std::string::npos) {
            auto light = std::make_shared<TrafficLight>();
            light->id_ = relId;
            light->position_ = Point2D(0, 0);  // Would extract from refers_to
            light->state_ = TrafficLightState::UNKNOWN;
            light->height_ = 5.0;
            mapServer.getTrafficLights()[light->id_] = light;
        } else if (relStr.find("subtype=\"traffic_sign\"") != std::string::npos) {
            auto sign = std::make_shared<TrafficSign>();
            sign->id_ = relId;
            sign->position_ = Point2D(0, 0);
            sign->type_ = TrafficSignType::OTHER;
            sign->height_ = 3.0;
            mapServer.getTrafficSigns()[sign->id_] = sign;
        }
        
        pos = endPos;
    }
    
    return true;
}

} // namespace hdmap
