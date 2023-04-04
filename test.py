import requests
import queue
import threading
import time
import json
import binascii
from datetime import datetime
import random

URL = 'http://192.168.1.7:8081'
ADDRESS = 'bcnZKxVKM3aaa5qGKos5UVK4KP3eEny2QYZqWzLWktEr2CSEAQAJmnU11r3ms7q4vS5uEoXkGyvC6PqnX2MqrWB663MdpNCaQs'
USER = 'user'
PASSWORD = 'password'
THREADS = 6
INTERVAL = 120

def get_block_template(session, address):
    while True:
        try:
            r = session.post(url=f'{URL}/json_rpc', json={'jsonrpc': '2.0', 'id': '0', 'method': 'get_block_template', 'params': {'wallet_address': address}})
            r.raise_for_status()
            data = r.json()
            result = data['result']
            return result
        except Exception as e:
            print(f'An error occurred: {e}')
            time.sleep(INTERVAL)

def worker(session, address, lock):
    while True:
        task = task_queue.get()
        if task is None:
            task_queue.task_done()
            break
        selected_thread = random.randint(1, THREADS)
        if selected_thread == threading.current_thread().name:
            result = get_block_template(session, address)
            with lock:
                result_queue.put(result)
        task_queue.task_done()
        if task_queue.empty():
            time.sleep(INTERVAL)

if __name__ == '__main__':

    session = requests.Session()
    session.auth = (USER, PASSWORD)

    task_queue = queue.Queue()
    result_queue = queue.Queue()
    lock = threading.Lock()

    workers = []
    for i in range(THREADS):
        t = threading.Thread(target=worker, name=str(i+1), args=(session, ADDRESS, lock))
        t.start()
        workers.append(t)

    while True:
        task_queue.put(0)
        time.sleep(INTERVAL)

        result = get_block_template(session, ADDRESS)

    for i in range(THREADS):
        task_queue.put(None)

    task_queue.join()

    while not result_queue.empty():
        result = result_queue.get()
