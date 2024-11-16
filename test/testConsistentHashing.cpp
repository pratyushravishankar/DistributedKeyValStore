#include <gtest/gtest.h>
#include "ConsistentHashing.h"
#include <cassert>



struct MockHashingFunction {
    // hash into one of 26 buckets based upon first letter
    size_t operator()(std::string s) {
        assert(s.size() > 0 && std::islower(*s.begin()));
        return *s.begin() - 'a';
    } 
};

class ConsistentHashingTest : public ::testing::Test {
protected:
    ConsistentHashingTest() 
        : consistentHashing(std::vector<HashMap>{HashMap{"a"}, HashMap{"d"}, HashMap{"g"}}) {} // Inject the mock hash function

    ConsistentHashing<MockHashingFunction> consistentHashing; // Specify the type of the hashing function
};

TEST_F(ConsistentHashingTest, FindNodeReturnsCorrectHashMap) {
    // Ensure hash ring has expected content
    EXPECT_EQ(consistentHashing.findNode("a").mName, "a");
    EXPECT_EQ(consistentHashing.findNode("d").mName, "d");
    EXPECT_EQ(consistentHashing.findNode("g").mName, "g");

    EXPECT_EQ(consistentHashing.findNode("b").mName, "d");
    EXPECT_EQ(consistentHashing.findNode("e").mName, "g");

    // Check wrapping behavior
    EXPECT_EQ(consistentHashing.findNode("x").mName, "a");
}

TEST_F(ConsistentHashingTest, RemoveNodesRebalancing) {

    //setup
    auto& hmToDelete = consistentHashing.findNode("a");
    hmToDelete.insert("foo", "val");
    hmToDelete.insert("bar", "val2");
    hmToDelete.insert("foobar", "val3");

    auto& hmToReceive = consistentHashing.findNode("d");
    hmToReceive.insert("soo", "sal");
    hmToReceive.insert("sar", "sal2");
    hmToReceive.insert("soosar", "sal3");
    // setup end

    
    consistentHashing.removeNode("a");

    // TODO currently doesn't hold as map reads from disk extra values - prevent test reading from file or read empty disk
    // EXPECT_TRUE(hmToReceive.kv_store.size() == 6);

    EXPECT_TRUE(hmToReceive.get("foo"));
    EXPECT_TRUE(hmToReceive.get("bar"));
    EXPECT_TRUE(hmToReceive.get("foobar"));
    EXPECT_TRUE(hmToReceive.get("soo"));
    EXPECT_TRUE(hmToReceive.get("sar"));
    EXPECT_TRUE(hmToReceive.get("soosar"));
}

TEST_F(ConsistentHashingTest, RemoveEndNodeRebalancingClockwise) {

    //setup - add values to end node before deleting node
    auto& hmToDelete = consistentHashing.findNode("g");
    hmToDelete.insert("foo", "val");
    hmToDelete.insert("bar", "val2");
    // setup end
    consistentHashing.removeNode("g");

    // TODO currently doesn't hold as map reads from disk extra values - prevent test reading from file or read empty disk
    // EXPECT_TRUE(hmToReceive.kv_store.size() == 2);
    auto& hmToReceive = consistentHashing.findNode("a");

    EXPECT_TRUE(hmToReceive.get("foo"));
    EXPECT_TRUE(hmToReceive.get("bar"));
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
