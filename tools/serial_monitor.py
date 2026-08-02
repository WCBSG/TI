#!/usr/bin/env python3
"""
STC32 USB-CDC 串口调试帧监视器

用途：代替逐飞助手，读取 STC32 任务运行时发出的调试帧并实时显示。
数据格式（每行一个参数，task_sched.c 输出）：
    T=1.2   E=-3   D1=1420  D2=-1500  E1=12  E2=-10  BASE=1500  SO=-45
    [球稳任务额外] B=  TGT=  PX=  V=  SD=

用法：
    py tools/serial_monitor.py                # 自动找端口，默认 115200
    py tools/serial_monitor.py --port COM5     # 指定端口
    py tools/serial_monitor.py --baud 115200   # 指定波特率
    py tools/serial_monitor.py --send "T2"     # 打开后先发命令（启动任务），然后持续监听
    py tools/serial_monitor.py --stop-same 5   # 连续 5 帧相同则自动退出（默认）
    py tools/serial_monitor.py --stop-same 0   # 关闭相同帧停止
    py tools/serial_monitor.py --stop-idle 3   # 3s 无新帧（任务结束/车停）自动退出（默认）
    py tools/serial_monitor.py --stop-idle 0   # 关闭无数据停止，一直监听
    py tools/serial_monitor.py --log out.txt   # 同时记录到文件
    py tools/serial_monitor.py --list          # 只列出端口

交互：
    Ctrl+C  退出
    连续 N 帧内容相同（去时间戳）→ 自动退出
    超 N 秒无新帧（任务结束/车停）→ 自动退出
"""
import sys, time, argparse, datetime
import serial
import serial.tools.list_ports

# 关键字段的颜色（Windows 控制台无 ANSI 时自动降级）
COLOR = {
    "T":    "\033[36m",   # 青：时间
    "E":    "\033[33m",   # 黄：红外偏差
    "D1":   "\033[32m",   # 绿：左轮 duty
    "D2":   "\033[32m",   # 绿：右轮 duty
    "E1":   "\033[35m",   # 紫：左编码器
    "E2":   "\033[35m",   # 紫：右编码器
    "BASE": "\033[0m",
    "SO":   "\033[31m",   # 红：差速 PID 输出
    "B":    "\033[36m",   # 球位置
    "TGT":  "\033[36m",
    "PX":   "\033[36m",
    "V":    "\033[36m",
    "SD":   "\033[31m",
    "GZ":   "\033[31m",   # 陀螺仪角速度
    "YAW":  "\033[33m",   # 航向角（度）
    "IR":   "\033[36m",   # 红外 8 位位串
    "PH":   "\033[35m",   # 阶段号
    "M":    "\033[35m",   # 里程 cm
}
RESET = "\033[0m"


def find_stc_port():
    """找 STC32 的 USB-CDC 口：优先描述含 STC/USB-CDC/CDC 的，否则取第一个 USB 串口"""
    ports = list(serial.tools.list_ports.comports())
    cand = []
    for p in ports:
        d = (p.description or "").lower()
        if "stc" in d or "usb-cdc" in d or "cdc" in d or "usb serial" in d:
            cand.append(p)
    if not cand and ports:
        cand = [ports[0]]
    return cand


def parse_frame(text):
    """把一行 'KEY=val KEY=val...' 解析成 dict"""
    kv = {}
    for tok in text.split():
        if "=" in tok:
            k, v = tok.split("=", 1)
            kv[k.strip()] = v.strip()
    return kv


def colorize(kv):
    """按字段上色，拼一行可读输出"""
    parts = []
    for k in sorted(kv):
        v = kv[k]
        c = COLOR.get(k, "")
        if k == "T":
            parts.append(f"{c}T={v}{RESET}")      # 时间保持原始格式
        elif v.startswith("-"):
            parts.append(f"{c}{k}={v}{RESET}")
        else:
            parts.append(f"{c}{k}={v}{RESET}")
    return "  ".join(parts)


def main():
    ap = argparse.ArgumentParser(description="STC32 USB-CDC 调试帧监视器")
    ap.add_argument("--port", help="串口名，如 COM5；缺省自动找")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--send", help="打开端口后先发送的命令（如 IRTEST\\n），然后持续监听")
    ap.add_argument("--log", help="同时把原始输出写到此文件")
    ap.add_argument("--list", action="store_true", help="只列出端口后退出")
    ap.add_argument("--raw", action="store_true", help="不解析不上色，原样显示")
    ap.add_argument("--stop-same", type=int, default=5,
                    help="连续 N 帧内容相同（去时间戳）则自动退出；0=不启用")
    ap.add_argument("--stop-idle", type=float, default=3.0,
                    help="超过 N 秒无新帧则判定任务结束自动退出；0=不启用")
    args = ap.parse_args()

    if args.list:
        for p in serial.tools.list_ports.comports():
            print(p.device, "|", p.description)
        return

    port = args.port
    if not port:
        cand = find_stc_port()
        if not cand:
            print("[!] 未找到串口。请插上小车的 USB 线后重试，或用 --port 指定。")
            sys.exit(1)
        port = cand[0].device
        print(f"[*] 自动选择: {port} ({cand[0].description})")
    else:
        print(f"[*] 指定端口: {port}")

    logf = open(args.log, "a", encoding="utf-8") if args.log else None

    ser = None
    try:
        ser = serial.Serial(port, args.baud, timeout=0.2)
    except serial.SerialException as e:
        print(f"[!] 打开 {port} 失败: {e}")
        print("    可能是端口被占用（逐飞助手还在开？）或设备未就绪。")
        sys.exit(1)

    print(f"[*] 已连接 {port} @ {args.baud}，Ctrl+C 退出\n")

    if args.send:
        cmd = args.send.encode("utf-8", "ignore")
        if not cmd.endswith(b"\n"):
            cmd += b"\n"
        ser.write(cmd)
        ser.flush()
        print(f"[*] 已发送: {cmd!r}")

    buf = b""
    last_frame = None      # 上一帧内容（去时间戳）
    same_cnt = 0           # 连续相同帧计数
    last_activity = time.time()   # 最后收到数据的时间

    try:
        while True:
            data = ser.read(256)
            if not data:
                # 无数据超时：任务结束/车停后数据流停止
                if args.stop_idle > 0 and time.time() - last_activity > args.stop_idle:
                    print(f"\n[*] {args.stop_idle:.0f}s 无新帧（任务结束/车停），自动退出")
                    return
                continue
            last_activity = time.time()
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.replace(b"\r", b"").strip()
                if not line:
                    continue
                text = line.decode("utf-8", "ignore")
                ts = time.strftime("%H:%M:%S")
                if args.raw:
                    print(f"{ts}  {text}")
                else:
                    kv = parse_frame(text)
                    if kv:
                        print(f"{ts}  {colorize(kv)}")
                    else:
                        print(f"{ts}  [非调试帧] {text}")
                if logf:
                    logf.write(f"{ts}  {text}\n")
                    logf.flush()

                # 提前停止：连续 N 帧内容相同（去 T 时间字段）→ 任务已结束/车已停
                if args.stop_same > 0:
                    kv = parse_frame(text) if not args.raw else {}
                    if kv and "T" in kv:
                        del kv["T"]                     # 去掉时间字段再比较
                        cur = " ".join(f"{k}={v}" for k in sorted(kv))
                    else:
                        cur = text                      # 非调试帧/raw：整行比较
                    if cur == last_frame:
                        same_cnt += 1
                        if same_cnt >= args.stop_same:
                            print(f"\n[*] 连续 {same_cnt} 帧相同（数据已稳定），自动退出")
                            return
                    else:
                        same_cnt = 0
                    last_frame = cur
    except KeyboardInterrupt:
        print("\n[*] 退出")
    finally:
        if ser:
            ser.close()
        if logf:
            logf.close()


if __name__ == "__main__":
    main()
