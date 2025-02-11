#include <iostream>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <locale>
#include <thread>
#include <chrono>
#include <string>

#define SIZE 50000
#define NUMTHREADS 192

using namespace std;
using namespace std::chrono;

// Генерація матриці
void fillMatrix(vector<vector<short int>>& matrix, short int minValue, short int maxValue) {
    srand(1);
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            matrix[i][j] = minValue + rand() % (maxValue - minValue + 1);
        }
    }
}

// Обробка БЕЗ потоків
void defaultprocessMatrix(vector<vector<short int>>& matrix) {
    for (int i = 0; i < SIZE; ++i) {
        int minIndex = 0;
        for (int j = 1; j < SIZE; ++j) {
            if (matrix[i][j] < matrix[i][minIndex]) {
                minIndex = j;
            }
        }
        swap(matrix[i][SIZE - 1 - i], matrix[i][minIndex]);
    }
}

// Функція обробки рядків у потоці
void processRows(vector<vector<short int>>& matrix, int threadID, int numThreads) {
    for (int i = threadID; i < SIZE; i += numThreads) {
        int minIndex = 0;
        for (int j = 1; j < SIZE; ++j) {
            if (matrix[i][j] < matrix[i][minIndex]) {
                minIndex = j;
            }
        }
        string message = to_string(threadID) + " " + to_string(i) + "\n";
        //cout << message;
        //swap(matrix[i][SIZE - 1 - i], matrix[i][minIndex]);
    }
}

// Функція для запуску паралельної обробки
void processMatrixParallel(vector<vector<short int>>& matrix, int numThreads) {
    vector<thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(processRows, ref(matrix), i, numThreads);
    }
    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "uk_UA.UTF-8");

    //vector<vector<short int>> matrix(SIZE, vector<short int>(SIZE));
    //fillMatrix(matrix, 1, 100);

    //auto start1 = high_resolution_clock::now();
    //defaultprocessMatrix(matrix);
    //auto end1 = high_resolution_clock::now();
    //auto executionTime1 = duration_cast<milliseconds>(end1 - start1).count();

    //cout << "Time1: " << executionTime1 << " ms\n";

    vector<vector<short int>> matrix2(SIZE, vector<short int>(SIZE));
    fillMatrix(matrix2, 1, 100);

    auto start2 = high_resolution_clock::now();
    processMatrixParallel(matrix2, NUMTHREADS);
    auto end2 = high_resolution_clock::now();
    auto executionTime2 = duration_cast<milliseconds>(end2 - start2).count();

    cout << "Time2: " << executionTime2 << " ms\n";
    return 0;
}
