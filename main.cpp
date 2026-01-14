#include "classes.cpp"
#include <iostream> 
#include <openssl/md5.h> 
#include <array> 
#include <string> 
#include <vector> 
#include <iomanip> 
#include <memory> 
#include <map> 
#include <sstream> 
#include <random> 
#include <set> 

int main() {
    using namespace std::chrono;

    std::cout << "\n=== Basic Node Construction Test ===\n";
    Node nodeA("NodeA", 5);

    std::cout << "NodeA physical hash: ";
    print_hash(nodeA.hash_value);

    std::cout << "\nVirtual nodes:\n";
    for (size_t i = 0; i < nodeA.virtual_nodes.size(); i++) {
        std::cout << "  vNode " << i << ": ";
        print_hash(nodeA.virtual_nodes[i]->hash_value);
    }

    std::cout << "\n=== Comparison / Ordering Test ===\n";
    Node nodeB("NodeB", 5);

    std::cout << "NodeB physical hash: ";
    print_hash(nodeB.hash_value);

    if (nodeA < nodeB) std::cout << "NodeA < NodeB\n";
    else if (nodeA > nodeB) std::cout << "NodeA > NodeB\n";
    else std::cout << "NodeA == NodeB\n";

    std::cout << "\n=== Virtual Node Sorting Test ===\n";
    {
        std::vector<virtual_node> vnodes;
        for (auto& vn : nodeA.virtual_nodes) vnodes.push_back(*vn);
        for (auto& vn : nodeB.virtual_nodes) vnodes.push_back(*vn);
        std::sort(vnodes.begin(), vnodes.end());

        std::cout << "Sorted virtual node hashes (" << vnodes.size() << " total):\n";
        for (auto &vn : vnodes) print_hash(vn.hash_value);
    }

    std::cout << "\n=== Ring Construction Test ===\n";
    Node nodeC("NodeC", 5);
    Node nodeD("NodeD", 5);

    Ring ring({ &nodeC, &nodeD });

    std::cout << "Ring contains " << ring.v_node_maps.size() << " virtual nodes.\n";

    size_t idx = 0;
    for (auto &[h, nodePtr] : ring.v_node_maps) {
        std::cout << "vNode " << idx++ << ": ";
        print_hash(h);
        std::cout << "  -> physical node: " << nodePtr->node_id << "\n";
    }

    std::cout << "\n=== Insert Data Test ===\n";
    Ring ring2({ &nodeA, &nodeB, &nodeC, &nodeD });

    std::vector<std::pair<std::string,std::string>> testData = {
        {"apple","red"}, {"orange","orange"}, {"banana","yellow"},
        {"foo","bar"}, {"zebra","animal"}, {"hello","world"}, {"world","earth"}
    };

    for (auto &[key,val] : testData) {
        data_node dn(val, key);
        Node* assigned = ring2.insert_data(&dn);

        std::cout << "Key \"" << key << "\" hashed to "
                  << hash_to_hex(dn.hash_value) << "\n";
        std::cout << "  -> stored on node: " << assigned->node_id << "\n";
    }

    std::cout << "\n=== Find Data Test ===\n";
    std::cout << "Test 1: Existing keys\n";

    for (auto &[key, expected] : testData) {
        std::string found = ring2.find_data(key);
        if (found == expected)
            std::cout << " ✓ \"" << key << "\" -> \"" << found << "\"\n";
        else
            std::cout << " ✗ \"" << key << "\" expected \"" << expected
                      << "\" got \"" << found << "\"\n";
    }

    std::cout << "\nTest 2: Missing keys\n";
    for (std::string miss : {"missing","nothere","fake"}) {
        std::string found = ring2.find_data(miss);
        std::cout << (found.empty() ? " ✓ " : " ✗ ") 
                  << "\"" << miss << "\" lookup\n";
    }

    std::cout << "\nTest 3: Consistency\n";
    {
        std::string k = "apple";
        std::string first = ring2.find_data(k);
        bool consistent = true;
        for (int i = 0; i < 5; i++) {
            if (ring2.find_data(k) != first) consistent = false;
        }
        std::cout << (consistent ? " ✓ consistent\n" : " ✗ inconsistent\n");
    }

    std::cout << "\n=== Large Scale Test (10,000 keys) ===\n";

    Node node1("Node1",150), node2("Node2",150),
         node3("Node3",150), node4("Node4",150), node5("Node5",150);

    Ring bigRing({&node1,&node2,&node3,&node4,&node5});

    int num_keys = 10000;
    std::vector<std::string> keys;
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0,999999);

    for (int i = 0; i < num_keys; i++)
        keys.push_back("key_" + std::to_string(dis(gen)));

    std::cout << "Generated " << num_keys << " keys\n";

    auto t1 = high_resolution_clock::now();
    for (auto &k : keys) {
        data_node dn("val_" + k, k);
        bigRing.insert_data(&dn);
    }
    auto t2 = high_resolution_clock::now();

    auto insert_ms = duration_cast<milliseconds>(t2 - t1).count();
    double throughput = num_keys / (insert_ms / 1000.0);

    std::cout << "Insert time: " << insert_ms << " ms\n";
    std::cout << "Throughput:  " << throughput << " ops/sec\n";
        std::cout << "\n=== Add Node Test ===\n";

    std::cout << "\n=== Tiny Add Node Test ===\n";

    std::cout << "\n=== Tiny Add Node Test (Multi Key) ===\n";

    Ring tiny2;
    tiny2.add_Node("A", 3);

    std::vector<std::string> keys1 = {"apple","banana","cherry","date","orange"};

    std::unordered_map<std::string,std::string> before;

    for (auto &k : keys1) {
        data_node dn(k,k);
        before[k] = tiny2.insert_data(&dn)->node_id;
    }

    tiny2.add_Node("B", 3);

    for (auto &k : keys1) {
        data_node dn(k,k);
        auto after = tiny2.insert_data(&dn)->node_id;

     std::cout << k
              << " moved: " << (before[k] != after ? "YES" : "NO")
              << "  " << before[k] << " -> " << after << "\n";
    }


    return 0;
}


