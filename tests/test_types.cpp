#include <gtest/gtest.h>
#include <memory>

#include "include/types.hpp"

TEST(Point2DTest, Distance) {
  const hdmap::Point2D p1{0, 0};
  const hdmap::Point2D p2{3, 4};

  EXPECT_DOUBLE_EQ(p1.distanceTo(p2), 5.0);
  EXPECT_DOUBLE_EQ(p2.distanceTo(p1), 5.0);
}

TEST(Point2DTest, SamePoint) {
  const hdmap::Point2D p1{10, 20};
  const hdmap::Point2D p2{10, 20};

  EXPECT_DOUBLE_EQ(p1.distanceTo(p2), 0.0);
}

TEST(BoundingBoxTest, Contains) {
  const hdmap::BoundingBox bbox{hdmap::Point2D(0, 0), hdmap::Point2D(10, 10)};

  EXPECT_TRUE(bbox.contains(hdmap::Point2D(5, 5)));
  EXPECT_TRUE(bbox.contains(hdmap::Point2D(0, 0)));
  EXPECT_TRUE(bbox.contains(hdmap::Point2D(10, 10)));
  EXPECT_FALSE(bbox.contains(hdmap::Point2D(-1, 5)));
  EXPECT_FALSE(bbox.contains(hdmap::Point2D(5, 11)));
}

TEST(BoundingBoxTest, Intersects) {
  const hdmap::BoundingBox bbox1{hdmap::Point2D(0, 0), hdmap::Point2D(10, 10)};
  const hdmap::BoundingBox bbox2{hdmap::Point2D(5, 5), hdmap::Point2D(15, 15)};
  const hdmap::BoundingBox bbox3{hdmap::Point2D(20, 20),
                                 hdmap::Point2D(30, 30)};

  EXPECT_TRUE(bbox1.intersects(bbox2));
  EXPECT_TRUE(bbox2.intersects(bbox1));
  EXPECT_FALSE(bbox1.intersects(bbox3));
  EXPECT_FALSE(bbox3.intersects(bbox1));
}

TEST(BoundingBoxTest, Area) {
  const hdmap::BoundingBox bbox{hdmap::Point2D(0, 0), hdmap::Point2D(10, 20)};
  EXPECT_DOUBLE_EQ(bbox.area(), 200.0);
}

TEST(BoundingBoxTest, Center) {
  const hdmap::BoundingBox bbox{hdmap::Point2D(0, 0), hdmap::Point2D(10, 20)};
  const auto center{bbox.center()};

  EXPECT_DOUBLE_EQ(center.x, 5.0);
  EXPECT_DOUBLE_EQ(center.y, 10.0);
}

TEST(LaneTest, ComputeBoundingBox) {
  hdmap::Lane lane{};
  lane.centerline = {hdmap::Point2D(0, 0), hdmap::Point2D(10, 10),
                     hdmap::Point2D(20, 5)};
  lane.leftBoundary = {hdmap::Point2D(-1, 1), hdmap::Point2D(9, 11),
                       hdmap::Point2D(19, 6)};
  lane.rightBoundary = {hdmap::Point2D(1, -1), hdmap::Point2D(11, 9),
                        hdmap::Point2D(21, 4)};

  lane.computeBoundingBox();

  EXPECT_DOUBLE_EQ(lane.bbox.min.x, -1.0);
  EXPECT_DOUBLE_EQ(lane.bbox.max.x, 21.0);
  EXPECT_DOUBLE_EQ(lane.bbox.min.y, -1.0);
  EXPECT_DOUBLE_EQ(lane.bbox.max.y, 11.0);
}

TEST(QueryResultTest, TotalCount) {
  hdmap::QueryResult result{};

  const hdmap::Lane lane1{}, lane2{};
  const hdmap::TrafficLight light1{};
  const hdmap::TrafficSign sign1{}, sign2{}, sign3{};
  result.lanes = {std::make_shared<hdmap::Lane>(lane1),
                  std::make_shared<hdmap::Lane>(lane2)};
  result.trafficLights = {std::make_shared<hdmap::TrafficLight>(light1)};
  result.trafficSigns = {std::make_shared<hdmap::TrafficSign>(sign1),
                         std::make_shared<hdmap::TrafficSign>(sign2),
                         std::make_shared<hdmap::TrafficSign>(sign3)};

  EXPECT_EQ(result.totalCount(), 6);
}
