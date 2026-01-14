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
std::array<unsigned char,MD5_DIGEST_LENGTH> computerMD5FromString(const std::string& str) {
    std::array<unsigned char,MD5_DIGEST_LENGTH> result; 
    MD5(reinterpret_cast<const unsigned char*>(str.data()),str.size(),result.data()); 
    return result; 
}

std::string hash_to_hex(const std::array<unsigned char, MD5_DIGEST_LENGTH>& hash) {
    std::ostringstream oss;
    for (unsigned char c : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

bool in_interval(const std::array<unsigned char,MD5_DIGEST_LENGTH>& k,
                 const std::array<unsigned char,MD5_DIGEST_LENGTH>& p,
                 const std::array<unsigned char,MD5_DIGEST_LENGTH>& v) 
{
    if (p < v) {
        return p < k && k <= v;
    } else { // wrap-around
        return k > p || k <= v;
    }
}


class data_node {
    public: 
        data_node() {
            throw std::invalid_argument("data_node must be created with key and data"); 
        }
        data_node(const std::string& Data, const std::string& Key) {
            data = Data; 
            hash_value = computerMD5FromString(Key); 
            key = hash_to_hex(hash_value); 
        }
        data_node& operator=(const data_node& other) {
            this->data = other.data;
            this->hash_value = other.hash_value;
            this->key = other.key; 
            return *this; 
        }
        ~data_node() = default; 
        std::string data; 
        std::string key; 
        std::array<unsigned char,MD5_DIGEST_LENGTH> hash_value; 
}; 

void print_hash(const std::array<unsigned char, MD5_DIGEST_LENGTH>& hash) {
    for (unsigned char byte : hash) {
        std::cout << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;
}

class virtual_node {
    public: 
        virtual_node() {
            hash_value = computerMD5FromString("");     
        }
        virtual_node(std::string node_id_plus, std::array<unsigned char,MD5_DIGEST_LENGTH> parent_node_hash) {
            hash_value = computerMD5FromString(node_id_plus); 
            this->parent_node_hash = parent_node_hash; 
        }

        virtual_node& operator=(const virtual_node& other) {
            this->hash_value = other.hash_value;
            this->parent_node_hash = other.parent_node_hash; 
            return *this; 
        }
        bool operator<(const virtual_node& other)const {
            return hash_value < other.hash_value; 
        }
        bool operator>(const virtual_node& other)const {
            return hash_value > other.hash_value; 
        }

        bool operator==(const virtual_node& other)const {
            return hash_value == other.hash_value; 
        }
        ~virtual_node() = default; 
        std::array<unsigned char,MD5_DIGEST_LENGTH> hash_value; 
        std::array<unsigned char,MD5_DIGEST_LENGTH> parent_node_hash; 
}; 

class Node {
    public: 
        std::array<unsigned char,MD5_DIGEST_LENGTH> hash_value; 
        std::string node_id;
        std::vector<std::unique_ptr<virtual_node>> virtual_nodes; 
        size_t num_v_nodes; 
        std::unordered_map<std::string, data_node> data; 
        Node(){
            throw std::invalid_argument("Node must be created with node_id"); 
        }
        Node(std::string node_id,size_t num_v_nodes) {
            this->node_id = node_id; 
            this->num_v_nodes = num_v_nodes; 
            hash_value = computerMD5FromString(node_id); 
            for(size_t i = 0; i < num_v_nodes; i++) {
                virtual_nodes.push_back(std::make_unique<virtual_node>(node_id + std::to_string(i),hash_value)); 
            }
        }
        

        ~Node() = default; 

        bool operator<(const Node& other)const {
            return hash_value < other.hash_value; 
        }
        bool operator>(const Node& other)const {
            return hash_value > other.hash_value; 
        }
        bool operator==(const Node& other)const {
            return hash_value == other.hash_value; 
        }
        // should store a list of pointers to virtual node 
        // create virtual nodes in constructor
}; 

// Define Ring DS
class Ring{
    public: 
        Ring() = default; 
        Ring(std::vector<Node*> Nodes) {
            for (Node* Np: Nodes) {
                for (auto &vn: Np->virtual_nodes) {
                    v_node_maps.insert({vn->hash_value,Np}); 
                }
            }
            num_nodes = Nodes.size(); 
        }
        ~Ring() = default; 
        Node* insert_data(data_node* dpp) { // Change return value to void after testing
            const int num_replications = 4; // placeholder value for how many replications we want 
            std::vector<Node*> node_locations = get_node_locations(dpp,num_replications); // TEST
            for (int i = 0; i< node_locations.size(); i++) { // TEST 
                node_locations[i]->data.emplace(dpp->key,*dpp); 
            }
            return node_locations.front(); 
        }

        std::vector<Node*> get_node_locations(data_node* dpp, int num_replications) {
            std::vector<Node*> node_loc; 
            std::set<Node*> node_hash; 
            num_replications = std::min(num_replications, (int)num_nodes);
            if(v_node_maps.empty()) {
                return node_loc; 
            }
            auto it = v_node_maps.upper_bound(dpp->hash_value);
            if(it == v_node_maps.end()){
                it = v_node_maps.begin(); 
            }
            while(node_loc.size() < num_replications && node_hash.size() < num_nodes) {
                Node* curr = it->second; 
                if(!node_hash.contains(curr)) {// gone full circle 
                    node_loc.push_back(curr);
                    node_hash.insert(curr);
                }
                it++;
                if(it == v_node_maps.end()) { // Gone to end of circle 
                    it = v_node_maps.begin(); 
                }
                // if at end iterator move to the begin iterator 
                // get current Node from map 
                // if already seen break 
                // else append to vector and ++ current_num and add into seen set and ++ iterator  
            }
            return node_loc; 
        }

        // returns value of data node that contains the key looking for. Return empty string if no key exists 
        std::string find_data(std::string& key){
            std::array<unsigned char,MD5_DIGEST_LENGTH> h_key = computerMD5FromString(key); 
            // find v_node map value for key 
            auto it = v_node_maps.upper_bound(h_key);
            if (it == v_node_maps.end()) { // Loop back if exceeds max 
                it = v_node_maps.begin(); 
            }
            auto des_node = it->second; 

            if(auto search = des_node->data.find(hash_to_hex(h_key));search != des_node->data.end()) {
                return search->second.data; 
            }
            return ""; 
        }

        void add_Node(const std::string& node_id, int num_v_nodes) {
            auto node_ptr = std::make_unique<Node>(node_id,num_v_nodes); 
            Node* tmp = node_ptr.get(); 
            for (auto& v: tmp->virtual_nodes){
                v_node_maps.insert({v->hash_value,tmp});
            }
            nodes.push_back(std::move(node_ptr)); 
            num_nodes++; 
            return; 
        }

        Node* successor_node_of_vnode(const std::array<unsigned char,MD5_DIGEST_LENGTH>& hash) {
            auto it = v_node_maps.upper_bound(hash);
            if (it == v_node_maps.end()) {
                it = v_node_maps.begin();
            }
            return it->second;
        }
        Node* predecessor_node_of_vnode(const std::array<unsigned char,MD5_DIGEST_LENGTH>& hash) {
            auto it = v_node_maps.lower_bound(hash);
            if (it == v_node_maps.begin()){
                it = v_node_maps.end();
            }
            --it; 
            return it->second;
        }



        void remove_Node(const std::string& node_id) {
            auto node_hashv = computerMD5FromString(node_id);
            Node* victim = nullptr;
            for(auto& node : nodes){
                if(node->hash_value == node_hashv){
                    victim = node.get();
                    break;
                }
            }
            if(!victim) throw std::invalid_argument("node not found");

            for(auto& vnode_ptr : victim->virtual_nodes) {
                auto& v = vnode_ptr->hash_value;
                Node* succ = successor_node_of_vnode(v);
                Node* pred = predecessor_node_of_vnode(v);
                if(succ == victim){ // only one node 
                    continue;
                }
                auto it = victim->data.begin();
                while(it != victim->data.end()) {
                    auto& key = it->first;
                    auto key_hash = computerMD5FromString(key);

                    if(in_interval(key_hash, pred->hash_value, v)) {
                        succ->data.emplace(key, it->second);
                        it = victim->data.erase(it);
                    } else {
                        ++it;
                    }
                }

                v_node_maps.erase(v);
            }

            nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
            [&](auto& p){ return p.get() == victim; }), nodes.end());
            num_nodes--;
        }

        std::map<std::array<unsigned char,MD5_DIGEST_LENGTH>,Node*> v_node_maps; 
        size_t num_nodes = 0; 
        std::vector<std::unique_ptr<Node>> nodes; 
    
}; 