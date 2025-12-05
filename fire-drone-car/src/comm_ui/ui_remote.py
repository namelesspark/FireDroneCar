#!/usr/bin/env python3

import socket
import threading
import argparse
import sys
import time


def parse_status_line(line: str) -> dict:
    """
    MODE=EXT;WLV=2;T_FIRE=120.5;DT=30.2;D=0.35;HOT=4,7;ESTOP=0
    이런 문자열을 dict로 파싱.
    문제가 생기면 raw만 담아서 반환.
    """
    state = {
        "raw": line.strip(),
        "MODE": "?",
        "WLV": None,
        "T_FIRE": None,
        "DT": None,
        "D": None,
        "HOT_ROW": None,
        "HOT_COL": None,
        "ESTOP": None,
    }

    try:
        parts = [p for p in line.strip().split(";") if p]
        for p in parts:
            if "=" not in p:
                continue
            key, val = p.split("=", 1)
            key = key.strip().upper()
            val = val.strip()

            if key == "MODE":
                state["MODE"] = val
            elif key == "WLV":
                state["WLV"] = int(val)
            elif key in ("T_FIRE", "TFIRE", "T"):
                state["T_FIRE"] = float(val)
            elif key == "DT":
                state["DT"] = float(val)
            elif key in ("D", "DIST", "DISTANCE"):
                state["D"] = float(val)
            elif key == "HOT":
                # HOT=4,7 같은 형태
                try:
                    r, c = val.split(",", 1)
                    state["HOT_ROW"] = int(r)
                    state["HOT_COL"] = int(c)
                except Exception:
                    pass
            elif key == "ESTOP":
                state["ESTOP"] = (val == "1")

    except Exception:
        # 파싱 실패 시 raw만 보여주면 됨
        pass

    return state


class RemoteUI:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port

        self.sock = None
        self.running = False

        self.lock = threading.Lock()
        self.latest_state = None  # dict

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        self.running = True
        print(f"[connected] {self.host}:{self.port}")

    def close(self):
        self.running = False
        try:
            if self.sock:
                self.sock.close()
        except Exception:
            pass

    def recv_loop(self):
        """
        서버에서 오는 상태 문자열 계속 읽어서 latest_state에 갱신.
        """
        buf = b""
        try:
            while self.running:
                data = self.sock.recv(4096)
                if not data:
                    print("\n[disconnected by server]")
                    self.running = False
                    break
                buf += data

                # 줄 단위 처리
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    line = line.decode("utf-8", errors="ignore").strip()
                    if not line:
                        continue

                    state = parse_status_line(line)
                    with self.lock:
                        self.latest_state = state

                    # 상태 한 번 출력
                    self.print_status_once(state)

        except Exception as e:
            print(f"\n[recv error] {e}")
        finally:
            self.close()

    def print_status_once(self, state: dict):
        """
        상태 한 줄을 보기 좋게 출력.
        """
        mode = state.get("MODE", "?")
        wlv = state.get("WLV")
        t_fire = state.get("T_FIRE")
        dT = state.get("DT")
        dist = state.get("D")
        hot_r = state.get("HOT_ROW")
        hot_c = state.get("HOT_COL")
        estop = state.get("ESTOP")

        line1 = f"MODE={mode:<10}  WLV={wlv if wlv is not None else '?'}"
        line2 = "T_FIRE={:.1f}°C  ΔT={:.1f}°C".format(
            t_fire if t_fire is not None else float("nan"),
            dT if dT is not None else float("nan"),
        ) if t_fire is not None and dT is not None else "T_FIRE=?, ΔT=?"
        line3 = "D={:.2f} m  HOT=({}, {})".format(
            dist if dist is not None else float("nan"),
            hot_r if hot_r is not None else -1,
            hot_c if hot_c is not None else -1,
        ) if dist is not None else f"D=?, HOT=({hot_r},{hot_c})"
        line4 = f"ESTOP={'ON' if estop else 'OFF'}"

        print()
        print("=== STATUS ===")
        print(line1)
        print(line2)
        print(line3)
        print(line4)
        print(f"RAW: {state.get('raw', '')}")
        print("==============")
        print("cmd> ", end="", flush=True)

    def send_command(self, cmd: str):
        if not self.running or not self.sock:
            print("[warn] not connected")
            return
        try:
            msg = (cmd.strip() + "\n").encode("utf-8")
            self.sock.sendall(msg)
        except Exception as e:
            print(f"[send error] {e}")
            self.close()


def main():
    parser = argparse.ArgumentParser(description="FireDroneCar remote UI")
    parser.add_argument("--host", default="127.0.0.1",
                        help="라즈베리파이 IP 주소 (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=5000,
                        help="포트 번호 (default: 5000)")
    args = parser.parse_args()

    ui = RemoteUI(args.host, args.port)

    try:
        ui.connect()
    except Exception as e:
        print(f"[connect error] {e}")
        return

    # 수신 스레드 시작
    t = threading.Thread(target=ui.recv_loop, daemon=True)
    t.start()

    # 메인 스레드는 명령 입력 루프
    print("사용 예: START / STOP / ESTOP / SET_WLV 2 등")
    print("CTRL+C 로 종료")
    print("cmd> ", end="", flush=True)

    try:
        while ui.running:
            cmd = input()
            if not cmd:
                print("cmd> ", end="", flush=True)
                continue
            if cmd.upper() in ("QUIT", "EXIT"):
                print("[info] exit command")
                break
            ui.send_command(cmd)
            # 다음 입력 프롬프트
            print("cmd> ", end="", flush=True)
    except KeyboardInterrupt:
        print("\n[KeyboardInterrupt] exit")
    finally:
        ui.close()
        time.sleep(0.2)


if __name__ == "__main__":
    main()
