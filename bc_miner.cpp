#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdint.h>
#include "cryptonight.h"
#include "cn_slow_hash.hpp"
#include "net/http_client.h"
#include "serialization/json_utils.h"

using namespace std;

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cout << "Usage: ./bytecoin_miner <node_address> <wallet_address>" << endl;
        return 1;
    }

    // Node address
    string node_address = argv[1];

    // Wallet address
    string wallet_address = argv[2];

    // Seed
    string seed_str = "random_seed";
    uint8_t seed[32];
    memcpy(seed, seed_str.c_str(), seed_str.size());

    // Nonce
    uint32_t nonce = 0;

    // Hash
    uint8_t hash[32];

    while (true) {

        // Concatenate seed and nonce
        uint8_t input[40];
        memcpy(input, seed, 32);
        memcpy(input + 32, &nonce, 4);

        // Hash using Cryptonight algorithm
        cn_slow_hash(input, 40, hash, 0);

        // Check if the hash meets the difficulty requirement
        if (hash[0] == 0 && hash[1] == 0 && hash[2] == 0) {

            // Serialize hash to string
            char hash_str[65];
            for (int i = 0; i < 32; i++) {
                sprintf(hash_str + (i * 2), "%02x", hash[i]);
            }
            hash_str[64] = '\0';

            // Submit the hash to the node for verification and credit
            HttpClient client(node_address);
            string response;
            int status_code = client.request("/", "{\"method\":\"submit\", \"params\":{\"wallet_address\":\"" + wallet_address + "\",\"hash\":\"" + string(hash_str) + "\"}}", response);

            // Check if the request was successful
            if (status_code == 200) {
                // Parse response
                JsonValue root;
                JsonReader reader;
                reader.parse(response.c_str(), response.size(), root);
                JsonObject obj;
                if (root.isObject() && root.getObject("result", obj)) {
                    if (obj.getString("status") == "OK") {
                        cout << "Hash " << hash_str << " submitted successfully to node " << node_address << " for credit to wallet " << wallet_address << endl;
                    } else {
                        cout << "Error submitting hash " << hash_str << " to node " << node_address << ": " << obj.getString("error") << endl;
                    }
                } else {
                    cout << "Invalid response received from node " << node_address << endl;
                }
            } else {
                cout << "Error submitting hash " << hash_str << " to node " << node_address << ": HTTP status code " << status_code << endl;
            }

            // Reset nonce
            nonce = 0;

        }

        // Increment nonce
        nonce++;

    }

    return 0;
}
