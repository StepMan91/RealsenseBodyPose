import socket
import json
import time

def start_server(host='127.0.0.1', port=8888):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host, port))
    print(f"Listening on {host}:{port}")
    
    start_time = time.time()
    packet_count = 0
    
    sock.settimeout(2.0) # 2 seconds timeout
    
    try:
        while time.time() - start_time < 15: # Run for 15 seconds
            try:
                data, addr = sock.recvfrom(65535)
                packet_count += 1
                message = data.decode('utf-8')
                try:
                    json_data = json.loads(message)
                    print(f"[{packet_count}] Received {len(message)} bytes from {addr}")
                    # Print first few chars of data to verify structure
                    print(f"Data sample: {str(json_data)[:100]}...")
                    
                    if "skeletons" in json_data:
                         skels = json_data["skeletons"]
                         print(f"Contains {len(skels)} skeletons")
                    
                except json.JSONDecodeError:
                    print(f"Received invalid JSON: {message[:50]}...")
            except socket.timeout:
                continue
    except KeyboardInterrupt:
        print("Stopped by user")
    finally:
        sock.close()
        print(f"Finished. Total packets: {packet_count}")

if __name__ == '__main__':
    start_server()
