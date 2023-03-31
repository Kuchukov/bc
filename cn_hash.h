#include <cstdint>
#include <cstring>
#include <vector>

std::vector<std::uint8_t> cn_hash(const std::vector<std::uint8_t>& input_data, const std::vector<std::uint8_t>& state)
{
    std::vector<std::uint8_t> s(state);
    
    for (std::size_t i = 0; i < input_data.size(); i += 8) {
        std::uint64_t chunk;
        std::memcpy(&chunk, &input_data[i], sizeof(chunk));
        std::vector<std::uint64_t> words{chunk};

        for (std::size_t j = 0; j < 0x400; ++j) {
            s[0] ^= (0xff & words[0]);
            s = cn_hash_permutation(s);
        }

        std::vector<std::uint8_t> hash_bytes;
        for (std::size_t k = 0; k < 64; ++k) {
            hash_bytes.push_back((s[k / 8] >> 8 * (k % 8)) & 0xff);
        }
        s.clear();
        for (std::size_t k = 0; k < 8; ++k) {
            s.push_back(hash_bytes[k * 8]);
            s.push_back(hash_bytes[k * 8 + 1]);
            s.push_back(hash_bytes[k * 8 + 2]);
            s.push_back(hash_bytes[k * 8 + 3]);
            s.push_back(hash_bytes[k * 8 + 4]);
            s.push_back(hash_bytes[k * 8 + 5]);
            s.push_back(hash_bytes[k * 8 + 6]);
            s.push_back(hash_bytes[k * 8 + 7]);
        }
    }

    return s;
}

std::vector<std::uint8_t> cn_hash_permutation(const std::vector<std::uint8_t>& s)
{
    std::vector<std::uint8_t> state(s);
    
    state[0] ^= state[4];
    state[4] ^= state[6];
    state[6] ^= state[2];
    state[2] ^= state[0];
    state[0] ^= state[5];
    state[4] ^= state[7];
    state[6] ^= state[1];
    state[2] ^= state[3];
    state[3] ^= state[7];
    state[7] ^= state[5];
    state[1] ^= state[5];
    state[5] ^= state[3];
    state[3] ^= state[1];
    state[1] ^= state[6];
    state[5] ^= state[7];
    state[3] ^= state[4];
    state[7] ^= state[2];
    state[1] ^= state[2];
    state[5] ^= state[0];
    state[3] ^= state[6];
    state[7] ^= state[4];
    state[1] ^= state[4];
    state[5] ^= state[2];
    state[3] ^= state[0];
    state[7] ^= state[6];

    return state;
}
