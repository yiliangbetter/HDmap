#include <gtest/gtest.h>

#include "types.hpp"

using namespace hdmap;

TEST(Point2DTest, Distance) {
  const Point2D p1{0, 0};
  const Point2D p2{3, 4};

  EXPECT_DOUBLE_EQ(p1.distanceTo(p2), 5.0);
  EXPECT_DOUBLE_EQ(p2.distanceTo(p1), 5.0);
}

TEST(Point2DTest, SamePoint) {
  const Point2D p1{10, 20};
  const Point2D p2{10, 20};

  EXPECT_DOUBLE_EQ(p1.distanceTo(p2), 0.0);
}

TEST(BoundingBoxTest, Contains) {
  const BoundingBox bbox{Point2D(0, 0), Point2D(10, 10)};

  EXPECT_TRUE(bbox.contains(Point2D(5, 5)));
  EXPECT_TRUE(bbox.contains(Point2D(0, 0)));
  EXPECT_TRUE(bbox.contains(Point2D(10, 10)));
  EXPECT_FALSE(bbox.contains(Point2D(-1, 5)));
  EXPECT_FALSE(bbox.contains(Point2D(5, 11)));
}

TEST(BoundingBoxTest, Intersects) {
  const BoundingBox bbox1{Point2D(0, 0), Point2D(10, 10)};
  const BoundingBox bbox2{Point2D(5, 5), Point2D(15, 15)};
  const BoundingBox bbox3{Point2D(20, 20), Point2D(30, 30)};

  EXPECT_TRUE(bbox1.intersects(bbox2));
  EXPECT_TRUE(bbox2.intersects(bbox1));
  EXPECT_FALSE(bbox1.intersects(bbox3));
  EXPECT_FALSE(bbox3.intersects(bbox1));
}

TEST(BoundingBoxTest, Area) {
  const BoundingBox bbox{Point2D(0, 0), Point2D(10, 20)};
  EXPECT_DOUBLE_EQ(bbox.area(), 200.0);
}

TEST(BoundingBoxTest, Center) {
  const BoundingBox bbox{Point2D(0, 0), Point2D(10, 20)};
  const auto center{bbox.center()};

  EXPECT_DOUBLE_EQ(center.x, 5.0);
  EXPECT_DOUBLE_EQ(center.y, 10.0);
}

TEST(LaneTest, ComputeBoundingBox) {
  Lane lane{};
  lane.centerline = {Point2D(0, 0), Point2D(10, 10), Point2D(20, 5)};
  lane.leftBoundary = {Point2D(-1, 1), Point2D(9, 11), Point2D(19, 6)};
  lane.rightBoundary = {Point2D(1, -1), Point2D(11, 9), Point2D(21, 4)};

  lane.computeBoundingBox();

  EXPECT_DOUBLE_EQ(lane.bbox.min.x, -1.0);
  EXPECT_DOUBLE_EQ(lane.bbox.max.x, 21.0);
  EXPECT_DOUBLE_EQ(lane.bbox.min.y, -1.0);
  EXPECT_DOUBLE_EQ(lane.bbox.max.y, 11.0);
}

TEST(QueryResultTest, TotalCount) {
  QueryResult result{};

  const Lane lane1{}, lane2{};
  const TrafficLight light1{};
  const TrafficSign sign1{}, sign2{}, sign3{};

  result.lanes = {std::make_shared<Lane>(lane1), std::make_shared<Lane>(lane2)};
  result.trafficLights = {std::make_shared<TrafficLight>(light1)};
  result.trafficSigns = {std::make_shared<TrafficSign>(sign1), std::make_shared<TrafficSign>(sign2),
                         std::make_shared<TrafficSign>(sign3)};

  EXPECT_EQ(result.totalCount(), 6);
}
