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

        self.create_layout()
    
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