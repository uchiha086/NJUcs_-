// Copyright 2025 user
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
// 在非 Windows 平台上提供空实现以避免编译错误
inline void SetConsoleOutputCP(unsigned int) {}
inline void SetConsoleCP(unsigned int) {}
#endif

enum class TransactionType { Income, Expense };
enum class TimeGroup { Daily, Monthly, Yearly };
enum class SortField { Amount, Time };
enum class SortDirection { Asc, Desc };

class DataPersistence;

// 分类类
class Category {
 public:
  Category(std::string id, std::string name)
      : categoryId(id), categoryName(name) {}

  static std::vector<Category> getCategoryList() { return categories; }

  static bool addCategory(const Category& category) {
    if (category.categoryId.empty()) return false;
    for (size_t i = 0; i < categories.size(); ++i) {
      if (categories[i].categoryId == category.categoryId) return false;
    }
    categories.push_back(category);
    return true;
  }

  static bool deleteCategory(const std::string& id) {
    for (size_t i = 0; i < categories.size(); ++i) {
      if (categories[i].categoryId == id) {
        categories.erase(
            categories.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
      }
    }
    return false;
  }

  static bool categoryExists(const std::string& id) {
    for (size_t i = 0; i < categories.size(); ++i) {
      if (categories[i].categoryId == id) return true;
    }
    return false;
  }

  static Category* findCategory(const std::string& id) {
    for (size_t i = 0; i < categories.size(); ++i) {
      if (categories[i].categoryId == id) return &categories[i];
    }
    return nullptr;
  }

  const std::string& getId() const { return categoryId; }
  const std::string& getName() const { return categoryName; }

 private:
  std::string categoryId;
  std::string categoryName;
  static std::vector<Category> categories;
};

// 交易类
class Transaction {
 public:
  Transaction()
      : transactionId(""), amount(0.0), transactionTime(""),
        categoryId(""), remarks(""),
        transactionType(TransactionType::Expense) {}

  static bool addTransaction(const Transaction& t);
  static bool deleteTransaction(const std::string& id);

  static bool validateTransaction(const Transaction& t) {
    return !t.transactionId.empty() && t.amount >= 0.0;
  }

  static bool categoryHasTransactions(const std::string& cId) {
    for (size_t i = 0; i < repository.size(); ++i) {
      if (repository[i].categoryId == cId) return true;
    }
    return false;
  }

  std::string getTransactionDetail() const {
    std::string detail = "交易[" + transactionId + "] 金额=" +
                        std::to_string(amount) + ", 时间=" + transactionTime +
                        ", 分类=";
    Category* cat = Category::findCategory(categoryId);
    if (cat) {
      detail += cat->getName() + "(" + categoryId + ")";
    } else {
      detail += categoryId;
    }
    detail += ", 类型=" + std::string(transactionType ==
              TransactionType::Income ? "收入" : "支出") +
              ", 备注=" + remarks;
    return detail;
  }

  bool updateTransaction() {
    for (size_t i = 0; i < repository.size(); ++i) {
      if (repository[i].transactionId == transactionId) {
        repository[i] = *this;
        return true;
      }
    }
    return false;
  }

  void setId(const std::string& id) { transactionId = id; }
  void setAmount(double amt) { amount = amt; }
  void setTime(const std::string& time) { transactionTime = time; }
  void setCategoryId(const std::string& cId) { categoryId = cId; }
  void setRemarks(const std::string& rem) { remarks = rem; }
  void setType(TransactionType type) { transactionType = type; }

  const std::string& getId() const { return transactionId; }
  double getAmount() const { return amount; }
  const std::string& getTime() const { return transactionTime; }
  const std::string& getCategoryId() const { return categoryId; }
  const std::string& getRemarks() const { return remarks; }
  TransactionType getType() const { return transactionType; }

  static const std::vector<Transaction>& list() { return repository; }
  static std::vector<Transaction>& getRepository() { return repository; }

 private:
  std::string transactionId;
  double amount;
  std::string transactionTime;
  std::string categoryId;
  std::string remarks;
  TransactionType transactionType;
  static std::vector<Transaction> repository;
};

// 统计服务
class StatisticService {
 public:
  explicit StatisticService(const std::vector<Transaction>& data)
      : transactions(data) {}

  void calculateAndDisplayByType() {
    double totalIncome = 0.0, totalExpense = 0.0;
    for (size_t i = 0; i < transactions.size(); ++i) {
      if (transactions[i].getType() == TransactionType::Income) {
        totalIncome += transactions[i].getAmount();
      } else {
        totalExpense += transactions[i].getAmount();
      }
    }
    displaySummary(totalIncome, totalExpense);
  }

  void calculateAndDisplayByTime(TimeGroup group,
                                 const std::vector<std::string>& range) {
    double totalIncome = 0.0, totalExpense = 0.0;
    std::vector<std::pair<std::string,
                std::pair<double, double>>> timeCounts;

    for (size_t i = 0; i < transactions.size(); ++i) {
      const Transaction& tx = transactions[i];
      if (!inDateRange(tx.getTime(), range)) continue;

      if (tx.getType() == TransactionType::Income) {
        totalIncome += tx.getAmount();
      } else {
        totalExpense += tx.getAmount();
      }

      std::string key = getTimeKey(tx.getTime(), group);
      updateTimeCounts(timeCounts, key, tx);
    }

    displaySummary(totalIncome, totalExpense);
    if (!timeCounts.empty()) {
      std::cout << "\n时间段统计：\n时间\t收入\t支出\t结余\n"
                << "--------------------------------------\n";
      for (size_t i = 0; i < timeCounts.size(); ++i) {
        double bal = timeCounts[i].second.first - timeCounts[i].second.second;
        std::cout << timeCounts[i].first << '\t'
                  << timeCounts[i].second.first << '\t'
                  << timeCounts[i].second.second << '\t' << bal << '\n';
      }
    }
  }

  void calculateAndDisplayByCategory() {
    double totalIncome = 0.0, totalExpense = 0.0;
    std::vector<std::pair<std::string,
                std::pair<double, double>>> categoryAmounts;

    for (size_t i = 0; i < transactions.size(); ++i) {
      const Transaction& tx = transactions[i];
      if (tx.getType() == TransactionType::Income) {
        totalIncome += tx.getAmount();
      } else {
        totalExpense += tx.getAmount();
      }

      Category* cat = Category::findCategory(tx.getCategoryId());
      std::string categoryName = cat ? cat->getName() : tx.getCategoryId();
      updateAmounts(categoryAmounts, categoryName, tx);
    }

    displaySummary(totalIncome, totalExpense);
    if (!categoryAmounts.empty()) {
      std::cout << "\n分类统计：\n分类\t收入\t支出\t净额\n"
                << "--------------------------------------\n";
      for (size_t i = 0; i < categoryAmounts.size(); ++i) {
        double net = categoryAmounts[i].second.first -
                    categoryAmounts[i].second.second;
        std::cout << categoryAmounts[i].first << '\t'
                  << categoryAmounts[i].second.first << '\t'
                  << categoryAmounts[i].second.second << '\t' << net << '\n';
      }
    }
  }

  void calculateAndDisplayByAmountRange() {
    double totalIncome = 0.0, totalExpense = 0.0;
    std::vector<std::pair<std::string, std::pair<int, int>>> rangeCounts;

    for (size_t i = 0; i < transactions.size(); ++i) {
      const Transaction& tx = transactions[i];
      if (tx.getType() == TransactionType::Income) {
        totalIncome += tx.getAmount();
      } else {
        totalExpense += tx.getAmount();
      }

      std::string rangeKey = getAmountRange(tx.getAmount());
      updateRangeCounts(rangeCounts, rangeKey, tx);
    }

    displaySummary(totalIncome, totalExpense);
    if (!rangeCounts.empty()) {
      std::cout << "\n金额区间统计：\n区间\t收入笔数\t支出笔数\t总笔数\n"
                << "--------------------------------------\n";
      for (size_t i = 0; i < rangeCounts.size(); ++i) {
        int total = rangeCounts[i].second.first + rangeCounts[i].second.second;
        std::cout << rangeCounts[i].first << '\t'
                  << rangeCounts[i].second.first << '\t'
                  << rangeCounts[i].second.second << '\t' << total << '\n';
      }
    }
  }

  void calculateHomePage(double& totalIncome, double& totalExpense,
                        double& balance) {
    totalIncome = totalExpense = 0.0;
    for (size_t i = 0; i < transactions.size(); ++i) {
      if (transactions[i].getType() == TransactionType::Income) {
        totalIncome += transactions[i].getAmount();
      } else {
        totalExpense += transactions[i].getAmount();
      }
    }
    balance = totalIncome - totalExpense;
  }

 private:
  const std::vector<Transaction>& transactions;

  void displaySummary(double income, double expense) {
    std::cout << "\n========== 统计结果 ==========\n"
              << "总收入: " << income << "\n总支出: " << expense
              << "\n结余: " << (income - expense) << std::endl;
  }

  bool inDateRange(const std::string& date,
                   const std::vector<std::string>& range) {
    if (range.size() != 2) return true;
    if (!range[0].empty() && date < range[0]) return false;
    if (!range[1].empty() && date > range[1]) return false;
    return true;
  }

  std::string getTimeKey(const std::string& time, TimeGroup group) {
    if (group == TimeGroup::Monthly && time.size() >= 7) {
      return time.substr(0, 7);
    } else if (group == TimeGroup::Yearly && time.size() >= 4) {
      return time.substr(0, 4);
    }
    return time;
  }

  void updateTimeCounts(std::vector<std::pair<std::string,
                        std::pair<double, double>>>& counts,
                        const std::string& key, const Transaction& tx) {
    for (size_t j = 0; j < counts.size(); ++j) {
      if (counts[j].first == key) {
        if (tx.getType() == TransactionType::Income) {
          counts[j].second.first += tx.getAmount();
        } else {
          counts[j].second.second += tx.getAmount();
        }
        return;
      }
    }
    double income = tx.getType() == TransactionType::Income ?
                   tx.getAmount() : 0.0;
    double expense = tx.getType() == TransactionType::Expense ?
                    tx.getAmount() : 0.0;
    counts.push_back(std::make_pair(key, std::make_pair(income, expense)));
  }

  void updateAmounts(std::vector<std::pair<std::string,
                     std::pair<double, double>>>& amounts,
                     const std::string& name, const Transaction& tx) {
    for (size_t j = 0; j < amounts.size(); ++j) {
      if (amounts[j].first == name) {
        if (tx.getType() == TransactionType::Income) {
          amounts[j].second.first += tx.getAmount();
        } else {
          amounts[j].second.second += tx.getAmount();
        }
        return;
      }
    }
    double income = tx.getType() == TransactionType::Income ?
                   tx.getAmount() : 0.0;
    double expense = tx.getType() == TransactionType::Expense ?
                    tx.getAmount() : 0.0;
    amounts.push_back(std::make_pair(name, std::make_pair(income, expense)));
  }

  std::string getAmountRange(double amt) {
    if (amt < 100.0) {
      return "小于100";
    } else if (amt < 500.0) {
      return "100-499";
    } else if (amt < 1000.0) {
      return "500-999";
    } else {
      return "1000及以上";
    }
  }

  void updateRangeCounts(std::vector<std::pair<std::string,
                         std::pair<int, int>>>& counts,
                         const std::string& key, const Transaction& tx) {
    for (size_t j = 0; j < counts.size(); ++j) {
      if (counts[j].first == key) {
        if (tx.getType() == TransactionType::Income) {
          counts[j].second.first++;
        } else {
          counts[j].second.second++;
        }
        return;
      }
    }
    int incomeCount = tx.getType() == TransactionType::Income ? 1 : 0;
    int expenseCount = tx.getType() == TransactionType::Expense ? 1 : 0;
    counts.push_back(std::make_pair(key,
                     std::make_pair(incomeCount, expenseCount)));
  }
};

// 搜索服务
class SearchService {
 public:
  explicit SearchService(const std::vector<Transaction>& data)
      : transactions(data) {}

  std::vector<Transaction> searchTransaction(
      const std::vector<std::string>& dateRange,
      const std::vector<double>& amountRange,
      const std::string& categoryId,
      const std::string& keyword) const {
    std::vector<Transaction> result;
    for (size_t i = 0; i < transactions.size(); ++i) {
      const Transaction& tx = transactions[i];
      if (dateRange.size() == 2) {
        if (!dateRange[0].empty() && tx.getTime() < dateRange[0]) continue;
        if (!dateRange[1].empty() && tx.getTime() > dateRange[1]) continue;
      }
      if (amountRange.size() == 2) {
        if (tx.getAmount() < amountRange[0] ||
            tx.getAmount() > amountRange[1]) continue;
      }
      if (!categoryId.empty() && tx.getCategoryId() != categoryId) continue;
      if (!keyword.empty()) {
        if (tx.getId().find(keyword) == std::string::npos &&
            tx.getRemarks().find(keyword) == std::string::npos) continue;
      }
      result.push_back(tx);
    }
    return result;
  }

 private:
  const std::vector<Transaction>& transactions;
};

// UI 服务
class UIService {
 public:
  static std::vector<Transaction> sortTransactionList(
      std::vector<Transaction> list, SortField field, SortDirection direction) {
    for (size_t i = 0; i < list.size(); ++i) {
      for (size_t j = i + 1; j < list.size(); ++j) {
        bool shouldSwap = false;
        if (field == SortField::Amount) {
          shouldSwap = (direction == SortDirection::Asc) ?
                      list[j].getAmount() < list[i].getAmount() :
                      list[j].getAmount() > list[i].getAmount();
        } else {
          shouldSwap = (direction == SortDirection::Asc) ?
                      list[j].getTime() < list[i].getTime() :
                      list[j].getTime() > list[i].getTime();
        }
        if (shouldSwap) std::swap(list[i], list[j]);
      }
    }
    return list;
  }
};

std::vector<Category> Category::categories;
std::vector<Transaction> Transaction::repository;

// 数据持久化管理类
class DataPersistence {
 public:
  static const char DB_FILE[];
  static const char DELIMITER = '|';

  static bool saveAll() {
    std::ofstream file(DB_FILE, std::ios::trunc);
    if (!file.is_open()) return false;

    file << "[CATEGORIES]\n";
    std::vector<Category> cats = Category::getCategoryList();
    for (size_t i = 0; i < cats.size(); ++i) {
      file << cats[i].getId() << DELIMITER << cats[i].getName() << '\n';
    }

    file << "[TRANSACTIONS]\n";
    const std::vector<Transaction>& txs = Transaction::list();
    for (size_t i = 0; i < txs.size(); ++i) {
      file << txs[i].getId() << DELIMITER << txs[i].getAmount() << DELIMITER
           << txs[i].getTime() << DELIMITER << txs[i].getCategoryId()
           << DELIMITER << static_cast<int>(txs[i].getType()) << DELIMITER
           << escapeString(txs[i].getRemarks()) << '\n';
    }
    file.close();
    return true;
  }

  static bool loadAll() {
    std::ifstream file(DB_FILE);
    if (!file.is_open()) return false;

    std::string line, section;
    while (std::getline(file, line)) {
      if (line.empty()) {
        continue;
      }
      if (line[0] == '[') {
        section = line;
        continue;
      }
      if (section == "[CATEGORIES]") {
        loadCategory(line);
      } else if (section == "[TRANSACTIONS]") {
        loadTransaction(line);
      }
    }
    file.close();
    return true;
  }

 private:
  static std::string escapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
      if (str[i] == DELIMITER) {
        result += "\\|";
      } else if (str[i] == '\n') {
        result += "\\n";
      } else if (str[i] == '\\') {
        result += "\\\\";
      } else {
        result += str[i];
      }
    }
    return result;
  }

  static std::string unescapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
      if (str[i] == '\\' && i + 1 < str.size()) {
        if (str[i + 1] == '|') {
          result += '|';
          ++i;
        } else if (str[i + 1] == 'n') {
          result += '\n';
          ++i;
        } else if (str[i + 1] == '\\') {
          result += '\\';
          ++i;
        } else {
          result += str[i];
        }
      } else {
        result += str[i];
      }
    }
    return result;
  }

  static std::vector<std::string> split(const std::string& str,
                                       char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    bool escaped = false;
    for (size_t i = 0; i < str.size(); ++i) {
      if (str[i] == '\\' && !escaped) {
        escaped = true;
        token += str[i];
      } else if (str[i] == delimiter && !escaped) {
        tokens.push_back(token);
        token.clear();
      } else {
        token += str[i];
        escaped = false;
      }
    }
    tokens.push_back(token);
    return tokens;
  }

  static void loadCategory(const std::string& line) {
    std::vector<std::string> parts = split(line, DELIMITER);
    if (parts.size() >= 2) {
      Category::addCategory(Category(parts[0], unescapeString(parts[1])));
    }
  }

  static void loadTransaction(const std::string& line) {
    std::vector<std::string> parts = split(line, DELIMITER);
    if (parts.size() >= 6) {
      Transaction tx;
      tx.setId(parts[0]);
      tx.setAmount(std::stod(parts[1]));
      tx.setTime(parts[2]);
      tx.setCategoryId(parts[3]);
      tx.setType(static_cast<TransactionType>(std::stoi(parts[4])));
      tx.setRemarks(unescapeString(parts[5]));
      if (Transaction::validateTransaction(tx)) {
        Transaction::getRepository().push_back(tx);
      }
    }
  }
};

const char DataPersistence::DB_FILE[] = "bookkeeping.db";

bool Transaction::addTransaction(const Transaction& t) {
  if (!validateTransaction(t)) return false;
  repository.push_back(t);
  DataPersistence::saveAll();
  return true;
}

bool Transaction::deleteTransaction(const std::string& id) {
  for (size_t i = 0; i < repository.size(); ++i) {
    if (repository[i].transactionId == id) {
      repository.erase(
          repository.begin() + static_cast<std::ptrdiff_t>(i));
      DataPersistence::saveAll();
      return true;
    }
  }
  return false;
}

std::string generateTransactionId() {
  static int counter = -1;
  if (counter == -1) {
    counter = 1000;
    const std::vector<Transaction>& txs = Transaction::list();
    for (size_t i = 0; i < txs.size(); ++i) {
      std::string id = txs[i].getId();
      if (id.size() > 1 && id[0] == 'T') {
        int num = std::stoi(id.substr(1));
        if (num >= counter) counter = num + 1;
      }
    }
  }
  return "T" + std::to_string(counter++);
}

std::string generateCategoryId() {
  static int counter = -1;
  if (counter == -1) {
    counter = 100;
    std::vector<Category> cats = Category::getCategoryList();
    for (size_t i = 0; i < cats.size(); ++i) {
      std::string id = cats[i].getId();
      if (id.size() > 1 && id[0] == 'c') {
        int num = std::stoi(id.substr(1));
        if (num >= counter) counter = num + 1;
      }
    }
  }
  return "c" + std::to_string(counter++);
}

std::string readString(const std::string& prompt) {
  std::cout << prompt;
  std::string input;
  std::getline(std::cin, input);
  return input;
}

double readDouble(const std::string& prompt) {
  while (true) {
    std::string input = readString(prompt);
    if (input.empty()) return -1.0;
    std::stringstream ss(input);
    double value;
    if (ss >> value && ss.eof()) return value;
    std::cout << "输入无效，请输入有效的数字！\n";
  }
}

int readInt(const std::string& prompt) {
  while (true) {
    std::string input = readString(prompt);
    if (input.empty()) return -1;
    std::stringstream ss(input);
    int value;
    if (ss >> value && ss.eof()) return value;
    std::cout << "输入无效，请输入有效的整数！\n";
  }
}

bool isValidDate(const std::string& date) {
  if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
  for (size_t i = 0; i < date.size(); ++i) {
    if (i == 4 || i == 7) continue;
    if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
  }
  int year = std::stoi(date.substr(0, 4));
  int month = std::stoi(date.substr(5, 2));
  int day = std::stoi(date.substr(8, 2));
  if (year < 1900 || year > 2100 || month < 1 || month > 12) return false;
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    daysInMonth[1] = 29;
  }
  return day >= 1 && day <= daysInMonth[month - 1];
}

std::string readDate(const std::string& prompt, bool required = true) {
  while (true) {
    std::string date = readString(prompt);
    if (!required && date.empty()) return date;
    if (required && date.empty()) {
      std::cout << "错误：日期不能为空！\n";
      continue;
    }
    if (!isValidDate(date)) {
      std::cout << "错误：日期格式无效或日期不存在！(YYYY-MM-DD)\n";
      continue;
    }
    return date;
  }
}

void displayCategories() {
  std::vector<Category> cats = Category::getCategoryList();
  std::cout << "\n可用分类：\n";
  for (size_t i = 0; i < cats.size(); ++i) {
    std::cout << "  [" << cats[i].getId() << "] " << cats[i].getName()
              << '\n';
  }
}

void manageCategoriesMenu() {
  std::cout << "\n========== 分类管理 ==========\n";
  bool dataChanged = false;
  while (true) {
    std::cout << "\n1. 添加分类\n2. 删除分类\n3. 返回\n";
    int choice = readInt("请输入选项: ");
    if (choice == 1) {
      std::string id = generateCategoryId();
      std::string name;
      while (name.empty()) {
        name = readString("分类名称（必填）: ");
        if (name.empty()) std::cout << "不能为空！\n";
      }
      if (Category::addCategory(Category(id, name))) {
        std::cout << "添加成功 ID: " << id << '\n';
        dataChanged = true;
      } else {
        std::cout << "添加失败！\n";
      }
    } else if (choice == 2) {
      displayCategories();
      std::string id = readString("删除的分类ID(0取消): ");
      if (id == "0") {
        std::cout << "已取消。\n";
        continue;
      }
      if (!Category::categoryExists(id)) {
        std::cout << "不存在！\n";
        continue;
      }
      if (Transaction::categoryHasTransactions(id)) {
        std::cout << "该分类下存在交易,无法删除！\n";
        continue;
      }
      if (Category::deleteCategory(id)) {
        std::cout << "删除成功。\n";
        dataChanged = true;
      }
    } else if (choice == 3) {
      break;
    }
  }
  if (dataChanged) DataPersistence::saveAll();
}

void recordTransaction() {
  std::cout << "\n========== 记一笔 ==========\n";
  while (true) {
    Transaction tx;
    tx.setId(generateTransactionId());
    double amount = -1.0;
    while (amount < 0.0) {
      amount = readDouble("金额（>=0）: ");
      if (amount < 0.0) std::cout << "金额不能为负！\n";
    }
    tx.setAmount(amount);
    tx.setTime(readDate("日期(YYYY-MM-DD): ", true));

    std::string categoryId;
    while (categoryId.empty()) {
      displayCategories();
      std::cout << "[0] 添加新分类\n";
      categoryId = readString("分类ID或0新增: ");
      if (categoryId == "0") {
        std::string newName;
        while (newName.empty()) {
          newName = readString("新分类名称: ");
          if (newName.empty()) std::cout << "不能为空！\n";
        }
        std::string newId = generateCategoryId();
        if (Category::addCategory(Category(newId, newName))) {
          std::cout << "新分类添加成功: " << newId << '\n';
          categoryId = newId;
        } else {
          categoryId.clear();
        }
      } else if (!Category::categoryExists(categoryId)) {
        std::cout << "分类不存在。\n";
        categoryId.clear();
      }
    }
    tx.setCategoryId(categoryId);

    int typeChoice = -1;
    while (typeChoice != 1 && typeChoice != 2) {
      typeChoice = readInt("类型 [1收入 2支出]: ");
    }
    tx.setType(typeChoice == 1 ? TransactionType::Income :
               TransactionType::Expense);
    tx.setRemarks(readString("备注(可空): "));

    if (Transaction::validateTransaction(tx)) {
      Transaction::addTransaction(tx);
      std::cout << "记账成功 ID: " << tx.getId() << '\n';
    }
    std::string cont = readString("继续？[y/n]: ");
    if (cont != "y" && cont != "Y") break;
  }
}

void statisticAnalysis() {
  std::cout << "\n========== 统计分析 ==========\n";
  while (true) {
    std::cout << "\n1. 按类型\n2. 按时间\n3. 按分类\n"
              << "4. 按金额区间\n0. 返回\n";
    int choice = readInt("选项: ");
    StatisticService stat(Transaction::list());
    if (choice == 1) {
      stat.calculateAndDisplayByType();
    } else if (choice == 2) {
      std::cout << "1日 2月 3年\n";
      int g = readInt("粒度: ");
      TimeGroup group = (g == 2) ? TimeGroup::Monthly :
                       (g == 3) ? TimeGroup::Yearly : TimeGroup::Daily;
      std::string startDate = readDate("开始日期(可空): ", false);
      std::string endDate = readDate("结束日期(可空): ", false);
      if (!startDate.empty() && !endDate.empty() && startDate > endDate) {
        std::swap(startDate, endDate);
      }
      stat.calculateAndDisplayByTime(group, {startDate, endDate});
    } else if (choice == 3) {
      stat.calculateAndDisplayByCategory();
    } else if (choice == 4) {
      stat.calculateAndDisplayByAmountRange();
    } else if (choice == 0) {
      break;
    }
    std::string cont = readString("继续统计？[y/n]: ");
    if (cont != "y" && cont != "Y") break;
  }
}

void displayTransactionList(const std::vector<Transaction>& list) {
  if (list.empty()) {
    std::cout << "\n无记录。\n";
    return;
  }
  std::cout << "\n========== 交易列表 ==========\n";
  for (size_t i = 0; i < list.size(); ++i) {
    std::cout << (i + 1) << ". " << list[i].getTransactionDetail() << '\n';
  }
  std::cout << "共 " << list.size() << " 条\n";
}

void editTransaction(Transaction& tx) {
  std::cout << "\n========== 编辑 ==========\n"
            << tx.getTransactionDetail() << '\n';
  std::string input = readString("新金额(回车跳过): ");
  if (!input.empty()) {
    std::stringstream ss(input);
    double v;
    if (ss >> v && v >= 0.0) tx.setAmount(v);
  }
  std::string nd = readString("新日期(回车跳过): ");
  if (!nd.empty() && isValidDate(nd)) tx.setTime(nd);

  displayCategories();
  std::cout << "[0] 新增分类\n";
  input = readString("新分类ID(回车跳过/0新增): ");
  if (!input.empty()) {
    if (input == "0") {
      std::string nm;
      while (nm.empty()) nm = readString("新分类名称: ");
      std::string nid = generateCategoryId();
      if (Category::addCategory(Category(nid, nm))) tx.setCategoryId(nid);
    } else if (Category::categoryExists(input)) {
      tx.setCategoryId(input);
    }
  }
  input = readString("新类型[1收入 2支出](回车跳过): ");
  if (input == "1") {
    tx.setType(TransactionType::Income);
  } else if (input == "2") {
    tx.setType(TransactionType::Expense);
  }

  input = readString("新备注(回车跳过): ");
  if (!input.empty()) tx.setRemarks(input);

  if (tx.updateTransaction()) {
    std::cout << "更新成功。\n";
    DataPersistence::saveAll();
  }
}

void searchBills() {
  std::cout << "\n========== 查找账单 ==========\n";
  while (true) {
    std::string startDate = readDate("起始日期(可空): ", false);
    std::string endDate = readDate("结束日期(可空): ", false);
    if (!startDate.empty() && !endDate.empty() && startDate > endDate) {
      std::swap(startDate, endDate);
    }
    double minAmount = readDouble("最小金额(可空): ");
    double maxAmount = readDouble("最大金额(可空): ");
    if (minAmount >= 0.0 && maxAmount >= 0.0 && minAmount > maxAmount) {
      std::swap(minAmount, maxAmount);
    }

    displayCategories();
    std::string categoryId = readString("分类ID(可空): ");
    if (!categoryId.empty() && !Category::categoryExists(categoryId)) {
      categoryId.clear();
    }
    std::string keyword = readString("关键词(可空): ");

    SearchService searchService(Transaction::list());
    std::vector<Transaction> results = searchService.searchTransaction(
        {startDate, endDate},
        {minAmount >= 0.0 ? minAmount : 0.0,
         maxAmount >= 0.0 ? maxAmount : 999999999.0},
        categoryId, keyword);

    displayTransactionList(results);

    if (results.empty()) {
      std::string c = readString("重新搜索？[y/n]: ");
      if (c != "y" && c != "Y") break;
      continue;
    }

    std::string sortChoice = readString("排序？[y/n]: ");
    if (sortChoice == "y" || sortChoice == "Y") {
      int f = readInt("字段[1金额 2时间]: ");
      int d = readInt("方向[1升 2降]: ");
      SortField sf = (f == 1) ? SortField::Amount : SortField::Time;
      SortDirection sd = (d == 1) ? SortDirection::Asc : SortDirection::Desc;
      results = UIService::sortTransactionList(results, sf, sd);
      displayTransactionList(results);
    }

    std::string op = readString("操作[1编辑 2删除 0跳过]: ");
    if (op == "1") {
      int idx = readInt("记录编号: ") - 1;
      if (idx >= 0 && static_cast<size_t>(idx) < results.size()) {
        std::vector<Transaction>& repo = Transaction::getRepository();
        for (size_t i = 0; i < repo.size(); ++i) {
          if (repo[i].getId() == results[idx].getId()) {
            editTransaction(repo[i]);
            break;
          }
        }
      }
    } else if (op == "2") {
      int idx = readInt("删除编号: ") - 1;
      if (idx >= 0 && static_cast<size_t>(idx) < results.size()) {
        std::string cf = readString("确认删除？[y/n]: ");
        if (cf == "y" || cf == "Y") {
          Transaction::deleteTransaction(results[idx].getId());
        }
      }
    }

    std::string cont = readString("继续查找？[y/n]: ");
    if (cont != "y" && cont != "Y") break;
  }
}

void displayHomePage() {
  StatisticService stat(Transaction::list());
  double ti, te, bal;
  stat.calculateHomePage(ti, te, bal);
  std::cout << "\n================================\n"
            << "        个人记账系统首页\n"
            << "================================\n"
            << "总收入: " << ti << "\n总支出: " << te << "\n结余:   "
            << bal << "\n================================\n";
}

int main() {
  // 设置控制台为 UTF-8，确保中文不乱码
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);

  if (!DataPersistence::loadAll()) {
    std::cout << "首次启动，初始化默认分类...\n";
    Category::addCategory(Category("c1", "餐饮"));
    Category::addCategory(Category("c2", "交通"));
    Category::addCategory(Category("c3", "购物"));
    Category::addCategory(Category("c4", "工资"));
    Category::addCategory(Category("c5", "奖金"));
    Category::addCategory(Category("c6", "娱乐"));
    Category::addCategory(Category("c7", "医疗"));
    Category::addCategory(Category("c8", "教育"));
    DataPersistence::saveAll();
  }

  std::cout << "欢迎使用个人记账系统！\n";
  while (true) {
    displayHomePage();
    std::cout << "\n1. 记一笔\n2. 统计分析\n3. 查找账单\n"
              << "4. 分类管理\n0. 退出\n";
    int choice = readInt("请选择功能: ");
    if (choice == 1) {
      recordTransaction();
    } else if (choice == 2) {
      statisticAnalysis();
    } else if (choice == 3) {
      searchBills();
    } else if (choice == 4) {
      manageCategoriesMenu();
    } else if (choice == 0) {
      std::cout << "正在保存数据...\n";
      DataPersistence::saveAll();
      std::cout << "再见！\n";
      break;
    }
  }
  return 0;
}