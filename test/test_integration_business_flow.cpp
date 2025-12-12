#include "../code/code.cpp"
#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>

using namespace std::filesystem;

struct BusinessFlowIntegration : public ::testing::Test {
  std::string tmpFile;
  void SetUp() override {
    Transaction::getRepository().clear();
    auto cats = Category::getCategoryList();
    for (auto &c : cats) Category::deleteCategory(c.getId());
    tmpFile = "test_bookkeeping_flow.db";
    if (exists(tmpFile)) remove(tmpFile.c_str());
    DataPersistence::setTestFilePath(tmpFile);
    // 准备初始分类
    Category::addCategory(Category("c200", "默认"));
  }
  void TearDown() override {
    Transaction::getRepository().clear();
    auto cats = Category::getCategoryList();
    for (auto &c : cats) Category::deleteCategory(c.getId());
    if (exists(tmpFile)) remove(tmpFile.c_str());
    DataPersistence::setTestFilePath("");
  }
};

TEST_F(BusinessFlowIntegration, AddSearchSortEditDeleteFlow) {
  // 1) 添加交易（模拟高层流程生成 id 并填充字段）
  std::string id = generateTransactionId();
  Transaction tx;
  tx.setId(id);
  tx.setAmount(123.45);
  tx.setTime("2025-01-01");
  tx.setCategoryId("c200");
  tx.setRemarks("flow test");
  tx.setType(TransactionType::Expense);
  EXPECT_TRUE(Transaction::addTransaction(tx));

  // 2) 检索（SearchService）并断言能找到该交易
  SearchService search(Transaction::list());
  auto results = search.searchTransaction({"2024-01-01", "2026-01-01"}, {0.0, 10000.0}, "c200", "flow");
  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].getId(), id);

  // 3) 排序：添加第二笔交易以验证排序效果
  Transaction tx2;
  tx2.setId(generateTransactionId());
  tx2.setAmount(10.0);
  tx2.setTime("2025-01-02");
  tx2.setCategoryId("c200");
  tx2.setRemarks("a");
  Transaction::addTransaction(tx2);
  auto all = Transaction::list();
  auto sorted = UIService::sortTransactionList(all, SortField::Amount, SortDirection::Asc);
  ASSERT_TRUE(sorted.size() >= 2);
  EXPECT_LE(sorted[0].getAmount(), sorted[1].getAmount());

  // 4) 编辑并保存：构造同 id 的新对象调用 updateTransaction()
  Transaction edited = tx;
  edited.setAmount(200.0);
  edited.setRemarks("edited");
  EXPECT_TRUE(edited.updateTransaction());
  EXPECT_TRUE(DataPersistence::saveAll()); // 模拟高层保存

  // 验证仓库中的记录已更新
  bool found = false;
  for (auto &r : Transaction::list()) {
    if (r.getId() == id) {
      found = true;
      EXPECT_EQ(r.getAmount(), 200.0);
      EXPECT_EQ(r.getRemarks(), "edited");
    }
  }
  EXPECT_TRUE(found);

  // 5) 删除并确认不存在
  EXPECT_TRUE(Transaction::deleteTransaction(id));
  bool still = false;
  for (auto &r : Transaction::list()) if (r.getId() == id) still = true;
  EXPECT_FALSE(still);

  // 边界断言：金额为0 的处理
  Transaction z;
  z.setId(generateTransactionId());
  z.setAmount(0.0);
  z.setTime("2025-02-02");
  z.setCategoryId("c200");
  EXPECT_TRUE(Transaction::addTransaction(z));
  EXPECT_TRUE(Transaction::deleteTransaction(z.getId()));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
