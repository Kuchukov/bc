def cn_hash(input_data, state):
    state = bytearray(state)
    
    for i in range(0, len(input_data), 8):
        chunk = input_data[i:i + 8]
        words = [int.from_bytes(chunk, byteorder='little')]

        for j in range(0, 0x400):
            state[0] ^= (0xff & words[0])
            state = cn_hash_permutation(state)

        hash_bytes = bytes([state[i // 8] >> 8 * (i % 8) & 0xff for i in range(64)])
        state = [int.from_bytes(hash_bytes[i:i + 8], byteorder='little') for i in range(0, 64, 8)]

    hash_str = ''.join([format(state[i], '08x') for i in range(8)])
    return bytes.fromhex(hash_str)

def cn_hash_permutation(state):
    state = bytearray(state)
    
    state[0] ^= state[4]
    state[4] ^= state[6]
    state[6] ^= state[2]
    state[2] ^= state[0]
    state[0] ^= state[5]
    state[4] ^= state[7]
    state[6] ^= state[1]
    state[2] ^= state[3]
    state[3] ^= state[7]
    state[7] ^= state[5]
    state[1] ^= state[5]
    state[5] ^= state[3]
    state[3] ^= state[1]
    state[1] ^= state[6]
    state[5] ^= state[7]
    state[3] ^= state[4]
    state[7] ^= state[2]
    state[1] ^= state[2]
    state[5] ^= state[0]
    state[3] ^= state[6]
    state[7] ^= state[4]
    state[1] ^= state[4]
    state[5] ^= state[2]
    state[3] ^= state[0]
    state[7] ^= state[6]
    
    return state
