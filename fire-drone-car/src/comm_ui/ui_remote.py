#!/usr/bin/env python3
"""
ui_remote_curses.py
- FireDroneCar 원격 UI (curses 기반)
- 상태는 위에 대시보드처럼 갱신되고,
  아래 cmd> 줄에서 입력하는 동안 화면이 깨지지 않게 처리.

윈도우:
    pip install windows-curses
사용 예:
    python ui_remote_curses.py --host 172.30.121.144 --port 5000
"""

import socket
import threading
import argparse
import sys
import time
import curses


def parse_status_line(line: str) -> dict:
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
                try:
                    r, c = val.split(",", 1)
                    state["HOT_ROW"] = int(r)
                    state["HOT_COL"] = int(c)
                except Exception:
                    pass
            elif key == "ESTOP":
                state["ESTOP"] = (val == "1")
    except Exception:
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

        self.input_buffer = ""    # 현재 입력 중인 명령

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        self.running = True

    def close(self):
        self.running = False
        try:
            if self.sock:
                self.sock.close()
        except Exception:
            pass

    def recv_loop(self):
        """
        서버로부터 상태 문자열 수신.
        화면을 건드리지 않고 latest_state만 갱신.
        """
        buf = b""
        try:
            while self.running:
                data = self.sock.recv(4096)
                if not data:
                    # 연결 끊김
                    with self.lock:
                        self.latest_state = {"raw": "[disconnected]"}
                    self.running = False
                    break
                buf += data

                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    line = line.decode("utf-8", errors="ignore").strip()
                    if not line:
                        continue
                    state = parse_status_line(line)
                    with self.lock:
                        self.latest_state = state
        except Exception as e:
            with self.lock:
                self.latest_state = {"raw": f"[recv error] {e}"}
        finally:
            self.close()

    def send_command(self, cmd: str):
        if not self.running or not self.sock:
            return
        try:
            msg = (cmd.strip() + "\n").encode("utf-8")
            self.sock.sendall(msg)
        except Exception:
            self.close()

    # -------- curses 관련 --------

    def draw(self, stdscr):
        """
        curses 메인 루프.
        - 이 함수가 화면 전체 그리기 + 키 입력 처리를 전담.
        - recv_thread는 latest_state만 업데이트.
        """
        curses.curs_set(1)        # 커서 표시
        stdscr.nodelay(True)      # getch 비블록
        stdscr.timeout(100)       # 100ms마다 wake

        while True:
            stdscr.erase()
            max_y, max_x = stdscr.getmaxyx()

            # 상태 읽기
            with self.lock:
                st = dict(self.latest_state) if self.latest_state else None

            # 상단: 연결 상태 / 호스트 정보
            title = f"FireDroneCar Remote UI  |  {self.host}:{self.port}"
            stdscr.addstr(0, 0, title[:max_x - 1])

            stdscr.hline(1, 0, "-", max_x)

            # 상태 표시
            if st is None:
                stdscr.addstr(2, 0, "상태 수신 대기 중...", curses.A_DIM)
            else:
                mode = st.get("MODE", "?")
                wlv = st.get("WLV")
                t_fire = st.get("T_FIRE")
                dT = st.get("DT")
                dist = st.get("D")
                hot_r = st.get("HOT_ROW")
                hot_c = st.get("HOT_COL")
                estop = st.get("ESTOP")

                stdscr.addstr(2, 0, f"MODE : {mode:<10}   WLV : {wlv if wlv is not None else '?'}")
                if t_fire is not None and dT is not None:
                    stdscr.addstr(3, 0, f"T_FIRE : {t_fire:6.1f} °C    ΔT : {dT:6.1f} °C")
                else:
                    stdscr.addstr(3, 0, "T_FIRE :    ?.? °C    ΔT :    ?.? °C")
                if dist is not None:
                    stdscr.addstr(4, 0, f"DIST  : {dist:6.2f} m     HOT : ({hot_r}, {hot_c})")
                else:
                    stdscr.addstr(4, 0, f"DIST  :    ?.?? m     HOT : ({hot_r}, {hot_c})")
                stdscr.addstr(5, 0, f"ESTOP : {'ON ' if estop else 'OFF'}")

                raw = st.get("raw", "")
                stdscr.addstr(7, 0, f"RAW : {raw[:max_x - 6]}")

            stdscr.hline(max_y - 4, 0, "-", max_x)
            stdscr.addstr(max_y - 3, 0, "명령 예시: START / STOP / ESTOP / SET_WLV 2 / EXIT")

            # 입력 줄
            prompt = "cmd> "
            input_line = prompt + self.input_buffer
            stdscr.addstr(max_y - 1, 0, input_line[:max_x - 1])

            # 커서 위치 이동
            curses.setsyx(max_y - 1, len(prompt) + len(self.input_buffer))
            stdscr.refresh()

            # 종료 조건
            if not self.running:
                # 연결이 끊긴 상태에서도 마지막으로 입력 한 번 처리하고 나갈 수 있음
                # 여기서는 바로 break
                break

            # 키 입력 처리
            try:
                ch = stdscr.getch()
            except KeyboardInterrupt:
                break

            if ch == -1:
                continue

            # 엔터
            if ch in (curses.KEY_ENTER, 10, 13):
                cmd = self.input_buffer.strip()
                if cmd:
                    if cmd.upper() in ("EXIT", "QUIT"):
                        self.send_command(cmd)
                        break
                    else:
                        self.send_command(cmd)
                self.input_buffer = ""
                continue

            # 백스페이스
            if ch in (curses.KEY_BACKSPACE, 127, 8):
                if self.input_buffer:
                    self.input_buffer = self.input_buffer[:-1]
                continue

            # 기타 문자 입력
            if 32 <= ch <= 126:  # printable ASCII
                self.input_buffer += chr(ch)
                continue

            # 그 외 키는 무시
            continue

        # 루프 종료 시
        self.close()


def curses_main(stdscr, ui: RemoteUI):
    ui.draw(stdscr)


def main():
    parser = argparse.ArgumentParser(description="FireDroneCar remote UI (curses)")
    parser.add_argument("--host", default="172.30.121.144",
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

    # 수신 스레드 시작 (상태만 갱신)
    t = threading.Thread(target=ui.recv_loop, daemon=True)
    t.start()

    # curses UI 시작
    curses.wrapper(curses_main, ui)


if __name__ == "__main__":
    main()
