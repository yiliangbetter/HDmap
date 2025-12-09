#include <gtest/gtest.h>
#include "types.hpp"

using namespace hdmap;

TEST(Point2DTest, Distance) {
    Point2D p1(0, 0);
    Point2D p2(3, 4);
    
    EXPECT_DOUBLE_EQ(p1.distanceTo(p2), 5.0);
    EXPECT_DOUBLE_EQ(p2.distanceTo(p1), 5.0);
}

TEST(Point2DTest, SamePoint) {
    Point2D p1(10, 20);
    Point2D p2(10, 20);
    
    EXPECT_DOUBLE_EQ(p1.distanceTo(p2), 0.0);
}

TEST(BoundingBoxTest, Contains) {
    BoundingBox bbox(Point2D(0, 0), Point2D(10, 10));
    
    EXPECT_TRUE(bbox.contains(Point2D(5, 5)));
    EXPECT_TRUE(bbox.contains(Point2D(0, 0)));
    EXPECT_TRUE(bbox.contains(Point2D(10, 10)));
    EXPECT_FALSE(bbox.contains(Point2D(-1, 5)));
    EXPECT_FALSE(bbox.contains(Point2D(5, 11)));
}

TEST(BoundingBoxTest, Intersects) {
    BoundingBox bbox1(Point2D(0, 0), Point2D(10, 10));
    BoundingBox bbox2(Point2D(5, 5), Point2D(15, 15));
    BoundingBox bbox3(Point2D(20, 20), Point2D(30, 30));
    
    EXPECT_TRUE(bbox1.intersects(bbox2));
    EXPECT_TRUE(bbox2.intersects(bbox1));
    EXPECT_FALSE(bbox1.intersects(bbox3));
    EXPECT_FALSE(bbox3.intersects(bbox1));
}

TEST(BoundingBoxTest, Area) {
    BoundingBox bbox(Point2D(0, 0), Point2D(10, 20));
    EXPECT_DOUBLE_EQ(bbox.area(), 200.0);
}

TEST(BoundingBoxTest, Center) {
    BoundingBox bbox(Point2D(0, 0), Point2D(10, 20));
    Point2D center = bbox.center();
    
    EXPECT_DOUBLE_EQ(center.x_, 5.0);
    EXPECT_DOUBLE_EQ(center.y_, 10.0);
}

TEST(LaneTest, ComputeBoundingBox) {
    Lane lane;
    lane.centerline_ = {Point2D(0, 0), Point2D(10, 10), Point2D(20, 5)};
    lane.leftBoundary_ = {Point2D(-1, 1), Point2D(9, 11), Point2D(19, 6)};
    lane.rightBoundary_ = {Point2D(1, -1), Point2D(11, 9), Point2D(21, 4)};
    
    lane.computeBoundingBox();
    
    EXPECT_DOUBLE_EQ(lane.bbox_.min_.x_, -1.0);
    EXPECT_DOUBLE_EQ(lane.bbox_.max_.x_, 21.0);
    EXPECT_DOUBLE_EQ(lane.bbox_.min_.y_, -1.0);
    EXPECT_DOUBLE_EQ(lane.bbox_.max_.y_, 11.0);
}

TEST(QueryResultTest, TotalCount) {
    QueryResult result;
    
    Lane lane1, lane2;
    TrafficLight light1;
    TrafficSign sign1, sign2, sign3;
    
    result.lanes_ = {&lane1, &lane2};
    result.trafficLights_ = {&light1};
    result.trafficSigns_ = {&sign1, &sign2, &sign3};
    
    EXPECT_EQ(result.totalCount(), 6);
}
