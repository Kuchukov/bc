#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdint.h>
#include <thread>
#include "cryptonight.h"
#include <curl/curl.h>
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

// Helper function to convert a uint8_t array to a hex string
string to_hex_string(uint8_t *data, size_t length) {
    string hex_string;
    for (size_t i = 0; i < length; i++) {
        char hex[3];
        sprintf(hex, "%02x", data[i]);
        hex_string += hex;
    }
    return hex_string;
}

// Helper function to submit a hash to the node
bool submit_hash(string node_address, string wallet_address, uint8_t *hash) {
    // Serialize hash to string
    string hash_str = to_hex_string(hash, 32);

    // Create JSON request data
    json request_data;
    request_data["method"] = "submit";
    request_data["params"]["wallet_address"] = wallet_address;
    request_data["params"]["hash"] = hash_str;

    // Serialize JSON request data to string
    string request_str = request_data.dump();

    // Initialize libcurl
    CURL *curl = curl_easy_init();
    if (!curl) {
        cerr << "Error: could not initialize libcurl" << endl;
        return false;
    }

    // Set libcurl options
    curl_easy_setopt(curl, CURLOPT_URL, node_address.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_str.size());
    curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        cerr << "Error: could not perform HTTP request: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return false;
    }

    // Check the response
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        cerr << "Error: invalid HTTP response code: " << http_code << endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return false;
    }

    // Cleanup
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return true;
}

// Function for each thread to mine hashes
void mine_hashes(uint8_t *target_blob, uint8_t *result_blob, uint32_t target_difficulty, uint64_t nonce_start, uint64_t nonce_range, string node_address, string wallet_address) {
    uint64_t nonce = nonce_start;
    uint32_t result_difficulty = 0xffffffff;
    while (nonce < nonce_start + nonce_range) {
        // Update nonce in target blob
        *((uint64_t *)(target_blob + 39)) = nonce;

        // Hash the target blob
        uint8_t hash[32];
        cn_slow_hash(target_blob, 76, hash);

        // Check if hash meets target difficulty
        uint32_t difficulty = *((uint32_t *)hash);
        if (difficulty <= target_difficulty && difficulty < result_difficulty) {
            // Found a new result with a lower difficulty
            result_difficulty = difficulty;

            // Copy result blob
            memcpy(result_blob, target_blob, 76);

            // Append extra nonce to result blob
            uint64_t extra_nonce = rand();
            *((uint64_t *)(result_blob + 39)) = extra_nonce;

            // Submit result to node
            string json_data = "{ \"jsonrpc\": \"2.0\", \"id\": 0, \"method\": \"submit_block\", \"params\": [\"" + string(result_blob, result_blob + 76) + "\"] }";
            submit_block(node_address, wallet_address, json_data);
        }

        nonce++;
    }
}

// Function to start mining
void start_mining(string node_address, string wallet_address, string miner_address, uint32_t difficulty, uint32_t threads) {
cout << "Starting mining with " << threads << " threads" << endl;

// Create target blob
uint8_t target_blob[76];
memset(target_blob, 0, 76);
memcpy(target_blob, miner_address.c_str(), min(miner_address.size(), (size_t)64));

// Initialize target blob with random data
for (int i = 39; i < 43; i++) {
    *((uint64_t *)(target_blob + i)) = rand();
}

// Set target difficulty
*((uint32_t *)(target_blob + 43)) = difficulty;

// Create result blob
uint8_t result_blob[76];
memset(result_blob, 0, 76);
memcpy(result_blob, miner_address.c_str(), min(miner_address.size(), (size_t)64));

// Initialize libcurl
curl_global_init(CURL_GLOBAL_DEFAULT);

// Start mining threads
thread *mining_threads = new thread[threads];
uint64_t nonce_per_thread = UINT64_MAX / threads;
for (uint32_t i = 0; i < threads; i++) {
    mining_threads[i] = thread(mine_hashes, target_blob, result_blob, difficulty, i * nonce_per_thread, nonce_per_thread, node_address, wallet_address);
}

// Wait for mining threads to finish
for (uint32_t i = 0; i < threads; i++) {
    mining_threads[i].join();
}

// Cleanup libcurl
curl_global_cleanup();

}

int main() {
// Initialize libcurl
curl_global_init(CURL_GLOBAL_DEFAULT);

// Get mining parameters from node
string node_address = "http://localhost:8080";
string wallet_address = "WALLET_ADDRESS";
string miner_address = "MINER_ADDRESS";
uint32_t difficulty = 10000;
uint32_t threads = 4;
string params_json = "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"get_mining_params\",\"params\":[]}";
string params_response = send_rpc_request(node_address, params_json);
json params_data = json::parse(params_response)["result"];
if (params_data.is_null()) {
    cerr << "Error: could not get mining parameters from node" << endl;
    return 1;
}
difficulty = params_data["difficulty"];
threads = params_data["threads"];

// Start mining
start_mining(node_address, wallet_address, miner_address, difficulty, threads);

// Cleanup libcurl
curl_global_cleanup();

return 0;

}
