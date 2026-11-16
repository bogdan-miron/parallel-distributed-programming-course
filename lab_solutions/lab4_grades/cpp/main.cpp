// final grades from 10 project files: readers put (id, grade) pairs in a queue, workers add them to a sorted list
// usage: main <p> [pr]   p = 0 is the sequential version, otherwise p threads of which pr (default 2) are readers
// sequential writes output_seq.txt, the parallel version writes Results.txt and compares it with output_seq.txt
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <semaphore>
#include <chrono>

using namespace std;

#define FILES 10

struct Pair {
    int id;
    int grade;
};

// sorted list of (id, grade), no id twice, the whole list is locked while a pair is added
struct Node {
    int id;
    int grade;
    Node* next;
};

class SortedList {
    Node* head = nullptr;
    mutex m;

public:
    void add(int id, int grade) {
        lock_guard<mutex> lock(m);
        Node* prev = nullptr;
        Node* cur = head;
        while (cur != nullptr && cur->id < id) {
            prev = cur;
            cur = cur->next;
        }
        if (cur != nullptr && cur->id == id) {
            cur->grade += grade;
            return;
        }
        Node* node = new Node{id, grade, cur};
        if (prev == nullptr) head = node;
        else prev->next = node;
    }

    // the final grade is the sum divided by 10, written with one decimal
    void write(const char* name) {
        ofstream out(name);
        for (Node* cur = head; cur != nullptr; cur = cur->next) {
            out << cur->id << " " << cur->grade / 10 << "." << cur->grade % 10 << "\n";
        }
    }

    ~SortedList() {
        while (head != nullptr) {
            Node* next = head->next;
            delete head;
            head = next;
        }
    }
};

// unbounded queue made by hand: the semaphore counts the pairs in it, the mutex protects the links
struct QNode {
    Pair value;
    QNode* next;
};

class Queue {
    QNode* head = nullptr;
    QNode* tail = nullptr;
    mutex m;
    counting_semaphore<> items{0};

public:
    void push(Pair v) {
        QNode* node = new QNode{v, nullptr};
        {
            lock_guard<mutex> lock(m);
            if (tail == nullptr) head = node;
            else tail->next = node;
            tail = node;
        }
        items.release();
    }

    // waits while the queue is empty
    Pair pop() {
        items.acquire();
        QNode* node;
        {
            lock_guard<mutex> lock(m);
            node = head;
            head = node->next;
            if (head == nullptr) tail = nullptr;
        }
        Pair v = node->value;
        delete node;
        return v;
    }
};

string fileName(int f) {
    return "project" + to_string(f + 1) + ".txt";
}

bool sameFile(const char* f1, const char* f2) {
    ifstream a(f1, ios::binary), b(f2, ios::binary);
    string s1((istreambuf_iterator<char>(a)), istreambuf_iterator<char>());
    string s2((istreambuf_iterator<char>(b)), istreambuf_iterator<char>());
    return s1 == s2;
}

void sequential() {
    SortedList list;
    for (int f = 0; f < FILES; f++) {
        ifstream in(fileName(f));
        int id, grade;
        while (in >> id >> grade) list.add(id, grade);
    }
    list.write("output_seq.txt");
}

// reader r takes the files r, r + pr, r + 2 * pr, ... and puts every pair in the queue
void reader(int r, int pr, Queue& q) {
    for (int f = r; f < FILES; f += pr) {
        ifstream in(fileName(f));
        int id, grade;
        while (in >> id >> grade) q.push({id, grade});
    }
}

// an empty queue does not mean the files are finished, so the main thread
// puts one pair with id -1 for every worker when all the readers are done
void worker(Queue& q, SortedList& list) {
    while (true) {
        Pair v = q.pop();
        if (v.id < 0) break;
        list.add(v.id, v.grade);
    }
}

void parallel(int p, int pr) {
    Queue q;
    SortedList list;
    int pw = p - pr;

    vector<thread> readers, workers;
    for (int r = 1; r < pr; r++) readers.emplace_back(reader, r, pr, ref(q));
    for (int w = 0; w < pw; w++) workers.emplace_back(worker, ref(q), ref(list));

    // the main thread is reader 0
    reader(0, pr, q);
    for (auto& t : readers) t.join();

    for (int w = 0; w < pw; w++) q.push({-1, 0});
    for (auto& t : workers) t.join();

    list.write("Results.txt");
}

int main(int argc, char** argv) {
    int p = (argc > 1) ? atoi(argv[1]) : 0;
    int pr = (argc > 2 && argv[2][0] != '\0') ? atoi(argv[2]) : 2;
    if (p < 0 || (p > 0 && (pr < 1 || pr >= p))) {
        cerr << "usage: main <p> [pr], with 1 <= pr < p (p = 0 is the sequential version)" << endl;
        return 2;
    }

    // the time includes reading the files and writing the result
    auto start = chrono::high_resolution_clock::now();
    if (p == 0) sequential();
    else parallel(p, pr);
    auto end = chrono::high_resolution_clock::now();

    if (p > 0) {
        ifstream ref("output_seq.txt");
        if (!ref) {
            cerr << "output_seq.txt is missing, run the sequential version first" << endl;
            return 2;
        }
        if (!sameFile("Results.txt", "output_seq.txt")) {
            cerr << "Results.txt is different from output_seq.txt" << endl;
            return 1;
        }
    }
    cout << fixed << setprecision(4) << chrono::duration<double, milli>(end - start).count() << endl;
    return 0;
}
