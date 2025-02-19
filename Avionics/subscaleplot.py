import customtkinter as ctk
import pandas as pd
import serial
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter
from tkinter import filedialog, messagebox
import threading
import random
import time
import queue
import tkintermapview

config = {                  #config for what is needed to run the app
    "com_port": None,
    "baud_rate": 9600,
    "data_file": None
}
ser = None
df = pd.DataFrame(columns=['time', 'pressure', 'altitude', 'velocity', 'acc_x', 'acc_y', 'acc_z', 'acc_tot','longitude', 'latitude']) #csv needs to contain this headers
data_queue = queue.Queue()

#for live data parsing
def read_serial_data():
    global df
    try:
        ser = serial.Serial(config["com_port"], config["baud_rate"], timeout=1)
        while True:
            line = ser.readline().decode("utf-8").strip()
            if line:
                values = line.split(";")
                if len(values) == 10:
                    time_data = time.strftime("%H:%M:%S")
                    parsed_data = {
                        "time": time_data,
                        "pressure": float(values[0]),
                        "altitude": float(values[1]),
                        "velocity": float(values[2]),
                        "acc_x": float(values[3]),
                        "acc_y": float(values[4]),
                        "acc_z": float(values[5]),
                        "acc_tot": float(values[6]),
                        "latitude": float(values[7]),
                        "longitude": float(values[8]),
                        "status": values[9]
                    }
                    df = pd.concat([df, pd.DataFrame([parsed_data])], ignore_index=True)
                    df.to_csv(config["data_file"], index=False)
                    data_queue.put(parsed_data)
    except Exception as e:
        print(f"Serial Read Error: {e}")

#for live data plot updates
def update_plot_loop(fig, axes, canvas, status_label):
    while True:
        if not data_queue.empty():
            new_data = data_queue.get()
            for i, col in enumerate(['pressure', 'altitude', 'velocity']):
                axes[i].clear()
                axes[i].plot(df['time'], df[col], label=col, color='cyan')
                axes[i].set_ylabel(col, color='white')
                axes[i].legend()
                axes[i].set_facecolor("#2b2b2b")
                axes[i].spines['bottom'].set_color('white')
                axes[i].spines['top'].set_color('white')
                axes[i].spines['right'].set_color('white')
                axes[i].spines['left'].set_color("white")
                axes[i].tick_params(axis='x', colors='white')
                axes[i].tick_params(axis='y', colors='white')
            
            axes[-1].clear()
            for col, color in zip(['acc_x', 'acc_y', 'acc_z', 'acc_tot'], ['r', 'g', 'b', 'cyan']):
                axes[-1].plot(df['time'], df[col], label=col, color=color)
            axes[-1].set_ylabel("Acceleration", color="white")
            axes[-1].legend()
            axes[-1].set_facecolor("#2b2b2b")
            axes[-1].spines['bottom'].set_color('white')
            axes[-1].spines['top'].set_color('white')
            axes[-1].spines['right'].set_color('white')
            axes[-1].spines['left'].set_color("white")
            axes[-1].tick_params(axis='x', colors='white')
            axes[-1].tick_params(axis='y', colors='white')

            fig.patch.set_facecolor("#2b2b2b")
            canvas.draw()

            status_label.configure(text=f"Latest Data: {new_data}  |  Status: {new_data['status']}")
        time.sleep(0.1)

#for only csv plot updates
def plot_csv_data_loop(file_path, canvas, fig, axes):
    df = pd.read_csv(file_path)
    expected_columns= ["time", 'pressure', 'altitude', 'velocity', 'acc_x', 'acc_y', 'acc_z', "acc_tot"]
    available_columns = [col for col in expected_columns if col in df.columns]

    if "time" not in df.columns:
        print("Error: no time column")
        return
    
    index = 0
    total_rows = len(df)
    
    def update_plot():
        nonlocal index

        for i, col in enumerate(['pressure', 'altitude','velocity']):
            if col in df.columns:
                ax = axes[i]
                ax.clear()
                ax.plot(df["time"].iloc[:index], df[col].iloc[:index], label=col, color='cyan')
                ax.set_ylabel(col, color="white")
                ax.legend()
                ax.set_facecolor("#2b2b2b")
                ax.spines['bottom'].set_color('white')
                ax.spines['top'].set_color('white')
                ax.spines['right'].set_color('white')
                ax.spines['left'].set_color("white")
                ax.tick_params(axis='x', colors='white')
                ax.tick_params(axis='y', colors='white')

        ax = axes[-1]
        ax.clear()
        for col, color in zip(["acc_x", "acc_y", "acc_z", "acc_tot"], ["r", "g", "b", "cyan"]):
            if col in df.columns:
                ax.plot(df["time"].iloc[:index], df[col].iloc[:index], label=col, color=color)

        ax.set_ylabel("Acceleration", color="white")
        ax.legend()
        ax.set_facecolor("#2b2b2b")
        ax.spines['bottom'].set_color('white')
        ax.spines['top'].set_color('white')
        ax.spines['right'].set_color('white')
        ax.spines['left'].set_color("white")
        ax.tick_params(axis='x', colors='white')
        ax.tick_params(axis='y', colors='white')

        axes[-1].set_xlabel("time", color='white')
        fig.patch.set_facecolor("#2b2b2b")
        canvas.draw()
        index = (index+1) % total_rows
        
        canvas.get_tk_widget().after(10, update_plot)

    update_plot()

#test data points generator 
def test_data():
    time_data = time.strftime("%H:%M:%S")
    pressure_data = random.randint(0, 1000)
    altitude_data = random.randint(0, 10000)
    velocity_data = random.randint(0, 10000)
    acc_x_data = random.randint(0, 1000)
    acc_y_data = random.randint(0, 1000)
    acc_z_data = random.randint(0, 1000)
    acc_tot_data = acc_x_data + acc_y_data + acc_z_data
    return {
        "time": time_data,
        "pressure": pressure_data,
        "altitude": altitude_data,
        "velocity": velocity_data,
        "acc_x": acc_x_data,
        "acc_y": acc_y_data,
        "acc_z": acc_z_data,
        "acc_tot": acc_tot_data,
    }

#test data graph generator 
def update_graph(axs, new_data):
    axs[0, 0].lines[0].set_ydata(new_data["pressure"])
    axs[0, 0].lines[0].set_xdata(range(len(new_data["pressure"])))
    axs[0, 0].lines[0].set_color("red")
    axs[0, 0].relim()
    axs[0, 0].autoscale_view()

    axs[0, 1].lines[0].set_ydata(new_data["altitude"])
    axs[0, 1].lines[0].set_xdata(range(len(new_data["altitude"])))
    axs[0, 1].lines[0].set_color("blue")
    axs[0, 1].relim()
    axs[0, 1].autoscale_view()

    axs[1, 0].lines[0].set_ydata(new_data["velocity"])
    axs[1, 0].lines[0].set_xdata(range(len(new_data["velocity"])))
    axs[1, 0].lines[0].set_color("green")
    axs[1, 0].relim()
    axs[1, 0].autoscale_view()

    axs[1,1].cla()
    labels = ["acc_x", "acc_y", "acc_z", "acc_tot"]
    colors = ["r", "g", "b", "cyan"]
    for i, label in enumerate(labels):
        axs[1,1].plot(new_data[label], color = colors[i],label = label)

    axs[1,1].legend()
    axs[1,1].set_title("Acceleration")
    

# app code and setup
class ThrustMITApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("ThrustMIT App")
        self.geometry("1920x1080")
        self.setup_screen()

    def setup_screen(self):
        self.clear_frame()

        ctk.CTkLabel(self, text="Enter Configuration", font=("Montserrat", 20)).pack(pady=20)

        self.com_port_entry = ctk.CTkEntry(self, placeholder_text="COM PORT")
        self.com_port_entry.pack(pady=10)

        self.baud_rate_entry = ctk.CTkEntry(self, placeholder_text="Baud Rate")
        self.baud_rate_entry.pack(pady=10)

        self.data_file_btn = ctk.CTkButton(self, text="Select Data File", command=self.select_data_file)
        self.data_file_btn.pack(pady=10)

        self.start_btn = ctk.CTkButton(self, text="Start", command=self.start_app)
        self.start_btn.pack(pady=10)

        self.gps_btn = ctk.CTkButton(self, text="Start GPS", command = self.start_gps)
        self.gps_btn.pack(pady=10)

        self.only_csv_btn = ctk.CTkButton(self, text="Only CSV", command= self.only_csv)
        self.only_csv_btn.pack(pady=10)

        self.test_btn = ctk.CTkButton(self, text="Test the App", command=self.test_app)
        self.test_btn.pack(pady=10)

    def clear_frame(self):
        for widget in self.winfo_children():
            widget.destroy()

    def select_state_file(self):
        file_path = filedialog.asksaveasfilename(defaultextension=".csv", filetypes=[("CSV files", "*.csv")])
        if file_path:
            config["state_file"] = file_path

    def select_data_file(self):
        file_path = filedialog.asksaveasfilename(defaultextension=".csv", filetypes=[("CSV files", "*.csv")])
        if file_path:
            config["data_file"] = file_path

    def start_app(self):
        config["com_port"] = self.com_port_entry.get()
        config["baud_rate"] = int(self.baud_rate_entry.get()) if self.baud_rate_entry.get() else 9600

        if not (config["com_port"] and config["state_file"] and config["data_file"]):
            messagebox.showerror("Error", "All configuration information must be provided!")
            return

    #test app graph integration code
    def test_app(self):
        test_window = ctk.CTkToplevel(self)
        test_window.title("Test Data Visualization")
        test_window.geometry("1980x1080")
        
        fig, axs = plt.subplots(2, 2, figsize=(12, 10))
        canvas = FigureCanvasTkAgg(fig, master=test_window)
        canvas.get_tk_widget().pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=True)
        background_color = '#2b2b2b'
        fig.patch.set_facecolor(background_color)

        axs[0, 0].plot([], [], label="Pressure")
        axs[0, 0].xaxis.label.set_color('w')
        axs[0, 0].yaxis.label.set_color('w')

        axs[0, 1].plot([], [], label="Altitude")
        axs[0, 1].xaxis.label.set_color('w')
        axs[0, 1].yaxis.label.set_color('w')
        
        axs[1, 0].plot([], [], label="Velocity")
        axs[1, 0].xaxis.label.set_color('w')
        axs[1, 0].yaxis.label.set_color('w')
        axs[1, 1].plot([], [], label="Acceleration")
        axs[1, 1].xaxis.label.set_color('w')
        axs[1, 1].yaxis.label.set_color('w')

        for ax_row in axs:
            for ax in ax_row:
                ax.legend()
                ax.set_xlim(0, 30)
                ax.set_facecolor('#2b2b2b')

        def background_update():
            while True:
                data = test_data()
                data_queue.put(data)
                time.sleep(0.1)

        def update_graph_loop():
            while True:
                if not data_queue.empty():
                    data = data_queue.get()
                    global df
                    df = pd.concat([df, pd.DataFrame([data])], ignore_index=True)
                    if len(df) > 30:
                        df = df.tail(30).reset_index(drop=True)
                    update_graph(axs, df)
                    canvas.draw()

        threading.Thread(target=background_update, daemon=True).start()
        threading.Thread(target=update_graph_loop, daemon=True).start()


    #gps integration 
    def start_gps(self):
        if not config["data_file"]:
            messagebox.showerror("error, no data file")
            return
        
        gps_window  = ctk.CTKToplevel(self)
        gps_window. title("gps window")
        gps_window.geometry("800x600")

        self.map_widget = tkintermapview.TkinterMapView(gps_window, width=800, height=600)
        self.map_widget.pack(fill="both", expand=True)

        threading.Thread(target=self.update_gps_location, daemon=True).start()

    def update_gps_location(self):
        while True: 
            try:
                df = pd.read_csv(config["data_file"])
                
                if "latitude" in df.columns and "longitude" in df.columns:
                    latest_row = df.iloc[-1]
                    lat, lon = latest_row['latitude'], latest_row["longitude"]

                    if not(-90<= lat <= 90 and -180<= lon <= 180):
                        raise ValueError("Invalid GPS coordinates")
                    
                    if self.map_marker:
                        self.map_marker.set_position(lat, lon)
                        self.map_marker.text = f"Lat: {lat:.5f}, Lon: {lon:.5f}"
                    else:
                        self.map_marker = self.map_widget.set_marker(lat, lon, text= f"Lat: {lat:.5f}, Lon: {lon:.5f}")
                    
                    self.map_widget.set_position(lat, lon)

            except Exception as e:
                print(f"GPS Update error: {e}")

            time.sleep(5)
    #only csv integration
    def only_csv(self):

        file_path = filedialog.askopenfilename(filetypes=[("CSV files", "*.csv")])
        if not file_path:
            return
        csv_window = ctk.CTkToplevel()
        csv_window.title("CSV Data Visualization")
        csv_window.geometry("1920x1080")
        csv_window.configure(bg="#2b2b2b")

        fig, axes = plt.subplots(4, 1, figsize=(10, 12), sharex=True)
        canvas = FigureCanvasTkAgg(fig, master=csv_window)
        canvas.get_tk_widget().pack(fill=tkinter.BOTH, expand=True)

        plot_csv_data_loop(file_path, canvas, fig, axes)
    #live data integration
    def start_live_data(self):
            config["com_port"] = self.com_port_entry.get()
            config["baud_rate"] = int(self.baud_rate_entry.get()) if self.baud_rate_entry.get() else 9600

            if not (config["com_port"] and config["data_file"]):
                messagebox.showerror("Error", "All configuration information must be provided!")
                return

            live_window = ctk.CTkToplevel(self)
            live_window.title("Live Data Visualization")
            live_window.geometry("1920x1080")
            live_window.configure(bg="#2b2b2b")

            fig, axes = plt.subplots(4, 1, figsize=(10, 12), sharex=True)
            canvas = FigureCanvasTkAgg(fig, master=live_window)
            canvas.get_tk_widget().pack(fill=tkinter.BOTH, expand=True)

            self.status_label = ctk.CTkLabel(live_window, text="Latest Data: - | Status: -", font=("Montserrat", 16), fg_color="#2b2b2b")
            self.status_label.pack(pady=20)

            threading.Thread(target=read_serial_data, daemon=True).start()
            threading.Thread(target=update_plot_loop, args=(fig, axes, canvas, self.status_label), daemon=True).start()

#launches the app
if __name__ == "__main__":
    app = ThrustMITApp()
    app.mainloop()