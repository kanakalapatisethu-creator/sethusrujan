#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

const std::string FILE_NAME = "employees.txt";
const int NUM_MONTHS = 3;

class InvalidSalaryException : public std::runtime_error {
public:
    explicit InvalidSalaryException(const std::string &m) : std::runtime_error(m) {}
};

// Base class
class Employee {
protected:
    int empId;
    std::string name;
public:
    Employee(int i = 0, const std::string &n = "") : empId(i), name(n) {}
    virtual ~Employee() = default;
    virtual void display() const {
        std::cout << "Emp ID : " << empId << "\nName   : " << name << "\n";
    }
    virtual std::string toFileString() const { return std::to_string(empId) + "," + name; }
};

// Derived class
class Payroll : public Employee {
    std::vector<double> salaries;   // last 3 months
    static void validate(const std::vector<double> &s) {
        if ((int)s.size() != NUM_MONTHS)
            throw InvalidSalaryException("Exactly " + std::to_string(NUM_MONTHS) + " monthly salaries required.");
        for (double x : s)
            if (x < 0 || x > 500000)
                throw InvalidSalaryException("Invalid salary (" + std::to_string((int)x) + "). Must be 0 to 500000.");
    }
public:
    Payroll(int i, const std::string &n, const std::vector<double> &s) : Employee(i, n), salaries(s) { validate(salaries); }
    double getTotal() const { return std::accumulate(salaries.begin(), salaries.end(), 0.0); }
    double getAverage() const { return getTotal() / NUM_MONTHS; }
    std::string getGrade() const {
        double avg = getAverage();
        return avg >= 50000 ? "A" : (avg >= 30000 ? "B" : "C");
    }
    void display() const override {
        Employee::display();
        std::cout << "Salaries: ";
        for (double x : salaries) std::cout << x << " ";
        std::cout << "\nTotal  : " << std::fixed << std::setprecision(2) << getTotal()
                  << "\nAverage: " << getAverage() << "\nGrade  : " << getGrade() << "\n"
                  << "-----------------------------------\n";
    }
    std::string toFileString() const override {
        std::string s = Employee::toFileString();
        for (double x : salaries) s += "," + std::to_string(x);
        return s;
    }
};

template <typename T>
void displayAll(const std::vector<std::shared_ptr<T>> &list) {
    if (list.empty()) { std::cout << "No records to display.\n"; return; }
    for (const auto &e : list) e->display();
}

void saveToFile(const std::vector<std::shared_ptr<Employee>> &recs) {
    std::ofstream out(FILE_NAME);
    if (!out) throw std::runtime_error("Unable to open file for writing.");
    for (const auto &r : recs) out << r->toFileString() << "\n";
    std::cout << "Saved " << recs.size() << " record(s).\n";
}

std::vector<std::shared_ptr<Employee>> loadFromFile() {
    std::ifstream in(FILE_NAME);
    if (!in) throw std::runtime_error("File not found. Save records first.");
    std::vector<std::shared_ptr<Employee>> recs;
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string tok;
        std::vector<std::string> p;
        while (std::getline(ss, tok, ',')) p.push_back(tok);
        if ((int)p.size() != 2 + NUM_MONTHS) continue;
        try {
            std::vector<double> s;
            for (int i = 2; i < 2 + NUM_MONTHS; i++) s.push_back(std::stod(p[i]));
            recs.push_back(std::make_shared<Payroll>(std::stoi(p[0]), p[1], s));
        } catch (const std::exception &e) { std::cout << "Skipping bad line: " << e.what() << "\n"; }
    }
    return recs;
}

int readInt(const std::string &prompt) {
    int v;
    while (true) {
        std::cout << prompt;
        if (std::cin >> v) return v;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Enter a valid number.\n";
    }
}

void addEmployee(std::vector<std::shared_ptr<Employee>> &recs) {
    int id = readInt("Enter Emp ID: ");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string name;
    std::cout << "Enter Name: ";
    std::getline(std::cin, name);
    std::vector<double> sal;
    for (int i = 1; i <= NUM_MONTHS; i++) sal.push_back(readInt("Salary for month " + std::to_string(i) + ": "));
    try {
        recs.push_back(std::make_shared<Payroll>(id, name, sal));
        std::cout << "Employee added.\n";
    } catch (const InvalidSalaryException &e) {
        std::cout << "Error: " << e.what() << " Record not added.\n";
    }
}

void showAbove(const std::vector<std::shared_ptr<Employee>> &recs) {
    int limit = readInt("Show employees with total salary above: ");
    std::vector<std::shared_ptr<Employee>> out;
    std::copy_if(recs.begin(), recs.end(), std::back_inserter(out),
                 [limit](const std::shared_ptr<Employee> &e) {
                     auto p = std::dynamic_pointer_cast<Payroll>(e);
                     return p && p->getTotal() > limit;
                 });
    std::cout << "\n" << out.size() << " employee(s) above " << limit << ":\n";
    displayAll(out);
}

int main() {
    std::vector<std::shared_ptr<Employee>> recs;
    int choice;
    do {
        std::cout << "\n===== Employee Payroll System =====\n1. Add employee\n2. Display all\n"
                     "3. Show total salary above a value\n4. Save to file\n5. Load from file\n0. Exit\n";
        choice = readInt("Enter choice: ");
        try {
            switch (choice) {
                case 1: addEmployee(recs); break;
                case 2: std::cout << "\n"; displayAll(recs); break;
                case 3: showAbove(recs); break;
                case 4: saveToFile(recs); break;
                case 5: recs = loadFromFile(); std::cout << "Loaded " << recs.size() << " record(s).\n"; break;
                case 0: std::cout << "Goodbye!\n"; break;
                default: std::cout << "Invalid choice.\n";
            }
        } catch (const std::exception &e) { std::cout << "Error: " << e.what() << "\n"; }
    } while (choice != 0);
}