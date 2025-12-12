import socket
import threading
import sys

HOST = "172.30.121.144"   # ← 여기 Pi IP 그대로 사용
PORT = 5000

def recv_loop(sock):
    """서버에서 오는 데이터 계속 출력"""
    try:
        while True:
            data = sock.recv(4096)
            if not data:
                print("\n[disconnected]")
                break
            print(data.decode("utf-8"), end="")
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
        sys.exit(0)

def send_loop(sock):
    """키보드에서 입력한 명령을 서버로 전송"""
    try:
        while True:
            line = input()
            sock.sendall((line + "\n").encode("utf-8"))
    except (KeyboardInterrupt, EOFError):
        print("\n[exit]")
        sock.close()
        sys.exit(0)

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    print(f"[connected] {HOST}:{PORT}")
    
    t = threading.Thread(target=recv_loop, args=(sock,), daemon=True)
    t.start()
    
    send_loop(sock)

if __name__ == "__main__":
    main()
