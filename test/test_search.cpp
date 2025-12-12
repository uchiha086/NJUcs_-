#include "../code/code.cpp"
#include <gtest/gtest.h>

struct SearchFixture : public ::testing::Test {
  void SetUp() override {
    Transaction::getRepository().clear();
    // 保证分类存在
    Category::addCategory(Category("tc1", "餐饮"));
    Category::addCategory(Category("tc2", "交通"));
  }
  void TearDown() override {
    Transaction::getRepository().clear();
    Category::deleteCategory("tc1");
    Category::deleteCategory("tc2");
  }
};

TEST_F(SearchFixture, EmptyRepository_ReturnsEmpty) {
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({}, {}, "", "");
  ASSERT_TRUE(r.empty());
}

TEST_F(SearchFixture, DateRange_FiltersCorrectly) {
  Transaction a; a.setId("D1"); a.setAmount(10); a.setTime("2021-01-01"); a.setCategoryId("tc1");
  Transaction b; b.setId("D2"); b.setAmount(20); b.setTime("2021-03-01"); b.setCategoryId("tc1");
  Transaction::getRepository().push_back(a); Transaction::getRepository().push_back(b);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"2021-02-01","2021-04-01"}, {0,9999}, "", "");
  ASSERT_EQ(r.size(), 1);
  EXPECT_EQ(r[0].getId(), "D2");
}

TEST_F(SearchFixture, DateRange_SizeNotTwo_Skipped) {
  Transaction t; t.setId("S1"); t.setAmount(5); t.setTime("2020-12-15"); t.setCategoryId("tc1");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  // 传入单元素，函数应跳过日期检查
  auto r = s.searchTransaction({"2020-01-01"}, {0,9999}, "", "");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, AmountRange_SizeNotTwo_Skipped) {
  Transaction t; t.setId("AM1"); t.setAmount(150); t.setTime("2022-01-01"); t.setCategoryId("tc1");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  // 空 amountRange 应跳过金额过滤
  auto r = s.searchTransaction({"",""}, {}, "", "");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, AmountBounds_Inclusive) {
  Transaction a; a.setId("A1"); a.setAmount(100); a.setTime("2022-01-01"); a.setCategoryId("tc1");
  Transaction b; b.setId("A2"); b.setAmount(499); b.setTime("2022-01-02"); b.setCategoryId("tc1");
  Transaction::getRepository().push_back(a); Transaction::getRepository().push_back(b);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {100,499}, "", "");
  ASSERT_EQ(r.size(), 2);
}

TEST_F(SearchFixture, AmountOutside_Excluded) {
  Transaction t; t.setId("A3"); t.setAmount(50); t.setTime("2022-01-01"); t.setCategoryId("tc1");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {100,200}, "", "");
  ASSERT_TRUE(r.empty());
}

TEST_F(SearchFixture, CategoryFilter_Works) {
  Transaction t; t.setId("C1"); t.setAmount(30); t.setTime("2022-03-01"); t.setCategoryId("tc2");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {0,9999}, "tc2", "");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, CategoryFilter_Empty_NoFilter) {
  Transaction t; t.setId("C2"); t.setAmount(30); t.setTime("2022-03-01"); t.setCategoryId("tc2");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {0,9999}, "", "");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, KeywordInId_Matches) {
  Transaction t; t.setId("KW-123"); t.setAmount(1); t.setTime("2022-01-01"); t.setCategoryId("tc1"); t.setRemarks("");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {0,9999}, "", "KW-");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, KeywordInRemarks_Matches) {
  Transaction t; t.setId("R1"); t.setAmount(1); t.setTime("2022-01-01"); t.setCategoryId("tc1"); t.setRemarks("地铁消费");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {0,9999}, "", "地铁");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, KeywordBoth_IdAndRemarks) {
  Transaction t; t.setId("KWORD"); t.setAmount(2); t.setTime("2022-02-02"); t.setCategoryId("tc1"); t.setRemarks("contains KWORD inside");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {0,9999}, "", "KWORD");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, CombinationFilters_AllMatch) {
  Transaction t; t.setId("ALL1"); t.setAmount(250); t.setTime("2022-06-15"); t.setCategoryId("tc2"); t.setRemarks("note");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"2022-06-01","2022-06-30"}, {200,300}, "tc2", "note");
  ASSERT_EQ(r.size(), 1);
}

TEST_F(SearchFixture, NoMatch_ReturnsEmpty) {
  Transaction t; t.setId("NM1"); t.setAmount(10); t.setTime("2020-01-01"); t.setCategoryId("tc1");
  Transaction::getRepository().push_back(t);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"2021-01-01","2021-12-31"}, {0,9999}, "", "");
  ASSERT_TRUE(r.empty());
}

TEST_F(SearchFixture, MultipleMatches_ReturnsAll) {
  Transaction a; a.setId("M1"); a.setAmount(10); a.setTime("2022-01-01"); a.setCategoryId("tc1");
  Transaction b; b.setId("M2"); b.setAmount(20); b.setTime("2022-01-02"); b.setCategoryId("tc1");
  Transaction::getRepository().push_back(a); Transaction::getRepository().push_back(b);
  SearchService s(Transaction::list());
  auto r = s.searchTransaction({"",""}, {0,9999}, "", "");
  ASSERT_EQ(r.size(), 2);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
