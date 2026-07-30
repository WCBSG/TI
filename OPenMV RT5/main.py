"""OMV-RT5 WiFi MJPEG 图传 — AP: OMVRT5 / 12345678 → http://192.168.4.1:8000/"""
import sensor, time, network, socket

# 配置
WIFI_SSID = "OMVRT5"
WIFI_PASSWORD = "12345678"
HTTP_PORT = 8000
STREAM_WIDTH = 320
STREAM_HEIGHT = 240
JPEG_QUALITY = 30
BOUNDARY = "frame"
stream_conn = None

# ---- 工具 ----
def _close(s):
    try: s.close()
    except Exception: pass

def http_send(conn, status, body, ct="text/html; charset=utf-8"):
    if isinstance(body, str): body = body.encode("utf-8")
    conn.sendall(("HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
                  "Connection: close\r\n\r\n" % (status, ct, len(body))).encode())
    conn.sendall(body)

def get_path(conn):
    """读取 HTTP 请求行，返回路径或空字符串"""
    try:
        conn.settimeout(2)
        d = conn.recv(1024)
        if not d: return ""
        line = d.split(b"\r\n", 1)[0]
        if not line.startswith(b"GET "): return ""
        return line.split(b" ")[1].decode("utf-8", "ignore")
    except Exception:
        return ""

def send_frame(conn, jpg):
    conn.sendall(("--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n"
                  % (BOUNDARY, len(jpg))).encode())
    conn.sendall(jpg)
    conn.sendall(b"\r\n")

# ---- HTML ----
PAGE = (
    '<!DOCTYPE html><html lang="zh"><head>'
    '<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'
    '<title>OMV-RT5</title><style>'
    '*{margin:0;padding:0;box-sizing:border-box}body{background:#000;overflow:hidden}'
    '#live{position:fixed;top:12px;left:16px;color:#58a6ff;font:600 12px -apple-system,sans-serif;'
    'background:rgba(0,0,0,.5);padding:4px 10px;border-radius:4px;z-index:10}'
    '.stream{width:100vw;height:100vh;object-fit:contain}'
    '</style></head><body>'
    '<div id="live">● LIVE %dx%d</div>' % (STREAM_WIDTH, STREAM_HEIGHT)
    + '<img class="stream" src="/stream" alt=""></body></html>'
)

# ---- WiFi ----
def connect_wifi_ap(ssid, key):
    print("[WIFI] AP:%s ..." % ssid)
    wlan = network.WLAN(network.AP_IF)
    wlan.config(ssid=ssid, key=key, channel=2)
    wlan.active(True)
    for _ in range(50):
        ip = wlan.ifconfig()[0]
        if ip and ip != "0.0.0.0":
            print("[WIFI] %s" % ip)
            return ip
        time.sleep_ms(100)

# ---- 主循环 ----
def main():
    global stream_conn
    try:
        ip = connect_wifi_ap(WIFI_SSID, WIFI_PASSWORD)
        if not ip: print("[ERROR] WiFi fail"); return

        sensor.reset()
        sensor.set_pixformat(sensor.RGB565)
        sensor.set_framesize(sensor.QVGA)
        sensor.skip_frames(time=2000)
        print("[CAM] %dx%d" % (sensor.width(), sensor.height()))

        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(["", HTTP_PORT])
        server.listen(2)
        server.setblocking(False)
        print("[HTTP] :%d  [RUN] http://%s:%d/" % (HTTP_PORT, ip, HTTP_PORT))

        while True:
            try:
                # 采集 + 推流
                jpg = sensor.snapshot().compress(quality=JPEG_QUALITY)
                if stream_conn:
                    try:
                        send_frame(stream_conn, jpg)
                    except Exception:
                        _close(stream_conn)
                        stream_conn = None

                # 接受新连接
                conn = None
                try:
                    conn, _ = server.accept()
                    conn.setblocking(True)
                    conn.settimeout(2)
                except Exception:
                    pass

                if not conn: continue

                try:
                    path = get_path(conn)
                    if not path:
                        _close(conn); conn = None; continue

                    if path == "/" or path == "":
                        http_send(conn, "200 OK", PAGE)

                    elif path == "/stream":
                        if stream_conn: _close(stream_conn)
                        conn.sendall((
                            "HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, "
                            "must-revalidate, max-age=0\r\nPragma: no-cache\r\n"
                            "Connection: close\r\n"
                            "Content-Type: multipart/x-mixed-replace; boundary=%s\r\n\r\n"
                            % BOUNDARY).encode())
                        stream_conn = conn
                        try: stream_conn.settimeout(1)
                        except Exception: pass
                        conn = None
                        print("[STREAM] on")

                    else:
                        http_send(conn, "404 Not Found", "404", "text/plain")

                except Exception as e:
                    print("[ERR] %s" % e)
                finally:
                    if conn: _close(conn)

            except Exception as e:
                print("[LOOP] %s" % e)
                time.sleep_ms(100)

    except KeyboardInterrupt:
        print("[STOP]")
    except Exception as e:
        print("[FATAL] %s" % e)
    finally:
        if stream_conn: _close(stream_conn)
        _close(server)
        print("[END]")

if __name__ == "__main__":
    main()
