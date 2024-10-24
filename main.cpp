#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>
#include <unordered_map>

int main() {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    
    std::ranges::sort(vec);
    
    for (int i : vec) {
        std::cout << i << ' ';
    }
    std::cout << '\n';
    
    return 0;
}



