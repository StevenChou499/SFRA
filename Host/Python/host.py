import serial
import serial.tools.list_ports
import time
import struct
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import math
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
import threading

class sfra_host:
    IDLE = 0
    SFRA_INIT = 1
    SWEEP_INIT = 2
    SWEEPING = 3
    SWEEP_DONE = 4
    SFRA_DONE = 5

    def __init__(self, master):
        self.master = master
        self.master.title("SFRA Host")
        self.master.geometry("1000x600")
        self.master.resizable(False, False)
        # self.master.protocol("WM_DELETE_WINDOW", self.on_closing)

        # Serial related settings 
        self.ser = None
        self.selected_port = None
        self.baudrate = 115200
        self.timeout = 10
        self.connected = False
        self.mutex = threading.Lock()
        self.worker_thread = threading.Thread(target=self.bg_task, daemon=True)
        
        # SFRA settings
        self.start_freq = tk.StringVar(value="10")
        self.step_freq = tk.StringVar(value="1.5")
        self.samp_freq = tk.StringVar(value="100000")
        self.inject_amplitude = tk.StringVar(value="2")
        self.sfra_status = self.IDLE

        self.create_layout()
        self.worker_thread.start()

    def on_closing(self):
        if self.connected == True:
            self.ser.close()
            self.master.destroy()
            print("Exiting...")
            exit()
    
    def create_layout(self):
        # Serial
        self.com_list = ttk.Combobox(self.master, state="readonly")
        self.com_list.place(x=10, y=10, width=150)
        self.com_list.bind("<Button-1>", self.list_comports)
        self.connect_btn = ttk.Button(self.master, text="Connect", command=self.con_dis_click)
        self.connect_btn.place(x=170, y=8, width=100)

        # SFRA setting
        self.start_freq_label = ttk.Label(self.master, text="Starting Frequency (Hz):")
        self.start_freq_label.place(x=10, y=50)
        self.start_freq_box = ttk.Entry(self.master, textvariable=self.start_freq, width=15)
        self.start_freq_box.place(x=10, y=70)
        self.set_start_freq_btn = ttk.Button(self.master, text="Set", command=self.set_start_freq, width=10)
        self.set_start_freq_btn.place(x=130, y=68)
        self.step_freq_label = ttk.Label(self.master, text="Step Frequency (Hz):")
        self.step_freq_label.place(x=10, y=95)
        self.step_freq_box = ttk.Entry(self.master, textvariable=self.step_freq, width=15)
        self.step_freq_box.place(x=10, y=115)
        self.set_step_freq_btn = ttk.Button(self.master, text="Set", command=self.set_step_freq, width=10)
        self.set_step_freq_btn.place(x=130, y=113)
        self.samp_freq_label = ttk.Label(self.master, text="Sampling Frequency (Hz):")
        self.samp_freq_label.place(x=10, y=140)
        self.samp_freq_box = ttk.Entry(self.master, textvariable=self.samp_freq, width=15)
        self.samp_freq_box.place(x=10, y=160)
        self.set_samp_freq_btn = ttk.Button(self.master, text="Set", command=self.set_samp_freq, width=10)
        self.set_samp_freq_btn.place(x=130, y=158)
        self.inject_amp_label = ttk.Label(self.master, text="Inject Amplitude")
        self.inject_amp_label.place(x=10, y=185)
        self.inject_amp_box = ttk.Entry(self.master, textvariable=self.inject_amplitude, width=15)
        self.inject_amp_box.place(x=10, y=205)
        self.set_inj_amp_btn = ttk.Button(self.master, text="Set", command=self.set_inject_amp, width=10)
        self.set_inj_amp_btn.place(x=130, y=203)
        self.status_label = ttk.Label(self.master, text="IDLE", font=("Arial", 16, "bold italic"), width=30)
        self.status_label.place(x=10, y=250)
        self.start_btn = ttk.Button(
            self.master, 
            text="Start", 
            width=20, 
            command=self.start_sfra)
        self.start_btn.place(x=10, y=300)
        self.stop_btn = ttk.Button(
            self.master, 
            text="Reset/Stop", 
            width=20, 
            command=self.stop_sfra)
        self.stop_btn.place(x=10, y=325)
        self.get_bode_btn = ttk.Button(
            self.master, 
            text="Get Bode", 
            width=20, 
            command=self.get_bode_plot
        )
        self.get_bode_btn.place(x=10, y=350)
        # --- Create a Matplotlib Figure ---
        self.fig = Figure(dpi=100, constrained_layout=True)
        ax = self.fig.add_subplot(211)
        ax.set_title("Magnitude")
        ax.set_xlabel("Frequency (Hz)")
        ax.set_ylabel("Mag (db)")
        ax.set_xscale("log")
        ax.grid()
        ay = self.fig.add_subplot(212)
        ay.set_title("Phase")
        ay.set_xlabel("Frequency (Hz)")
        ay.set_ylabel("Phase (deg)")
        ay.set_xscale("log")
        ay.grid()
        
        # --- Embed figure into Tkinter window ---
        canvas = FigureCanvasTkAgg(self.fig, master=self.master)
        canvas.draw()  # draw the initial plot
        canvas.get_tk_widget().place(x=220, y=50, width=770, height=540)
    
    def list_comports(self, e):
        """Shows the available ports when combobox is clicked"""
        ports = serial.tools.list_ports.comports()
        ports_list = [p.name for p in ports]
        self.com_list['values'] = ports_list
    
    def con_dis_click(self):
        """Connect or disconnect corresponding port"""
        self.selected_port = self.com_list.get()
        if self.connected == False: # Not connected
            if self.selected_port != None:
                try:
                    self.ser = serial.Serial(
                        port = self.selected_port, 
                        baudrate = self.baudrate, 
                        timeout = self.timeout
                    )
                    self.connected = True
                    self.com_list.config(state="disabled")
                    self.connect_btn['text'] = "Disconnect"
                    print("Com port connect success")
                except serial.SerialException as e:
                    print(f"Error opening serial port: {e}")
                    messagebox.showinfo("Error", "Connection failed!")
        else: # Already connected
            print("Disconnecting")
            self.connected = False
            self.com_list.config(state="enabled")
            self.connect_btn['text'] = "Connect"
            self.ser.close()
    
    def set_start_freq(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00])
        start_freq_value = float(self.start_freq.get())
        start_freq_byte = struct.pack("<f", start_freq_value)
        frame[-5:-1] = start_freq_byte
        for i in (range(len(frame) - 1)):
            frame[-1] = frame[-1] ^ frame[i]
        print(frame)
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)
    
    def set_step_freq(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00])
        step_freq_value = float(self.step_freq.get())
        step_freq_byte = struct.pack("<f", step_freq_value)
        frame[-5:-1] = step_freq_byte
        for i in (range(len(frame) - 1)):
            frame[-1] = frame[-1] ^ frame[i]
        print(frame)
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)

    def set_samp_freq(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x07, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00])
        samp_freq_value = float(self.samp_freq.get())
        samp_freq_byte = struct.pack("<f", samp_freq_value)
        frame[-5:-1] = samp_freq_byte
        for i in (range(len(frame) - 1)):
            frame[-1] = frame[-1] ^ frame[i]
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)
    
    def set_inject_amp(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00])
        inject_amp_value = float(self.inject_amplitude.get())
        inject_amp_byte = struct.pack("<f", inject_amp_value)
        frame[-5:-1] = inject_amp_byte
        for i in (range(len(frame) - 1)):
            frame[-1] = frame[-1] ^ frame[i]
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)
    
    def start_sfra(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x02, 0x01, 0xA8])
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)
    
    def stop_sfra(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x01, 0x01, 0xAA])
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)
    
    def get_bode_plot(self):
        if self.connected == False:
            return
        frame = bytearray([0xAA, 0x04, 0x01, 0xAF])
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(4)
            payload_len = struct.unpack('<H', ret[2:4])[0]
            # payload_len = ret[2]
            ret = self.ser.read(payload_len)
        print(ret)
        payload = ret[:-1]
        count = len(payload) // 4
        # print(payload[1008:1012])
        values = struct.unpack('<' + 'f'*count, payload)
        group_size = int(count / 3)
        print(f"Count = {count}, group_size = {group_size}")
        groups = [values[i:i+group_size] for i in range(0, len(values), group_size)]
        freq_list, mag_list, pha_list = [list(g) for g in groups]
        # print(pha_list)
        
        # update bode plot
        self.fig = Figure(dpi=100, constrained_layout=True)
        ax = self.fig.add_subplot(211)
        ax.plot(freq_list, mag_list)
        ax.set_title("Magnitude")
        ax.set_xlabel("Frequency (Hz)")
        ax.set_ylabel("Mag (db)")
        ax.set_xscale("log")
        ax.grid()
        ay = self.fig.add_subplot(212)
        ay.plot(freq_list, pha_list)
        ay.set_title("Phase")
        ay.set_xlabel("Frequency (Hz)")
        ay.set_ylabel("Phase (deg)")
        ay.set_xscale("log")
        ay.grid()
        
        # --- Embed figure into Tkinter window ---
        canvas = FigureCanvasTkAgg(self.fig, master=self.master)
        canvas.draw()  # draw the initial plot
        canvas.get_tk_widget().place(x=220, y=50, width=770, height=540)
    
    def bg_task(self):
        if self.connected == False:
            self.master.after(2000, self.bg_task)
            return
        frame = bytearray([0xAA, 0x03, 0x01, 0xA8])
        print(frame)
        with self.mutex:
            self.ser.write(frame)
            ret = self.ser.read(6)
        print(ret)
        match ret[4]:
            case self.IDLE:
                self.status_label['text'] = "IDLE"
            case self.SFRA_INIT:
                self.status_label['text'] = "SFRA INIT"
            case self.SWEEP_INIT:
                self.status_label['text'] = "SWEEP INIT"
            case self.SWEEPING:
                self.status_label['text'] = "SWEEPING"
            case self.SWEEP_DONE:
                self.status_label['text'] = "SWEEP DONE"
            case self.SFRA_DONE:
                self.status_label['text'] = "SFRA DONE"
            case _:
                self.status_label['text'] = "COMM ERROR"
        self.master.after(2000, self.bg_task)
        

if __name__ == "__main__":
    root = tk.Tk()
    app = sfra_host(root)
    root.mainloop()

# window format
# +-----------------> x axis
# |
# |
# |
# |
# |
# v
# y axis