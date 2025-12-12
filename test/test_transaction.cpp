#include "../code/code.cpp"
#include <gtest/gtest.h>

struct TxFixture : public ::testing::Test {
  void SetUp() override {
    Transaction::getRepository().clear();
    // 保证测试需要的分类存在
    Category::addCategory(Category("tc1", "测试1"));
    Category::addCategory(Category("tc2", "测试2"));
  }
  void TearDown() override {
    Transaction::getRepository().clear();
    Category::deleteCategory("tc1");
    Category::deleteCategory("tc2");
  }
};

// 验证函数边界与行为
TEST_F(TxFixture, Validate_NegativeAmount) {
  Transaction t; t.setId("A"); t.setAmount(-0.01);
  EXPECT_FALSE(Transaction::validateTransaction(t));
}

TEST_F(TxFixture, Validate_EmptyId) {
  Transaction t; t.setId(""); t.setAmount(10);
  EXPECT_FALSE(Transaction::validateTransaction(t));
}

TEST_F(TxFixture, Validate_ZeroAmount) {
  Transaction t; t.setId("B"); t.setAmount(0.0);
  EXPECT_TRUE(Transaction::validateTransaction(t));
}

TEST_F(TxFixture, Validate_PositiveAmount) {
  Transaction t; t.setId("C"); t.setAmount(123.45);
  EXPECT_TRUE(Transaction::validateTransaction(t));
}

TEST_F(TxFixture, AddTransaction_ValidAddsToRepository) {
  Transaction::getRepository().clear();
  Transaction t; t.setId("T100"); t.setAmount(50); t.setTime("2022-01-01");
  t.setCategoryId("tc1"); t.setRemarks("ok");
  bool r = Transaction::addTransaction(t);
  EXPECT_TRUE(r);
  EXPECT_EQ(Transaction::getRepository().size(), 1);
  EXPECT_EQ(Transaction::getRepository()[0].getId(), "T100");
}

TEST_F(TxFixture, AddTransaction_InvalidRejected) {
  Transaction::getRepository().clear();
  Transaction t; t.setId(""); t.setAmount(-5);
  bool r = Transaction::addTransaction(t);
  EXPECT_FALSE(r);
  EXPECT_TRUE(Transaction::getRepository().empty());
}

TEST_F(TxFixture, AddTransaction_DuplicateIdRejected) {
  Transaction::getRepository().clear();
  Transaction t1; t1.setId("Dup"); t1.setAmount(1);
  Transaction::addTransaction(t1);
  Transaction t2; t2.setId("Dup"); t2.setAmount(2);
  EXPECT_FALSE(Transaction::addTransaction(t2));
  EXPECT_EQ(Transaction::getRepository().size(), 1);
}

TEST_F(TxFixture, CategoryHasTransactionsTrue) {
  Transaction::getRepository().clear();
  Transaction t; t.setId("X1"); t.setAmount(1); t.setCategoryId("tc2");
  Transaction::getRepository().push_back(t);
  EXPECT_TRUE(Transaction::categoryHasTransactions("tc2"));
}

TEST_F(TxFixture, CategoryHasTransactionsFalse) {
  Transaction::getRepository().clear();
  EXPECT_FALSE(Transaction::categoryHasTransactions("non"));
}

TEST_F(TxFixture, DeleteTransaction_Existing) {
  Transaction::getRepository().clear();
  Transaction t; t.setId("D1"); t.setAmount(10);
  Transaction::getRepository().push_back(t);
  EXPECT_TRUE(Transaction::deleteTransaction("D1"));
  EXPECT_TRUE(Transaction::getRepository().empty());
}

TEST_F(TxFixture, DeleteTransaction_NonExisting) {
  Transaction::getRepository().clear();
  EXPECT_FALSE(Transaction::deleteTransaction("NOPE"));
}

// 额外用例：updateTransaction 替换已存在记录
TEST_F(TxFixture, UpdateTransaction_ReplacesExisting) {
  Transaction::getRepository().clear();
  Transaction t; t.setId("U1"); t.setAmount(10); t.setRemarks("old");
  Transaction::getRepository().push_back(t);
  Transaction newt; newt.setId("U1"); newt.setAmount(99); newt.setRemarks("new");
  EXPECT_TRUE(newt.updateTransaction());
  EXPECT_EQ(Transaction::getRepository()[0].getAmount(), 99);
  EXPECT_EQ(Transaction::getRepository()[0].getRemarks(), "new");
}

TEST_F(TxFixture, UpdateTransaction_NotFound_ReturnsFalse) {
  Transaction::getRepository().clear();
  Transaction t; t.setId("NX"); t.setAmount(1);
  EXPECT_FALSE(t.updateTransaction());
}

TEST_F(TxFixture, GetTransactionDetail_CategoryMissingAndPresent) {
  Transaction::getRepository().clear();
  Transaction t; t.setId("G1"); t.setAmount(5); t.setCategoryId("nope"); t.setRemarks("r");
  std::string d1 = t.getTransactionDetail();
  EXPECT_NE(d1.find("分类="), std::string::npos);
  // 当分类存在
  Category::addCategory(Category("gcat","gname"));
  t.setCategoryId("gcat");
  std::string d2 = t.getTransactionDetail();
  EXPECT_NE(d2.find("gname"), std::string::npos);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
