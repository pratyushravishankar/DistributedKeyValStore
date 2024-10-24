#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

int main() {
    std::unordered_map<std::string, std::string> kv_store;

    // Load initial data from file
    std::ifstream input_file("initial_data.txt");
    if (input_file.is_open()) {
        std::string key, value;
        while (input_file >> key >> value) {
            kv_store[key] = value;
        }
        input_file.close();
    } else {
        std::cerr << "Unable to open initial data file" << std::endl;
    }

    // Example usage
    std::cout << "Initial key-value store size: " << kv_store.size() << std::endl;


    for (const auto& [k, v] : kv_store) { 
        std::cout << k << " -> " << v << std::endl;
    }
    


    
}
