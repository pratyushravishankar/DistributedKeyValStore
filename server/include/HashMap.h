#include <unordered_map>
#include <string_view>
#include <string>

class HashMap
{
    // private:
public:
    std::unordered_map<std::string, std::string> kv_store;

    HashMap(std::string_view filename);
    void insert(const std::string &key, const std::string &value);
    std::string get(const std::string &key);
};
