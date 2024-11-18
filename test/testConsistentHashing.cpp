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
        : consistentHashing(std::vector<std::string>{"a", "d", "g"}, 26) {}

    ConsistentHashing<MockHashingFunction> consistentHashing; // Inject the mock hash function
};

TEST_F(ConsistentHashingTest, FindNodeReturnsCorrectHashMap) {
    // Ensure hash ring has expected content
    EXPECT_EQ(consistentHashing.findServer("a"), "a");
    EXPECT_EQ(consistentHashing.findServer("d"), "d");
    EXPECT_EQ(consistentHashing.findServer("g"), "g");

    EXPECT_EQ(consistentHashing.findServer("b"), "d");
    EXPECT_EQ(consistentHashing.findServer("e"), "g");

    // Check wrapping behavior
    EXPECT_EQ(consistentHashing.findServer("x"), "a");
}

// TODO reintroduce tests once deleting from hashing ring, means to rebalance keys between servers



// TEST_F(ConsistentHashingTest, RemoveNodesRebalancing) {

//     //setup
//     auto& hmToDelete = consistentHashing.findServer("a");
//     hmToDelete.insert("foo", "val");
//     hmToDelete.insert("bar", "val2");
//     hmToDelete.insert("foobar", "val3");

//     auto& hmToReceive = consistentHashing.findServer("d");
//     hmToReceive.insert("soo", "sal");
//     hmToReceive.insert("sar", "sal2");
//     hmToReceive.insert("soosar", "sal3");
//     // setup end

    
//     consistentHashing.removeNode("a");

//     // TODO currently doesn't hold as map reads from disk extra values - prevent test reading from file or read empty disk
//     // EXPECT_TRUE(hmToReceive.kv_store.size() == 6);

//     EXPECT_TRUE(hmToReceive.get("foo"));
//     EXPECT_TRUE(hmToReceive.get("bar"));
//     EXPECT_TRUE(hmToReceive.get("foobar"));
//     EXPECT_TRUE(hmToReceive.get("soo"));
//     EXPECT_TRUE(hmToReceive.get("sar"));
//     EXPECT_TRUE(hmToReceive.get("soosar"));
// }

// TEST_F(ConsistentHashingTest, RemoveEndNodeRebalancingClockwise) {

//     //setup - add values to end node before deleting node
//     auto& hmToDelete = consistentHashing.findNode("g");
//     hmToDelete.insert("foo", "val");
//     hmToDelete.insert("bar", "val2");
//     // setup end
//     consistentHashing.removeNode("g");

//     // TODO currently doesn't hold as map reads from disk extra values - prevent test reading from file or read empty disk
//     // EXPECT_TRUE(hmToReceive.kv_store.size() == 2);
//     auto& hmToReceive = consistentHashing.findServer("a");

//     EXPECT_TRUE(hmToReceive.get("foo"));
//     EXPECT_TRUE(hmToReceive.get("bar"));
// }


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
