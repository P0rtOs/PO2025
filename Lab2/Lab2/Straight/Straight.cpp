#include <iostream>
#include <chrono>
#include <climits>
#include <vector>
#include <cstdlib>

#define ARRAY_SIZE 10000000

void generate_array(std::vector<int>& array) {
    srand(1);
    for (int i = 0; i < array.size(); i++) {
        array[i] = (rand() % 201) - 100;
    }
}

void find_top3(const std::vector<int>& array, int& max1, int& max2, int& max3) {
    max1 = INT_MIN;
    max2 = INT_MIN;
    max3 = INT_MIN;

    for (int val : array) {
        if (val > max1) {
            max3 = max2;
            max2 = max1;
            max1 = val;
        }
        else if (val > max2) {
            max3 = max2;
            max2 = val;
        }
        else if (val > max3) {
            max3 = val;
        }
    }
}

int main() {
    std::vector<int> array(ARRAY_SIZE);
    generate_array(array);

    int max1, max2, max3;
    auto start = std::chrono::high_resolution_clock::now();
    find_top3(array, max1, max2, max3);
    int sum = max1 + max2 + max3;
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

    std::cout << "\nMax: " << max1 << ", " << max2 << ", " << max3 << "\n";
    std::cout << "Sum: " << sum << "\n";
    std::cout << "Time: " << duration << " ms\n";

    return 0;
}
