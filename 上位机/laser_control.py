"""
脉冲激光控制模块 - 串口上位机
设置脉冲数 / 延时时间，查询当前配置
"""

import tkinter as tk
from tkinter import ttk, scrolledtext
import serial
import serial.tools.list_ports
import threading
import time

BAUD = 115200


class App:
    def __init__(self, root):
        self.root = root
        root.title("脉冲激光控制模块")
        root.resizable(False, False)

        self.ser = None
        self.reading = False

        # ---- 串口连接 ----
        frm = ttk.LabelFrame(root, text="串口连接", padding=8)
        frm.pack(fill="x", padx=12, pady=(12, 0))

        self.port_var = tk.StringVar()
        self.cb = ttk.Combobox(frm, textvariable=self.port_var, width=12, state="readonly")
        self.cb.pack(side="left")

        ttk.Button(frm, text="刷新", width=5, command=self.refresh_ports).pack(side="left", padx=6)

        self.btn_conn = ttk.Button(frm, text="连接", width=6, command=self.toggle_conn)
        self.btn_conn.pack(side="left", padx=6)

        self.lb_status = ttk.Label(frm, text="未连接", foreground="gray")
        self.lb_status.pack(side="left", padx=12)

        self.refresh_ports()

        # ---- 参数设置 ----
        frm2 = ttk.LabelFrame(root, text="参数设置", padding=8)
        frm2.pack(fill="x", padx=12, pady=(12, 0))

        ttk.Label(frm2, text="每组脉冲数:").grid(row=0, column=0, sticky="e", padx=(0, 4))
        self.entry_n = ttk.Entry(frm2, width=10)
        self.entry_n.grid(row=0, column=1, padx=(0, 16))
        self.entry_n.insert(0, "2")

        ttk.Label(frm2, text="间隔时间(ms):").grid(row=0, column=2, sticky="e", padx=(0, 4))
        self.entry_t = ttk.Entry(frm2, width=10)
        self.entry_t.grid(row=0, column=3, padx=(0, 12))
        self.entry_t.insert(0, "2")

        ttk.Button(frm2, text="写入设备", command=self.send_params).grid(row=0, column=4, padx=4)
        ttk.Button(frm2, text="查询配置", command=self.send_status).grid(row=0, column=5, padx=4)

        # ---- 通讯记录 ----
        frm4 = ttk.LabelFrame(root, text="通讯记录", padding=8)
        frm4.pack(fill="both", expand=True, padx=12, pady=12)

        self.log = scrolledtext.ScrolledText(frm4, width=52, height=14, state="disabled")
        self.log.pack(fill="both", expand=True)

        root.protocol("WM_DELETE_WINDOW", self.on_close)

    # ========== 串口 ==========

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.cb["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def toggle_conn(self):
        if self.ser and self.ser.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_var.get()
        if not port:
            return
        try:
            self.ser = serial.Serial(port, BAUD, timeout=0.05)
            self.reading = True
            threading.Thread(target=self.reader, daemon=True).start()
            self.btn_conn.config(text="断开")
            self.lb_status.config(text=f"已连接 {port}", foreground="green")
            self.log_line(f"已连接 {port}")
            time.sleep(0.3)
            self.send_status()
        except Exception as e:
            self.log_line(f"连接失败: {e}")

    def disconnect(self):
        self.reading = False
        if self.ser:
            try:
                self.ser.close()
            except:
                pass
            self.ser = None
        self.btn_conn.config(text="连接")
        self.lb_status.config(text="未连接", foreground="gray")
        self.log_line("已断开")

    def reader(self):
        buf = b""
        while self.reading and self.ser and self.ser.is_open:
            try:
                n = self.ser.in_waiting
                if n:
                    data = self.ser.read(n)
                    buf += data
                    if b"\n" in buf:
                        lines = buf.split(b"\n")
                        for line in lines[:-1]:
                            text = line.strip().decode("utf-8", errors="replace")
                            if text:
                                self.root.after(0, lambda t=text: self._on_recv(t))
                        buf = lines[-1]
                else:
                    time.sleep(0.05)
            except:
                break

    def _on_recv(self, text):
        self.log_line(text, tag="recv")
        # parse "N=10 T=2ms" or "OK N=10 T=2ms" and update fields
        import re
        m = re.search(r"N=(\d+)\s+T=(\d+)", text)
        if m:
            self.entry_n.delete(0, "end")
            self.entry_n.insert(0, m.group(1))
            self.entry_t.delete(0, "end")
            self.entry_t.insert(0, m.group(2))

    # ========== 发送 ==========

    def send_params(self):
        n = self.entry_n.get().strip()
        t = self.entry_t.get().strip()
        if n and t:
            self.send(f"N={n} T={t}")

    def send_status(self):
        self.send("STATUS?")

    def send(self, text):
        if not self.ser or not self.ser.is_open:
            self.log_line("请先连接设备", tag="err")
            return
        try:
            self.ser.write((text + "\r\n").encode())
            self.log_line(f"发送: {text}", tag="send")
        except Exception as e:
            self.log_line(f"发送失败: {e}", tag="err")

    # ========== 日志 ==========

    def log_line(self, text, tag=None):
        self.log.config(state="normal")
        ts = time.strftime("%H:%M:%S")
        color = {"send": "#333", "recv": "#060", "err": "#c00"}.get(tag, "#000")
        self.log.tag_config(tag, foreground=color) if tag else None
        self.log.insert("end", f"[{ts}]  {text}\n", tag)
        self.log.see("end")
        self.log.config(state="disabled")

    def on_close(self):
        self.disconnect()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    App(root)
    root.mainloop()
