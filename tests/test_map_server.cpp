#include <fstream>
#include <gtest/gtest.h>

#include "map_server.hpp"

using namespace hdmap;

class MapServerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a temporary test map file
    createTestMapFile();
  }

  void TearDown() override {
    // Clean up test file
    std::remove(testMapPath.c_str());
  }

  void createTestMapFile() {
    testMapPath = "/tmp/test_map.osm";
    std::ofstream file(testMapPath);
    file << R"(<?xml version="1.0" encoding="UTF-8"?>
<osm version="0.6">
  <node id="1" lat="0.0" lon="0.0"/>
  <node id="2" lat="0.0" lon="100.0"/>
  <node id="3" lat="100.0" lon="0.0"/>
  <node id="4" lat="100.0" lon="100.0"/>
  
  <way id="100">
    <nd ref="1"/>
    <nd ref="2"/>
    <tag k="type" v="lanelet"/>
    <tag k="subtype" v="road"/>
  </way>
  
  <way id="101">
    <nd ref="3"/>
    <nd ref="4"/>
    <tag k="type" v="lanelet"/>
    <tag k="subtype" v="road"/>
  </way>
  
  <relation id="200">
    <tag k="type" v="regulatory_element"/>
    <tag k="subtype" v="traffic_light"/>
    <member type="node" ref="2" role="ref_line"/>
  </relation>
</osm>)";
    file.close();
  }

  std::string testMapPath;
};

TEST_F(MapServerTest, LoadMap) {
  auto server{MapServer::getInstance()};

  EXPECT_TRUE(server->loadFromFile(std::move(testMapPath)));
  EXPECT_GT(server->getLaneCount(), 0);
}

TEST_F(MapServerTest, QueryRegion) {
  auto server{MapServer::getInstance()};
  server->loadFromFile(std::move(testMapPath));

  const BoundingBox region{Point2D(0, 0), Point2D(50, 50)};
  const QueryResult result{server->queryRegion(region)};

  EXPECT_GT(result.lanes.size(), 0);
}

TEST_F(MapServerTest, QueryRadius) {
  auto server{MapServer::getInstance()};
  server->loadFromFile(std::move(testMapPath));

  const Point2D center{50, 50};
  const QueryResult result{server->queryRadius(center, 100.0)};

  EXPECT_GT(result.lanes.size(), 0);
}

TEST_F(MapServerTest, GetLaneById) {
  auto server{MapServer::getInstance()};
  server->loadFromFile(std::move(testMapPath));

  auto lane = server->getLaneById(100);
  EXPECT_TRUE(lane.has_value());

  if (lane.has_value()) {
    EXPECT_EQ((*lane)->id, 100);
    EXPECT_GT((*lane)->centerline.size(), 0);
  }

  auto nonexistent = server->getLaneById(99999);
  EXPECT_FALSE(nonexistent.has_value());
}

TEST_F(MapServerTest, GetClosestLane) {
  auto server{MapServer::getInstance()};
  server->loadFromFile(std::move(testMapPath));

  const Point2D position{10, 10};
  const auto closestLane{server->getClosestLane(position)};

  EXPECT_TRUE(closestLane.has_value());
}

TEST_F(MapServerTest, Clear) {
  auto server{MapServer::getInstance()};

  server->loadFromFile(std::move(testMapPath));

  EXPECT_GT(server->getLaneCount(), 0);

  server->clear();

  EXPECT_EQ(server->getLaneCount(), 0);
  EXPECT_EQ(server->getTrafficLightCount(), 0);
  EXPECT_EQ(server->getTrafficSignCount(), 0);
}

TEST_F(MapServerTest, MemoryUsage) {
  auto server{MapServer::getInstance()};
  server->loadFromFile(testMapPath);

  const auto memUsage{server->getMemoryUsage()};
  EXPECT_GT(memUsage, 0);

  // Should be reasonable for a small test map
  EXPECT_LT(memUsage, 10 * 1024 * 1024);  // Less than 10 MB
}

TEST_F(MapServerTest, InvalidFile) {
  auto server{MapServer::getInstance()};

  EXPECT_FALSE(server->loadFromFile("/nonexistent/path/map.osm"));
  EXPECT_EQ(server->getLaneCount(), 0);
}
