#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <memory>
#include <thread>
#include <mutex>
#include <iomanip>

using namespace std;

// 1. Custom Exception Handling
class InventoryException : public exception {
protected:
    string message;

public:
    explicit InventoryException(const string& msg)
        : message(msg) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

class InvalidQuantityException : public InventoryException {
public:
    explicit InvalidQuantityException(int quantity)
        : InventoryException(
            "Invalid Quantity: " + to_string(quantity) +
            ". Quantity cannot be negative.") {}
};

class FileAccessException : public InventoryException {
public:
    explicit FileAccessException(const string& filename)
        : InventoryException(
            "Unable to access file: " + filename) {}
};

// 2. Singleton Design Pattern and Thread-Safe Logger
class AuditLogger {
private:
    mutex logMutex;

    AuditLogger() {}

public:
    AuditLogger(const AuditLogger&) = delete;
    void operator=(const AuditLogger&) = delete;

    static AuditLogger& getInstance() {
        static AuditLogger instance;
        return instance;
    }

    void log(const string& event) {
        lock_guard<mutex> lock(logMutex);

        ofstream file("inventory_log.txt", ios::app);

        if (file) {
            file << "[LOG] " << event << "\n";
        }
    }
};

// 3. Template Function
template <typename T>
T calculateAverage(const vector<T>& values) {
    if (values.empty())
        return T(0);

    T sum = accumulate(
        values.begin(), values.end(), T(0));

    return sum / static_cast<T>(values.size());
}

// 4. Abstract Base Class
class InventoryRecord {
public:
    virtual void displayRecord() const = 0;
    virtual string serialize() const = 0;
    virtual ~InventoryRecord() {}
};

// 5. Base Class
class Product {
protected:
    string productID;
    string productName;
    string category;

public:
    Product(string id = "", string name = "",
            string cat = "General")
        : productID(move(id)),
          productName(move(name)),
          category(move(cat)) {}

    virtual ~Product() {}

    string getID() const {
        return productID;
    }

    string getName() const {
        return productName;
    }

    string getCategory() const {
        return category;
    }
};

// 6. Derived Class and Multiple Inheritance
class InventoryItem : public Product,
                      public InventoryRecord {
private:
    int quantity;
    double price;
    double totalValue;

public:
    InventoryItem(const string& id,
                  const string& name,
                  const string& cat,
                  int qty,
                  double cost)
        : Product(id, name, cat),
          quantity(0), price(cost), totalValue(0) {

        setQuantity(qty);
        calculateValue();
    }

    void setQuantity(int qty) {
        if (qty < 0)
            throw InvalidQuantityException(qty);

        quantity = qty;
    }

    void calculateValue() {
        totalValue = quantity * price;
    }

    int getQuantity() const {
        return quantity;
    }

    double getPrice() const {
        return price;
    }

    double getTotalValue() const {
        return totalValue;
    }

    // Runtime Polymorphism
    void displayRecord() const override {
        cout << left
             << setw(12) << productID
             << setw(20) << productName
             << setw(15) << category
             << setw(10) << quantity
             << setw(12) << fixed << setprecision(2)
             << price
             << setw(12) << totalValue
             << "\n";
    }

    // File Serialization
    string serialize() const override {
        ostringstream oss;

        oss << productID << ","
            << productName << ","
            << category << ","
            << quantity << ","
            << price;

        return oss.str();
    }
};

// 7. Factory Design Pattern
class ProductFactory {
public:
    static unique_ptr<InventoryItem> createProduct(
        const string& id,
        const string& name,
        const string& category,
        int quantity,
        double price) {

        return make_unique<InventoryItem>(
            id, name, category, quantity, price);
    }
};

// 8. Inventory Manager
class InventoryManager {
private:
    vector<unique_ptr<InventoryItem>> products;

    const string dataFile = "inventory.csv";

public:

    // Add Product
    void addProduct(const string& id,
                    const string& name,
                    const string& category,
                    int quantity,
                    double price) {

        if (price < 0)
            throw InventoryException(
                "Product price cannot be negative.");

        auto item = ProductFactory::createProduct(
            id, name, category, quantity, price);

        AuditLogger::getInstance().log(
            "Added product: " + id);

        products.push_back(move(item));
    }

    // Save Records to CSV File
    void saveToFile() const {
        ofstream outFile(dataFile);

        if (!outFile)
            throw FileAccessException(dataFile);

        for (const auto& item : products) {
            outFile << item->serialize() << "\n";
        }

        outFile.close();

        AuditLogger::getInstance().log(
            "Inventory saved successfully.");

        cout << "\n[INFO] Inventory saved successfully.\n";
    }

    // Load Records from CSV File
    void loadFromFile() {
        ifstream inFile(dataFile);

        if (!inFile)
            throw FileAccessException(dataFile);

        products.clear();

        string line;

        while (getline(inFile, line)) {
            if (line.empty())
                continue;

            stringstream ss(line);

            string id, name, category;
            string quantityStr, priceStr;

            getline(ss, id, ',');
            getline(ss, name, ',');
            getline(ss, category, ',');
            getline(ss, quantityStr, ',');
            getline(ss, priceStr, ',');

            int quantity = stoi(quantityStr);
            double price = stod(priceStr);

            products.push_back(
                ProductFactory::createProduct(
                    id, name, category,
                    quantity, price));
        }

        inFile.close();

        AuditLogger::getInstance().log(
            "Inventory loaded successfully.");

        cout << "\n[INFO] Inventory loaded successfully.\n";
    }

    // Display All Products
    void displayAll() const {
        cout << "\n=============================================================="
             << "================\n";

        cout << left
             << setw(12) << "Product ID"
             << setw(20) << "Product Name"
             << setw(15) << "Category"
             << setw(10) << "Quantity"
             << setw(12) << "Price"
             << setw(12) << "Total Value"
             << "\n";

        cout << "--------------------------------------------------------------"
             << "----------------\n";

        for (const auto& item : products) {
            InventoryRecord* ptr = item.get();
            ptr->displayRecord();
        }

        cout << "=============================================================="
             << "================\n";
    }

    // Sort Products by Quantity
    void sortByQuantity() {
        sort(products.begin(), products.end(),
             [](const unique_ptr<InventoryItem>& a,
                const unique_ptr<InventoryItem>& b) {

                 return a->getQuantity() >
                        b->getQuantity();
             });

        cout << "\n[INFO] Products sorted by quantity.\n";
    }

    // Inventory Analysis and Filtering
    void analyzeInventory(int cutoff) const {
        int lowStock = count_if(
            products.begin(), products.end(),
            [cutoff](const unique_ptr<InventoryItem>& p) {
                return p->getQuantity() < cutoff;
            });

        cout << "\n========== INVENTORY ANALYSIS ==========\n";

        cout << "Total Products: " << products.size() << "\n";

        cout << "Low Stock Products: " << lowStock << "\n";

        cout << "Available Products: "
             << products.size() - lowStock << "\n";

        vector<double> values;

        double totalInventoryValue = 0;

        for (const auto& p : products) {
            values.push_back(p->getTotalValue());
            totalInventoryValue += p->getTotalValue();
        }

        cout << fixed << setprecision(2);

        cout << "Average Product Value: "
             << calculateAverage(values) << "\n";

        cout << "Total Inventory Value: "
             << totalInventoryValue << "\n";

        cout << "\nProducts Below Stock Limit ("
             << cutoff << "):\n";

        for_each(products.begin(), products.end(),
            [cutoff](const unique_ptr<InventoryItem>& p) {

                if (p->getQuantity() < cutoff) {
                    cout << " -> " << p->getName()
                         << " | Quantity: "
                         << p->getQuantity() << "\n";
                }
            });
    }

    // Multithreading and Mutex
    void runConcurrentAudit() const {
        mutex reportMutex;

        int lowStockCount = 0;

        auto worker = [&](size_t start, size_t end) {
            int localCount = 0;

            for (size_t i = start; i < end; ++i) {
                if (products[i]->getQuantity() < 20) {
                    ++localCount;
                }
            }

            lock_guard<mutex> lock(reportMutex);
            lowStockCount += localCount;
        };

        size_t mid = products.size() / 2;

        thread t1(worker, 0, mid);
        thread t2(worker, mid, products.size());

        t1.join();
        t2.join();

        cout << "\n[THREAD AUDIT] Concurrent inventory scan completed.\n";

        cout << "Products with quantity below 20: "
             << lowStockCount << "\n";

        AuditLogger::getInstance().log(
            "Concurrent stock audit completed.");
    }
};

// 9. Main Function
int main() {

    InventoryManager manager;

    cout << "============================================\n";
    cout << "       INVENTORY MANAGEMENT SYSTEM\n";
    cout << "============================================\n";

    // Adding Sample Products
    try {
        manager.addProduct(
            "P101", "Laptop", "Electronics", 35, 55000);

        manager.addProduct(
            "P102", "Keyboard", "Accessories", 15, 800);

        manager.addProduct(
            "P103", "Mouse", "Accessories", 45, 500);

        manager.addProduct(
            "P104", "Monitor", "Electronics", 10, 12000);

        manager.addProduct(
            "P105", "Printer", "Electronics", 25, 8500);

        cout << "\n[INFO] Five products added successfully.\n";
    }
    catch (const InventoryException& e) {
        cerr << "[EXCEPTION] " << e.what() << "\n";
    }

    // Testing Custom Exception
    cout << "\n--- Testing Exception Handling ---\n";

    try {
        cout << "Attempting to add invalid quantity...\n";

        manager.addProduct(
            "P106", "Test Product",
            "General", -5, 1000);
    }
    catch (const InvalidQuantityException& e) {
        cerr << "[CAUGHT EXCEPTION] "
             << e.what() << "\n";
    }
    catch (const InventoryException& e) {
        cerr << "[GENERAL EXCEPTION] "
             << e.what() << "\n";
    }

    // Saving and Loading Records
    cout << "\n--- File Handling ---\n";

    try {
        manager.saveToFile();
        manager.loadFromFile();
    }
    catch (const FileAccessException& e) {
        cerr << "[FILE ERROR] " << e.what() << "\n";
    }
    catch (const exception& e) {
        cerr << "[DATA ERROR] " << e.what() << "\n";
    }

    // Display Products
    cout << "\n--- All Inventory Records ---\n";
    manager.displayAll();

    // Sorting
    cout << "\n--- Sorting Inventory ---\n";

    manager.sortByQuantity();
    manager.displayAll();

    // Inventory Analysis
    manager.analyzeInventory(20);

    // Concurrent Audit
    manager.runConcurrentAudit();

    cout << "\n============================================\n";
    cout << "     INVENTORY SYSTEM COMPLETED\n";
    cout << "============================================\n";

    return 0;
}