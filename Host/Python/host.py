import serial
import serial.tools.list_ports
import time
import struct
import matplotlib.pyplot as plt
import math
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

class sfra_host:
    IDLE = 0
    SFRA_INIT = 1
    SWEEPING = 2
    SWEEP_DONE = 3
    SFRA_DONE = 4

    def __init__(self, master):
        self.master = master
        self.master.title("SFRA Host")
        self.master.geometry("800x600")
        self.master.resizable(False, False)

        # Serial related settings
        self.ser = None
        self.selected_port = None
        self.baudrate = 115200
        self.timeout = 10
        self.connected = False
        
        # SFRA settings
        self.start_freq = tk.StringVar(value="10")
        self.step_freq = tk.StringVar(value="1.5")
        self.samp_freq = tk.StringVar(value="10000")
        self.inject_amplitude = tk.StringVar(value="2")
        self.sfra_status = self.IDLE

        self.create_layout()
        self.timer_loop()
    
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
        self.step_freq_label = ttk.Label(self.master, text="Step Frequency (Hz):")
        self.step_freq_label.place(x=10, y=95)
        self.step_freq_box = ttk.Entry(self.master, textvariable=self.step_freq, width=15)
        self.step_freq_box.place(x=10, y=115)
        self.samp_freq_label = ttk.Label(self.master, text="Sampling Frequency (Hz):")
        self.samp_freq_label.place(x=10, y=140)
        self.samp_freq_box = ttk.Entry(self.master, textvariable=self.samp_freq, width=15)
        self.samp_freq_box.place(x=10, y=160)
        self.inject_amp_label = ttk.Label(self.master, text="Inject Amplitude")
        self.inject_amp_label.place(x=10, y=185)
        self.inject_amp_box = ttk.Entry(self.master, textvariable=self.inject_amplitude, width=15)
        self.inject_amp_box.place(x=10, y=205)
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
    
    def start_sfra(self):
        if self.connected == False:
            return
        frame = bytes([0xAA, 0x02, 0x01, 0xA8])
        self.ser.write(frame)
        ret = self.ser.read(5)
        print(ret)
    
    def stop_sfra(self):
        if self.connected == False:
            return
        frame = bytes([0xAA, 0x01, 0x01, 0xAA])
        self.ser.write(frame)
        ret = self.ser.read(5)
        print(ret)
    
    def timer_loop(self):
        if self.connected == False:
            self.master.after(500, self.timer_loop)
            return
        
        frame = bytes([0xAA, 0x03, 0x01, 0xA8])
        self.ser.write(frame)
        ret = self.ser.read(5)
        print(ret)
        match ret[3]:
            case self.IDLE:
                self.status_label['text'] = "IDLE"
            case self.SFRA_INIT:
                self.status_label['text'] = "SFRA INIT"
            case self.SWEEPING:
                self.status_label['text'] = "SWEEPING"
            case self.SWEEP_DONE:
                self.status_label['text'] = "SWEEP DONE"
            case self.SFRA_DONE:
                self.status_label['text'] = "SFRA DONE"
            case _:
                self.status_label['text'] = "COMM ERROR"
        self.master.after(500, self.timer_loop)
        

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