"""OMV-RT5 WiFi MJPEG 图传 - AP: OMVRT-WC/12345678 -> http://192.168.4.1:8000/"""
import sensor, time, network, socket, errno

SSID, KEY, PORT = "OMVRT-WC", "12345678", 8000
W, H, Q, B = 640, 480, 45, "frame"

PAGE = ('<html><head><meta charset="utf-8"><title>OMV-RT5</title>'
        '<style>body{background:#000;margin:0;overflow:hidden}'
        '#l{position:fixed;top:12px;left:16px;color:#58a6ff;font:600 12px sans-serif;'
        'background:rgba(0,0,0,.5);padding:4px 10px;z-index:9}'
        'img{width:100vw;height:100vh;object-fit:contain}</style></head>'
        '<body><div id="l">&#9679; LIVE %dx%d</div>'
        '<img src="/stream"></body></html>' % (W, H))

# 流客户端表：c -> {pending: 未发完的 multipart 帧或 None, off: 已发偏移}
clients = {}

def drop(c, why):
  try: c.close()
  except: pass
  if c in clients:
    del clients[c]
    print("[off]", why, "streams=%d" % len(clients))

def handle_client(c):
  """读取请求行 → 路由。返回后 c 处于非阻塞（stream）或已关闭（其余）。"""
  c.setblocking(True)
  c.settimeout(0.5)                       # 请求读/前导发送用阻塞+超时，防 EWOULDBLOCK
  buf = b""
  try:
    while b"\r\n" not in buf:             # 循环读，凑齐请求行，防 TCP 分片
      d = c.recv(256)
      if not d: break
      buf += d
      if len(buf) > 2048: break
  except: pass

  parts = buf.split(b"\r\n", 1)[0].split(b" ", 2)
  p = parts[1] if len(parts) >= 2 else b""    # 路径 bytes 比较，避免 decode 抛 UnicodeError

  if p == b"/":
    try:
      body = PAGE.encode()
      c.sendall(("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                 "Content-Length: %d\r\n\r\n" % len(body)).encode() + body)
    except OSError as e:
      print("[err] page:", e)
    try: c.close()
    except: pass
  elif p == b"/stream":
    try:
      c.sendall(("HTTP/1.1 200 OK\r\nCache-Control: no-store\r\n"
                 "Content-Type: multipart/x-mixed-replace; boundary=%s\r\n\r\n" % B).encode())
    except OSError as e:                  # 前导没发出去 → 不挂进流表
      print("[err] stream hdr:", e)
      try: c.close()
      except: pass
      return
    try: c.setblocking(False)             # 只有前导发送成功，才切非阻塞挂进流表
    except: pass
    clients[c] = {"pending": None, "off": 0}
    print("[stream] +1, streams=%d" % len(clients))
  else:
    try: c.close()
    except: pass

def pump(jpg):
  """jpg=None 只 flush 存量 pending；否则给空闲客户端补一帧。
  发不完的帧保留在 pending，绝不在半截丢弃（保住 multipart 边界+Content-Length 配对）。"""
  for c in list(clients):
    st = clients[c]
    if jpg is not None and st["pending"] is None:
      st["pending"] = (b"--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n"
                       % (B, len(jpg))) + jpg + b"\r\n"   # 全部 bytes，勿加 .encode()
      st["off"] = 0
    buf, off = st["pending"], st["off"]
    if buf is None:
      continue
    while off < len(buf):
      try:
        n = c.send(buf[off:])
      except OSError as e:
        if e.errno == errno.EAGAIN:    # EWOULDBLOCK 与 EAGAIN 同值，部分移植版未定义前者
          st["off"] = off              # 缓冲满：留着，下次续发
          break
        drop(c, e); break
      if not n:
        drop(c, "closed"); break
      off += n
    if off >= len(buf):
      st["pending"], st["off"] = None, 0
    else:
      st["off"] = off

def main():
  wlan = network.WLAN(network.AP_IF)
  wlan.config(ssid=SSID, key=KEY, channel=2); wlan.active(True)
  ip = None
  for _ in range(50):
    ip = wlan.ifconfig()[0]
    if ip and ip != "0.0.0.0": break
    time.sleep_ms(100)
  if not ip or ip == "0.0.0.0": print("[ERR] wifi"); return

  sensor.reset(); sensor.set_pixformat(sensor.RGB565); sensor.set_framesize(sensor.VGA)   # 640x480（帧率低，WiFi 图传可能卡）
  sensor.skip_frames(time=2000)

  srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  srv.bind(["", PORT]); srv.listen(8); srv.setblocking(False)   # 大 backlog，扛多连接
  print("[RUN] http://%s:%d/" % (ip, PORT))

  while True:
    while True:                        # 排空 accept 队列：每循环处理所有待连连接
      try:
        c, _ = srv.accept()
      except OSError:
        break
      handle_client(c)

    if not clients:
      time.sleep_ms(20)                # 无观众：降速省 CPU，不抓帧
      continue

    if any(st["pending"] is None for st in clients.values()):
      try:
        jpg = sensor.snapshot().compress(quality=Q)
      except Exception:
        continue                       # 单帧失败跳过本帧，图传不崩
      pump(jpg)
    else:
      pump(None)                       # 都有半帧没发完 → 只 flush，不抓新帧

main()
