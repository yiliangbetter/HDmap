#include <gtest/gtest.h>
#include <vector>

#include "include/rtree.hpp"

TEST(RTreeTest, InsertAndQuery) {
  hdmap::RTree tree;

  // Insert some bounding boxes with data
  const hdmap::Data data1{}, data2{}, data3{};

  tree.insert(hdmap::BoundingBox(hdmap::Point2D(0, 0), hdmap::Point2D(10, 10)),
              data1);
  tree.insert(
      hdmap::BoundingBox(hdmap::Point2D(20, 20), hdmap::Point2D(30, 30)),
      data2);
  tree.insert(hdmap::BoundingBox(hdmap::Point2D(5, 5), hdmap::Point2D(15, 15)),
              data3);

  EXPECT_EQ(tree.size(), 3);

  // Query overlapping region
  std::vector<hdmap::Data> results;
  tree.query(hdmap::BoundingBox(hdmap::Point2D(0, 0), hdmap::Point2D(10, 10)),
             results);

  EXPECT_GE(results.size(), 2);  // Should find data1 and data3
}

TEST(RTreeTest, QueryRadius) {
  hdmap::RTree tree;

  const hdmap::Data data1{}, data2{}, data3{};

  tree.insert(hdmap::BoundingBox(hdmap::Point2D(0, 0), hdmap::Point2D(2, 2)),
              data1);
  tree.insert(
      hdmap::BoundingBox(hdmap::Point2D(100, 100), hdmap::Point2D(102, 102)),
      data2);
  tree.insert(hdmap::BoundingBox(hdmap::Point2D(8, 8), hdmap::Point2D(10, 10)),
              data3);

  // Query around origin
  std::vector<hdmap::Data> results;
  tree.queryRadius(hdmap::Point2D(5, 5), 10.0, results);

  EXPECT_GE(results.size(), 2);  // Should find data1 and data3, not data2
}

TEST(RTreeTest, EmptyTree) {
  const hdmap::RTree tree{};

  EXPECT_EQ(tree.size(), 0);

  std::vector<hdmap::Data> results;
  tree.query(hdmap::BoundingBox(hdmap::Point2D(0, 0), hdmap::Point2D(100, 100)),
             results);

  EXPECT_EQ(results.size(), 0);
}

TEST(RTreeTest, Clear) {
  hdmap::RTree tree;

  const hdmap::Data data1{}, data2{};
  tree.insert(hdmap::BoundingBox(hdmap::Point2D(0, 0), hdmap::Point2D(10, 10)),
              data1);
  tree.insert(
      hdmap::BoundingBox(hdmap::Point2D(20, 20), hdmap::Point2D(30, 30)),
      data2);

  EXPECT_EQ(tree.size(), 2);

  tree.clear();
  EXPECT_EQ(tree.size(), 0);
}

TEST(RTreeTest, ManyInsertions) {
  hdmap::RTree tree;

  // Insert many elements to test splitting
  std::vector<hdmap::Data> data(100);
  for (int i = 0; i < 100; ++i) {
    data[i] = hdmap::Data{};
    const hdmap::BoundingBox bbox{
        hdmap::Point2D(i * 10.0, i * 10.0),
        hdmap::Point2D(i * 10.0 + 5.0, i * 10.0 + 5.0)};
    tree.insert(bbox, data[i]);
  }

  EXPECT_EQ(tree.size(), 100);
  EXPECT_GT(tree.height(), 1);  // Should have created multiple levels

  // Query should still work
  std::vector<hdmap::Data> results;
  tree.query(hdmap::BoundingBox(hdmap::Point2D(0, 0), hdmap::Point2D(100, 100)),
             results);
  EXPECT_GT(results.size(), 0);
}
