#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

vector<double> readVector(const string& path) {
    ifstream f(path);
    if (!f) {
        cerr << "can't open " << path << endl;
        exit(1);
    }
    int n;
    f >> n;
    vector<double> v(n);
    for (int i = 0; i < n; i++) f >> v[i];
    return v;
}

void writeVector(const string& path, const vector<double>& v) {
    ofstream f(path);
    f << v.size() << "\n" << fixed << setprecision(6);
    for (double x : v) f << x << "\n";
}

bool sameFile(const string& p1, const string& p2) {
    ifstream f1(p1, ios::binary), f2(p2, ios::binary);
    if (!f1 || !f2) return false;
    string s1((istreambuf_iterator<char>(f1)), istreambuf_iterator<char>());
    string s2((istreambuf_iterator<char>(f2)), istreambuf_iterator<char>());
    return s1 == s2;
}

int main() {
    vector<double> a = readVector("A.txt");
    vector<double> b = readVector("B.txt");
    int n = a.size();
    vector<double> c(n);

    auto t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
    auto t2 = chrono::high_resolution_clock::now();

    writeVector("output.txt", c);

    if (ifstream("expected.txt") && !sameFile("output.txt", "expected.txt")) {
        cerr << "output.txt is different from expected.txt" << endl;
        return 1;
    }

    chrono::duration<double, milli> ms = t2 - t1;
    cout << fixed << setprecision(4) << ms.count() << endl;
    return 0;
}
