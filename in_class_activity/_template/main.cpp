#include <iostream>
#include <thread>
#include <vector>

using namespace std;

void work(int id) {
    cout << "thread " << id << " running" << endl;
}

int main(int argc, char** argv) {
    int p = argc > 1 ? atoi(argv[1]) : 4;  // number of threads

    vector<thread> threads;
    for (int t = 0; t < p; t++) threads.emplace_back(work, t);
    for (auto& th : threads) th.join();
    return 0;
}
