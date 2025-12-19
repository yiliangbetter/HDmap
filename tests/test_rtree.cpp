#include <gtest/gtest.h>

#include "rtree.hpp"

using namespace hdmap;

TEST(RTreeTest, InsertAndQuery) {
  RTree tree;

  // Insert some bounding boxes with data
  auto data1{std::make_shared<Object>(1)}, data2{std::make_shared<Object>(2)};
  auto data3{std::make_shared<Object>(3)};

  tree.insert(BoundingBox(Point2D(0, 0), Point2D(10, 10)), data1);
  tree.insert(BoundingBox(Point2D(20, 20), Point2D(30, 30)), data2);
  tree.insert(BoundingBox(Point2D(5, 5), Point2D(15, 15)), data3);

  EXPECT_EQ(tree.size(), 3);

  // Query overlapping region
  std::vector<std::shared_ptr<Object>> results;
  tree.query(BoundingBox(Point2D(0, 0), Point2D(10, 10)), results);

  EXPECT_GE(results.size(), 2);  // Should find data1 and data3
}

TEST(RTreeTest, QueryRadius) {
  RTree tree;

  auto data1{std::make_shared<Object>(1)}, data2{std::make_shared<Object>(2)};
  auto data3{std::make_shared<Object>(3)};

  tree.insert(BoundingBox(Point2D(0, 0), Point2D(2, 2)), data1);
  tree.insert(BoundingBox(Point2D(100, 100), Point2D(102, 102)), data2);
  tree.insert(BoundingBox(Point2D(8, 8), Point2D(10, 10)), data3);

  // Query around origin
  std::vector<std::shared_ptr<Object>> results;
  tree.queryRadius(Point2D(5, 5), 10.0, results);

  EXPECT_GE(results.size(), 2);  // Should find data1 and data3, not data2
}

TEST(RTreeTest, EmptyTree) {
  RTree tree;

  EXPECT_EQ(tree.size(), 0);

  std::vector<std::shared_ptr<Object>> results;
  tree.query(BoundingBox(Point2D(0, 0), Point2D(100, 100)), results);

  EXPECT_EQ(results.size(), 0);
}

TEST(RTreeTest, Clear) {
  RTree tree;

  auto data1{std::make_shared<Object>(1)}, data2{std::make_shared<Object>(2)};
  tree.insert(BoundingBox(Point2D(0, 0), Point2D(10, 10)), data1);
  tree.insert(BoundingBox(Point2D(20, 20), Point2D(30, 30)), data2);

  EXPECT_EQ(tree.size(), 2);

  tree.clear();
  EXPECT_EQ(tree.size(), 0);
}

TEST(RTreeTest, ManyInsertions) {
  RTree tree;

  // Insert many elements to test splitting
  std::vector<std::shared_ptr<Object>> data(100);
  for (int i = 0; i < 100; ++i) {
    data[i] = std::make_shared<Object>(1);
    BoundingBox bbox(Point2D(i * 10.0, i * 10.0),
                     Point2D(i * 10.0 + 5.0, i * 10.0 + 5.0));
    tree.insert(bbox, data[i]);
  }

  EXPECT_EQ(tree.size(), 100);
  EXPECT_GT(tree.height(), 1);  // Should have created multiple levels

  // Query should still work
  std::vector<std::shared_ptr<Object>> results;
  tree.query(BoundingBox(Point2D(0, 0), Point2D(100, 100)), results);
  EXPECT_GT(results.size(), 0);
}
