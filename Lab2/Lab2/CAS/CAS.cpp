#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <climits>
#include <algorithm>

#define ARRAY_SIZE 10000000
#define NUM_THREADS 6

std::atomic<int> max_values[3] = { INT_MIN, INT_MIN, INT_MIN };

void generate_array(int* array, int size) {
    srand(1);
    for (int i = 0; i < size; i++) {
        array[i] = (rand() % 201) - 100;
    }
}

void find_max_in_part(int* array, int start, int end) {
    int local_max[3] = { INT_MIN, INT_MIN, INT_MIN };

    for (int i = start; i < end; i++) {
        int value = array[i];
        if (value > local_max[0]) {
            local_max[2] = local_max[1];
            local_max[1] = local_max[0];
            local_max[0] = value;
        }
        else if (value > local_max[1]) {
            local_max[2] = local_max[1];
            local_max[1] = value;
        }
        else if (value > local_max[2]) {
            local_max[2] = value;
        }
    }

    // Оновлення глобальних максимумів один раз
    while (true) {
        int global0 = max_values[0].load();
        int global1 = max_values[1].load();
        int global2 = max_values[2].load();

        int new_global[3] = { global0, global1, global2 };

        // Злиття локальних максимумів з глобальними
        for (int local_val : local_max) {
            if (local_val > new_global[0]) {
                new_global[2] = new_global[1];
                new_global[1] = new_global[0];
                new_global[0] = local_val;
            }
            else if (local_val > new_global[1]) {
                new_global[2] = new_global[1];
                new_global[1] = local_val;
            }
            else if (local_val > new_global[2]) {
                new_global[2] = local_val;
            }
        }

        if (max_values[2].compare_exchange_weak(global2, new_global[2]) &&
            max_values[1].compare_exchange_weak(global1, new_global[1]) &&
            max_values[0].compare_exchange_weak(global0, new_global[0])) {
            break;
        }
    }
}

int main() {
    int* array = new int[ARRAY_SIZE];
    generate_array(array, ARRAY_SIZE);

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    int part_size = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        int start = i * part_size;
        int end = (i == NUM_THREADS - 1) ? ARRAY_SIZE : start + part_size;
        threads.emplace_back(find_max_in_part, array, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }

    int sum = max_values[0] + max_values[1] + max_values[2];
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start) / 1000;

    std::cout << "Max: " << max_values[0] << ", "
        << max_values[1] << ", " << max_values[2] << std::endl;
    std::cout << "Sum " << sum << std::endl;
    std::cout << "Time: " << duration.count() << " ms\n";

    delete[] array;
    return 0;
}