#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>
#include <chrono>

#define ARRAY_SIZE 10000000
#define THREAD_COUNT 6

std::mutex max_mutex;
int global_max[3] = { -101, -101, -101 };

void generate_array(std::vector<int>& array) {
    srand(1);
    for (int& num : array) {
        num = (rand() % 201) - 100;
    }
}

// Функція знаходження локального максимуму для потоку
void find_max_in_chunk(const std::vector<int>& array, int start, int end, int local_max[3]) {
    local_max[0] = local_max[1] = local_max[2] = -101;
    for (int i = start; i < end; ++i) {
        if (array[i] > local_max[0]) {
            local_max[2] = local_max[1];
            local_max[1] = local_max[0];
            local_max[0] = array[i];
        }
        else if (array[i] > local_max[1]) {
            local_max[2] = local_max[1];
            local_max[1] = array[i];
        }
        else if (array[i] > local_max[2]) {
            local_max[2] = array[i];
        }
    }
}

int main() {
    std::vector<int> array(ARRAY_SIZE);
    generate_array(array);

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    int chunk_size = ARRAY_SIZE / THREAD_COUNT;

    // Кожен потік матиме свій локальний масив максимумів
    int local_max[THREAD_COUNT][3];

    // Запускаємо потоки
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int start = i * chunk_size;
        int end = (i == THREAD_COUNT - 1) ? ARRAY_SIZE : start + chunk_size;
        threads.emplace_back(find_max_in_chunk, std::cref(array), start, end, local_max[i]);
    }

    // Чекаємо завершення потоків
    for (auto& thread : threads) {
        thread.join();
    }

    // Об'єднуємо локальні максимуми потоків в глобальний максимум (1 блокування)
    {
        std::lock_guard<std::mutex> lock(max_mutex);
        std::vector<int> merged;
        for (int i = 0; i < THREAD_COUNT; ++i) {
            merged.push_back(local_max[i][0]);
            merged.push_back(local_max[i][1]);
            merged.push_back(local_max[i][2]);
        }
        std::sort(merged.rbegin(), merged.rend()); // Сортуємо у спадаючому порядку
        global_max[0] = merged[0];
        global_max[1] = merged[1];
        global_max[2] = merged[2];
    }

    int sum = global_max[0] + global_max[1] + global_max[2];

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Max: " << global_max[0] << ", " << global_max[1] << ", " << global_max[2] << "\n";
    std::cout << "Sum: " << sum << "\n";
    std::cout << "Time: " << duration.count() << " ms\n";

    return 0;
}
