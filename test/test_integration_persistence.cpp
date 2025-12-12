#include "../code/code.cpp"
#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>

using namespace std::filesystem;

struct PersistenceIntegration : public ::testing::Test {
  std::string tmpFile;
  void SetUp() override {
    Transaction::getRepository().clear();
    // 清理所有分类
    auto cats = Category::getCategoryList();
    for (auto &c : cats) Category::deleteCategory(c.getId());
    // 设置临时文件路径
    tmpFile = "test_bookkeeping.db";
    // 确保不存在旧文件
    if (exists(tmpFile)) remove(tmpFile.c_str());
    DataPersistence::setTestFilePath(tmpFile);
  }
  void TearDown() override {
    Transaction::getRepository().clear();
    auto cats = Category::getCategoryList();
    for (auto &c : cats) Category::deleteCategory(c.getId());
    // 清理临时文件
    if (exists(tmpFile)) remove(tmpFile.c_str());
    DataPersistence::setTestFilePath("");
  }
};

TEST_F(PersistenceIntegration, SaveLoad_RoundTrip) {
  // 添加分类
  Category::addCategory(Category("c100", "餐饮"));
  Category::addCategory(Category("c101", "交通"));

  // 添加多笔交易（包含边界金额、空备注）
  Transaction t1; t1.setId("T1001"); t1.setAmount(0.0); t1.setTime("2022-01-01"); t1.setCategoryId("c100"); t1.setRemarks("");
  Transaction t2; t2.setId("T1002"); t2.setAmount(9999.99); t2.setTime("2022-12-31"); t2.setCategoryId("c101"); t2.setRemarks("note|with|pipe\\and\\backslash\nnewline");
  EXPECT_TRUE(Transaction::addTransaction(t1));
  EXPECT_TRUE(Transaction::addTransaction(t2));

  // 调用持久化保存（此时 testFilePath 已设置，UNIT_TEST 下仍会写入临时文件）
  EXPECT_TRUE(DataPersistence::saveAll());

  // 记录当前数据以便后续比较
  auto beforeCats = Category::getCategoryList();
  auto beforeTxs = Transaction::list();
  EXPECT_EQ(beforeCats.size(), 2);
  EXPECT_EQ(beforeTxs.size(), 2);

  // 清空内存仓库与分类
  Transaction::getRepository().clear();
  auto cs = Category::getCategoryList();
  for (auto &c : cs) Category::deleteCategory(c.getId());
  EXPECT_TRUE(Transaction::getRepository().empty());
  EXPECT_TRUE(Category::getCategoryList().empty());

  // 从文件加载
  EXPECT_TRUE(DataPersistence::loadAll());

  // 验证恢复
  auto afterCats = Category::getCategoryList();
  auto afterTxs = Transaction::list();
  EXPECT_EQ(afterCats.size(), 2);
  EXPECT_EQ(afterTxs.size(), 2);

  // 检查某些字段
  EXPECT_EQ(afterCats[0].getId().size() > 0, true);
  EXPECT_EQ(afterTxs[0].getId().size() > 0, true);
  // 找到具体 id
  bool foundT1001 = false, foundT1002 = false;
  for (auto &tx : afterTxs) {
    if (tx.getId() == "T1001") {
      foundT1001 = true;
      EXPECT_EQ(tx.getAmount(), 0.0);
      EXPECT_EQ(tx.getTime(), "2022-01-01");
    }
    if (tx.getId() == "T1002") {
      foundT1002 = true;
      EXPECT_DOUBLE_EQ(tx.getAmount(), 9999.99);
      EXPECT_EQ(tx.getCategoryId(), "c101");
      EXPECT_NE(tx.getRemarks().find("pipe"), std::string::npos);
    }
  }
  EXPECT_TRUE(foundT1001);
  EXPECT_TRUE(foundT1002);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
