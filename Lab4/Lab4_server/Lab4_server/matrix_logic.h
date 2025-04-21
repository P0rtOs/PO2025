#pragma once

#include <vector>
#include <thread>
#include <iostream>
#include <algorithm>
#include <atomic>

inline std::atomic<bool> matrix_processing_in_progress = false;

inline void process_matrix(std::vector<std::vector<int>>& matrix, int num_threads) {
    int rows = matrix.size();
    if (rows == 0) return;

    int cols = matrix[0].size();
    int last_col = cols - 1;

    std::cout << "[Matrix] Processing with " << num_threads << " threads..." << std::endl;

    auto worker = [&](int start_row) {
        for (int i = start_row; i < rows; i += num_threads) {
            int max_idx = 0;
            for (int j = 1; j < last_col; ++j) {
                if (matrix[i][j] > matrix[i][max_idx]) {
                    max_idx = j;
                }
            }
            std::swap(matrix[i][max_idx], matrix[i][last_col]);
        }
        };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "[Matrix] Processing complete." << std::endl;
}
