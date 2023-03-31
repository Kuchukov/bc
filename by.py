import argparse
import requests
import json
from cn_hash import cn_hash
import time
import threading

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--url', default='http://localhost:8081', help='API-RPC Node URL')
    parser.add_argument('--address', required=True, help='Wallet address')
    parser.add_argument('--user', default='', help='API-RPC user')
    parser.add_argument('--password', default='', help='API-RPC password')
    parser.add_argument('--interval', type=int, default=33, help='Interval between requests in sec.')
    parser.add_argument('--threads', type=int, default=1, help='Number of threads to use')
    args = parser.parse_args()

    headers = {'Content-Type': 'application/json'}

    if args.user and args.password:
        auth = (args.user, args.password)
    else:
        auth = None

    previous_block_hash = ''
    lock = threading.Lock()
    print(args)

    def worker(start_nonce, end_nonce):
        nonlocal previous_block_hash
        nonce = start_nonce
        while nonce < end_nonce:
            try:
                payload = {
                    'method': 'get_block_template',
                    'params': {'wallet_address': args.address},
                    'jsonrpc': '2.0',
                    'id': 0
                }
                request_json = json.dumps(payload)
                response = requests.post(args.url + '/json_rpc', headers=headers, auth=auth, data=request_json)
                response.raise_for_status()
                result = response.json().get('result')

                if result:
                    with lock:
                        if result['previous_block_hash'] != previous_block_hash:
                            print(f'PRIVIOUS BLOCK HASH: {result["previous_block_hash"]}')
                            new_previous_block_hash = result['previous_block_hash']
                            if new_previous_block_hash != previous_block_hash:
                               previous_block_hash = new_previous_block_hash

                    block_header = bytes.fromhex(previous_block_hash) if previous_block_hash else b''
                    while nonce < end_nonce:
                        nonce_bytes = nonce.to_bytes(4, byteorder='little')
                        block_header_with_nonce = block_header + nonce_bytes
                        new_block_hash = cn_hash(block_header_with_nonce, state=bytes.fromhex(result.get('blocktemplate_blob', '')))
                        if int.from_bytes(new_block_hash, byteorder='little') < result.get('difficulty', 0):
                            with lock:
                                if new_block_hash.hex() != previous_block_hash:
                                    print(f'NEW BLOCK HASH FOUND: {new_block_hash.hex()}')
                                    try:
                                        submit_block(args.url, result.get('blocktemplate_blob', ''))
                                    except Exception as e:
                                        print(f"Error submitting block: {e}")
                                    previous_block_hash = new_block_hash.hex()
                                    break
                        nonce += 1
                else:
                    print("No result returned from the node.")
            except requests.exceptions.RequestException as e:
                print(f"Error sending request: {e}")

            time.sleep(args.interval)

    threads = []
    for i in range(args.threads):
        start_nonce = (2**32 // args.threads) * i
        end_nonce = (2**32 // args.threads) * (i + 1)
        t = threading.Thread(target=worker, args=(start_nonce, end_nonce))
        t.daemon = True
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

def submit_block(url, blocktemplate_blob):
    headers = {'Content-Type': 'application/json'}
    payload = {
        'method': 'submit_block',
        'params': [blocktemplate_blob],
        'jsonrpc': '2.0',
        'id': 0
    }
    request_json = json.dumps(payload)
    response = requests.post(url + '/json_rpc', headers=headers, data=request_json)
    response.raise_for_status()
    result = response.json().get('result')
    if result:
        print(f"Block submitted: {result}")
    else:
        print(f"Error submitting block: {response.json().get('error')}")

if __name__ == "__main__":
    main()
