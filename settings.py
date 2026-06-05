import os
import sys
import tkinter as tk
from tkinter import ttk, colorchooser, messagebox

CONFIG_FILE = "overlay.ini"

# Default configuration
DEFAULT_CONFIG = {
    "width": 200,
    "height": 110,
    "alpha": 0.88,
    "margin": 14,
    "corner": "top_right",
    "bg_r": 10, "bg_g": 10, "bg_b": 10,
    "fps_ok_r": 0, "fps_ok_g": 220, "fps_ok_b": 120,
    "fps_warn_r": 255, "fps_warn_g": 170, "fps_warn_b": 0,
    "fps_crit_r": 255, "fps_crit_g": 50, "fps_crit_b": 70,
    "text_dim_r": 60, "text_dim_g": 100, "text_dim_b": 75,
    "sep_r": 40, "sep_g": 70, "sep_b": 50,
    "show_fps": True,
    "show_cpu": True,
    "show_gpu": True,
    "show_title": True,
    "font_fps": 18,
    "font_val": 14,
    "font_small": 10,
    "update_metrics": 800,
    "update_overlay": 33
}

class OverlaySettings:
    def __init__(self):
        self.config = self.load_config()
        self.root = tk.Tk()
        self.root.title("Overlay Settings")
        self.root.geometry("500x650")
        self.root.resizable(False, False)
        
        # Style
        self.root.configure(bg='#2b2b2b')
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('TLabel', background='#2b2b2b', foreground='white', font=('Segoe UI', 10))
        style.configure('TFrame', background='#2b2b2b')
        style.configure('TLabelframe', background='#2b2b2b', foreground='white', font=('Segoe UI', 10, 'bold'))
        style.configure('TLabelframe.Label', background='#2b2b2b', foreground='white', font=('Segoe UI', 10, 'bold'))
        style.configure('TButton', background='#3c3c3c', foreground='white', borderwidth=1, focusthickness=3)
        style.map('TButton', background=[('active', '#4c4c4c')])
        
        self.create_widgets()
        
    def load_config(self):
        config = DEFAULT_CONFIG.copy()
        
        if os.path.exists(CONFIG_FILE):
            with open(CONFIG_FILE, 'r') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith(';') or line.startswith('#'):
                        continue
                    
                    if '=' in line:
                        key, value = line.split('=', 1)
                        key = key.strip()
                        value = value.strip()
                        
                        if key in config:
                            # Parse based on type
                            if isinstance(config[key], bool):
                                config[key] = value.lower() in ('true', '1', 'yes')
                            elif isinstance(config[key], int):
                                # Handle RGB tuples
                                if ',' in value and key.endswith('_r'):
                                    # RGB values come as "r,g,b" for color keys
                                    continue
                                else:
                                    try:
                                        config[key] = int(value)
                                    except:
                                        pass
                            elif isinstance(config[key], float):
                                try:
                                    config[key] = float(value)
                                except:
                                    pass
                            else:
                                config[key] = value
        
        # Parse RGB colors (stored as separate r,g,b in ini)
        if os.path.exists(CONFIG_FILE):
            with open(CONFIG_FILE, 'r') as f:
                for line in f:
                    if 'background=' in line:
                        val = line.split('=')[1].strip()
                        if ',' in val:
                            rgb = val.split(',')
                            if len(rgb) == 3:
                                config['bg_r'] = int(rgb[0])
                                config['bg_g'] = int(rgb[1])
                                config['bg_b'] = int(rgb[2])
                    elif 'fps_ok=' in line:
                        val = line.split('=')[1].strip()
                        if ',' in val:
                            rgb = val.split(',')
                            if len(rgb) == 3:
                                config['fps_ok_r'] = int(rgb[0])
                                config['fps_ok_g'] = int(rgb[1])
                                config['fps_ok_b'] = int(rgb[2])
                    elif 'fps_warn=' in line:
                        val = line.split('=')[1].strip()
                        if ',' in val:
                            rgb = val.split(',')
                            if len(rgb) == 3:
                                config['fps_warn_r'] = int(rgb[0])
                                config['fps_warn_g'] = int(rgb[1])
                                config['fps_warn_b'] = int(rgb[2])
                    elif 'fps_crit=' in line:
                        val = line.split('=')[1].strip()
                        if ',' in val:
                            rgb = val.split(',')
                            if len(rgb) == 3:
                                config['fps_crit_r'] = int(rgb[0])
                                config['fps_crit_g'] = int(rgb[1])
                                config['fps_crit_b'] = int(rgb[2])
                    elif 'text_dim=' in line:
                        val = line.split('=')[1].strip()
                        if ',' in val:
                            rgb = val.split(',')
                            if len(rgb) == 3:
                                config['text_dim_r'] = int(rgb[0])
                                config['text_dim_g'] = int(rgb[1])
                                config['text_dim_b'] = int(rgb[2])
                    elif 'separator=' in line:
                        val = line.split('=')[1].strip()
                        if ',' in val:
                            rgb = val.split(',')
                            if len(rgb) == 3:
                                config['sep_r'] = int(rgb[0])
                                config['sep_g'] = int(rgb[1])
                                config['sep_b'] = int(rgb[2])
        
        return config
    
    def save_config(self):
        with open(CONFIG_FILE, 'w') as f:
            f.write("; Overlay Configuration File\n")
            f.write("; Edit this file or use settings.exe to change values\n\n")
            
            f.write("[Window]\n")
            f.write(f"width={self.config['width']}\n")
            f.write(f"height={self.config['height']}\n")
            f.write(f"alpha={self.config['alpha']}\n")
            f.write(f"margin={self.config['margin']}\n")
            f.write(f"corner={self.config['corner']}\n\n")
            
            f.write("[Colors]\n")
            f.write(f"background={self.config['bg_r']},{self.config['bg_g']},{self.config['bg_b']}\n")
            f.write(f"fps_ok={self.config['fps_ok_r']},{self.config['fps_ok_g']},{self.config['fps_ok_b']}\n")
            f.write(f"fps_warn={self.config['fps_warn_r']},{self.config['fps_warn_g']},{self.config['fps_warn_b']}\n")
            f.write(f"fps_crit={self.config['fps_crit_r']},{self.config['fps_crit_g']},{self.config['fps_crit_b']}\n")
            f.write(f"text_dim={self.config['text_dim_r']},{self.config['text_dim_g']},{self.config['text_dim_b']}\n")
            f.write(f"separator={self.config['sep_r']},{self.config['sep_g']},{self.config['sep_b']}\n\n")
            
            f.write("[Display]\n")
            f.write(f"show_fps={'true' if self.config['show_fps'] else 'false'}\n")
            f.write(f"show_cpu={'true' if self.config['show_cpu'] else 'false'}\n")
            f.write(f"show_gpu={'true' if self.config['show_gpu'] else 'false'}\n")
            f.write(f"show_title={'true' if self.config['show_title'] else 'false'}\n")
            f.write(f"font_fps={self.config['font_fps']}\n")
            f.write(f"font_val={self.config['font_val']}\n")
            f.write(f"font_small={self.config['font_small']}\n\n")
            
            f.write("[Update]\n")
            f.write(f"update_metrics={self.config['update_metrics']}\n")
            f.write(f"update_overlay={self.config['update_overlay']}\n")
        
        messagebox.showinfo("Success", "Settings saved! Restart overlay to apply changes.")
    
    def create_widgets(self):
        notebook = ttk.Notebook(self.root)
        notebook.pack(fill='both', expand=True, padx=10, pady=10)
        
        self.create_display_tab(notebook)
        self.create_colors_tab(notebook)
        self.create_advanced_tab(notebook)
        
        button_frame = ttk.Frame(self.root)
        button_frame.pack(fill='x', padx=10, pady=(0, 10))
        
        ttk.Button(button_frame, text="Save Settings", command=self.save_config).pack(side='right', padx=5)
        ttk.Button(button_frame, text="Reset to Default", command=self.reset_config).pack(side='right', padx=5)
        ttk.Button(button_frame, text="Preview", command=self.preview).pack(side='right', padx=5)
    
    def create_display_tab(self, notebook):
        tab = ttk.Frame(notebook)
        notebook.add(tab, text="Display")
        
        frame_show = ttk.LabelFrame(tab, text="Show/Hide Elements", padding=10)
        frame_show.pack(fill='x', padx=10, pady=10)
        
        self.show_fps = tk.BooleanVar(value=self.config['show_fps'])
        ttk.Checkbutton(frame_show, text="Show FPS", variable=self.show_fps,
                       command=lambda: self.config.update({'show_fps': self.show_fps.get()})).pack(anchor='w')
        
        self.show_cpu = tk.BooleanVar(value=self.config['show_cpu'])
        ttk.Checkbutton(frame_show, text="Show CPU Usage", variable=self.show_cpu,
                       command=lambda: self.config.update({'show_cpu': self.show_cpu.get()})).pack(anchor='w')
        
        self.show_gpu = tk.BooleanVar(value=self.config['show_gpu'])
        ttk.Checkbutton(frame_show, text="Show GPU Usage", variable=self.show_gpu,
                       command=lambda: self.config.update({'show_gpu': self.show_gpu.get()})).pack(anchor='w')
        
        self.show_title = tk.BooleanVar(value=self.config['show_title'])
        ttk.Checkbutton(frame_show, text="Show Title", variable=self.show_title,
                       command=lambda: self.config.update({'show_title': self.show_title.get()})).pack(anchor='w')
        
        frame_font = ttk.LabelFrame(tab, text="Font Sizes", padding=10)
        frame_font.pack(fill='x', padx=10, pady=10)
        
        ttk.Label(frame_font, text="FPS Font Size:").grid(row=0, column=0, sticky='w', pady=5)
        self.fps_font = tk.IntVar(value=self.config['font_fps'])
        ttk.Spinbox(frame_font, from_=10, to=40, textvariable=self.fps_font, width=10,
                   command=lambda: self.config.update({'font_fps': self.fps_font.get()})).grid(row=0, column=1, padx=10)
        
        ttk.Label(frame_font, text="Value Font Size:").grid(row=1, column=0, sticky='w', pady=5)
        self.val_font = tk.IntVar(value=self.config['font_val'])
        ttk.Spinbox(frame_font, from_=10, to=30, textvariable=self.val_font, width=10,
                   command=lambda: self.config.update({'font_val': self.val_font.get()})).grid(row=1, column=1, padx=10)
        
        ttk.Label(frame_font, text="Small Font Size:").grid(row=2, column=0, sticky='w', pady=5)
        self.small_font = tk.IntVar(value=self.config['font_small'])
        ttk.Spinbox(frame_font, from_=8, to=20, textvariable=self.small_font, width=10,
                   command=lambda: self.config.update({'font_small': self.small_font.get()})).grid(row=2, column=1, padx=10)
    
    def create_colors_tab(self, notebook):
        tab = ttk.Frame(notebook)
        notebook.add(tab, text="Colors")
        
        colors_frame = ttk.Frame(tab)
        colors_frame.pack(fill='both', expand=True, padx=10, pady=10)
        
        canvas = tk.Canvas(colors_frame, bg='#2b2b2b', highlightthickness=0)
        scrollbar = ttk.Scrollbar(colors_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        color_configs = [
            ("Background", "bg"),
            ("FPS OK (≥70%)", "fps_ok"),
            ("FPS Warning (45-70%)", "fps_warn"),
            ("FPS Critical (<45%)", "fps_crit"),
            ("Dim Text", "text_dim"),
            ("Separator Line", "separator")
        ]
        
        self.color_buttons = {}
        row = 0
        
        for label, key in color_configs:
            frame = ttk.Frame(scrollable_frame)
            frame.pack(fill='x', pady=5)
            
            ttk.Label(frame, text=f"{label}:", width=25).pack(side='left')
            
            # Get current RGB
            if key == "bg":
                rgb = (self.config['bg_r'], self.config['bg_g'], self.config['bg_b'])
            elif key == "fps_ok":
                rgb = (self.config['fps_ok_r'], self.config['fps_ok_g'], self.config['fps_ok_b'])
            elif key == "fps_warn":
                rgb = (self.config['fps_warn_r'], self.config['fps_warn_g'], self.config['fps_warn_b'])
            elif key == "fps_crit":
                rgb = (self.config['fps_crit_r'], self.config['fps_crit_g'], self.config['fps_crit_b'])
            elif key == "text_dim":
                rgb = (self.config['text_dim_r'], self.config['text_dim_g'], self.config['text_dim_b'])
            elif key == "separator":
                rgb = (self.config['sep_r'], self.config['sep_g'], self.config['sep_b'])
            
            hex_color = f'#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}'
            
            btn = tk.Button(frame, bg=hex_color, width=10, height=1,
                          command=lambda k=key: self.choose_color(k))
            btn.pack(side='left', padx=10)
            self.color_buttons[key] = btn
            
            rgb_label = ttk.Label(frame, text=f"RGB({rgb[0]}, {rgb[1]}, {rgb[2]})")
            rgb_label.pack(side='left', padx=5)
            self.color_buttons[f"{key}_label"] = rgb_label
            
            row += 1
        
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
    
    def choose_color(self, key):
        current = None
        if key == "bg":
            current = (self.config['bg_r'], self.config['bg_g'], self.config['bg_b'])
        elif key == "fps_ok":
            current = (self.config['fps_ok_r'], self.config['fps_ok_g'], self.config['fps_ok_b'])
        elif key == "fps_warn":
            current = (self.config['fps_warn_r'], self.config['fps_warn_g'], self.config['fps_warn_b'])
        elif key == "fps_crit":
            current = (self.config['fps_crit_r'], self.config['fps_crit_g'], self.config['fps_crit_b'])
        elif key == "text_dim":
            current = (self.config['text_dim_r'], self.config['text_dim_g'], self.config['text_dim_b'])
        elif key == "separator":
            current = (self.config['sep_r'], self.config['sep_g'], self.config['sep_b'])
        
        color = colorchooser.askcolor(title=f"Choose {key} color", 
                                       initialcolor=f'#{current[0]:02x}{current[1]:02x}{current[2]:02x}')
        if color[0]:
            rgb = (int(color[0][0]), int(color[0][1]), int(color[0][2]))
            
            if key == "bg":
                self.config['bg_r'], self.config['bg_g'], self.config['bg_b'] = rgb
            elif key == "fps_ok":
                self.config['fps_ok_r'], self.config['fps_ok_g'], self.config['fps_ok_b'] = rgb
            elif key == "fps_warn":
                self.config['fps_warn_r'], self.config['fps_warn_g'], self.config['fps_warn_b'] = rgb
            elif key == "fps_crit":
                self.config['fps_crit_r'], self.config['fps_crit_g'], self.config['fps_crit_b'] = rgb
            elif key == "text_dim":
                self.config['text_dim_r'], self.config['text_dim_g'], self.config['text_dim_b'] = rgb
            elif key == "separator":
                self.config['sep_r'], self.config['sep_g'], self.config['sep_b'] = rgb
            
            hex_color = f'#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}'
            self.color_buttons[key].configure(bg=hex_color)
            self.color_buttons[f"{key}_label"].configure(text=f"RGB({rgb[0]}, {rgb[1]}, {rgb[2]})")
    
    def create_advanced_tab(self, notebook):
        tab = ttk.Frame(notebook)
        notebook.add(tab, text="Advanced")
        
        frame_window = ttk.LabelFrame(tab, text="Window Settings", padding=10)
        frame_window.pack(fill='x', padx=10, pady=10)
        
        ttk.Label(frame_window, text="Window Width:").grid(row=0, column=0, sticky='w', pady=5)
        self.win_width = tk.IntVar(value=self.config['width'])
        ttk.Spinbox(frame_window, from_=150, to=400, textvariable=self.win_width, width=10,
                   command=lambda: self.config.update({'width': self.win_width.get()})).grid(row=0, column=1, padx=10)
        
        ttk.Label(frame_window, text="Window Height:").grid(row=1, column=0, sticky='w', pady=5)
        self.win_height = tk.IntVar(value=self.config['height'])
        ttk.Spinbox(frame_window, from_=80, to=300, textvariable=self.win_height, width=10,
                   command=lambda: self.config.update({'height': self.win_height.get()})).grid(row=1, column=1, padx=10)
        
        ttk.Label(frame_window, text="Transparency (0-1):").grid(row=2, column=0, sticky='w', pady=5)
        self.alpha = tk.DoubleVar(value=self.config['alpha'])
        ttk.Scale(frame_window, from_=0.3, to=1.0, variable=self.alpha, orient='horizontal',
                 command=lambda x: self.config.update({'alpha': self.alpha.get()})).grid(row=2, column=1, padx=10, sticky='ew')
        
        ttk.Label(frame_window, text=f"Value: {self.alpha.get():.2f}").grid(row=2, column=2, padx=5)
        
        frame_update = ttk.LabelFrame(tab, text="Update Intervals", padding=10)
        frame_update.pack(fill='x', padx=10, pady=10)
        
        ttk.Label(frame_update, text="Metrics Update (ms):").grid(row=0, column=0, sticky='w', pady=5)
        self.metrics_ms = tk.IntVar(value=self.config['update_metrics'])
        ttk.Spinbox(frame_update, from_=200, to=2000, textvariable=self.metrics_ms, width=10,
                   command=lambda: self.config.update({'update_metrics': self.metrics_ms.get()})).grid(row=0, column=1, padx=10)
        
        ttk.Label(frame_update, text="Overlay FPS (ms):").grid(row=1, column=0, sticky='w', pady=5)
        self.overlay_fps = tk.IntVar(value=self.config['update_overlay'])
        ttk.Spinbox(frame_update, from_=16, to=100, textvariable=self.overlay_fps, width=10,
                   command=lambda: self.config.update({'update_overlay': self.overlay_fps.get()})).grid(row=1, column=1, padx=10)
        
        frame_pos = ttk.LabelFrame(tab, text="Position", padding=10)
        frame_pos.pack(fill='x', padx=10, pady=10)
        
        ttk.Label(frame_pos, text="Default Corner:").grid(row=0, column=0, sticky='w', pady=5)
        corners = ["top_left", "top_right", "bottom_left", "bottom_right"]
        self.corner = tk.StringVar(value=self.config['corner'])
        corner_combo = ttk.Combobox(frame_pos, textvariable=self.corner, values=corners, state='readonly', width=15)
        corner_combo.grid(row=0, column=1, padx=10)
        corner_combo.bind('<<ComboboxSelected>>', lambda e: self.config.update({'corner': self.corner.get()}))
        
        ttk.Label(frame_pos, text="Margin (px):").grid(row=1, column=0, sticky='w', pady=5)
        self.margin = tk.IntVar(value=self.config['margin'])
        ttk.Spinbox(frame_pos, from_=0, to=50, textvariable=self.margin, width=10,
                   command=lambda: self.config.update({'margin': self.margin.get()})).grid(row=1, column=1, padx=10)
    
    def reset_config(self):
        if messagebox.askyesno("Reset", "Reset all settings to default?"):
            self.config = DEFAULT_CONFIG.copy()
            messagebox.showinfo("Reset", "Settings reset to default. Restart overlay to apply changes.")
            self.root.destroy()
            self.__init__()
    
    def preview(self):
        preview_window = tk.Toplevel(self.root)
        preview_window.title("Overlay Preview")
        preview_window.geometry(f"{self.config['width']}x{self.config['height']}")
        
        bg_color = f'#{self.config["bg_r"]:02x}{self.config["bg_g"]:02x}{self.config["bg_b"]:02x}'
        preview_window.configure(bg=bg_color)
        preview_window.attributes('-alpha', self.config['alpha'])
        preview_window.attributes('-topmost', True)
        
        # Preview content
        colors = {
            'fps_ok': f'#{self.config["fps_ok_r"]:02x}{self.config["fps_ok_g"]:02x}{self.config["fps_ok_b"]:02x}',
            'fps_warn': f'#{self.config["fps_warn_r"]:02x}{self.config["fps_warn_g"]:02x}{self.config["fps_warn_b"]:02x}',
            'fps_crit': f'#{self.config["fps_crit_r"]:02x}{self.config["fps_crit_g"]:02x}{self.config["fps_crit_b"]:02x}',
            'text_dim': f'#{self.config["text_dim_r"]:02x}{self.config["text_dim_g"]:02x}{self.config["text_dim_b"]:02x}',
            'separator': f'#{self.config["sep_r"]:02x}{self.config["sep_g"]:02x}{self.config["sep_b"]:02x}'
        }
        
        # Title
        if self.config['show_title']:
            title = tk.Label(preview_window, text="MONITOR", 
                            font=('Consolas', self.config['font_small']),
                            fg=colors['text_dim'], bg=bg_color)
            title.pack(anchor='nw', padx=8, pady=2)
        
        # Separator
        sep_frame = tk.Frame(preview_window, height=1, bg=colors['separator'])
        sep_frame.pack(fill='x', padx=8, pady=(0, 8))
        
        # FPS
        if self.config['show_fps']:
            fps_label = tk.Label(preview_window, text="FPS  60",
                                font=('Consolas', self.config['font_fps'], 'bold'),
                                fg=colors['fps_ok'], bg=bg_color)
            fps_label.pack(anchor='nw', padx=8, pady=(0, 5))
        
        # CPU
        if self.config['show_cpu']:
            cpu_label = tk.Label(preview_window, text="CPU  45.2%",
                                font=('Consolas', self.config['font_val']),
                                fg=colors['fps_ok'], bg=bg_color)
            cpu_label.pack(anchor='nw', padx=8, pady=(0, 5))
        
        # GPU
        if self.config['show_gpu']:
            gpu_label = tk.Label(preview_window, text="GPU  78.5%",
                                font=('Consolas', self.config['font_val']),
                                fg=colors['fps_warn'], bg=bg_color)
            gpu_label.pack(anchor='nw', padx=8, pady=(0, 5))
        
        # Hint
        hint_label = tk.Label(preview_window, text="RMB menu  |  LMB drag",
                             font=('Consolas', self.config['font_small']),
                             fg=colors['text_dim'], bg=bg_color)
        hint_label.pack(anchor='nw', padx=8, pady=(0, 5))
        
        tk.Button(preview_window, text="Close", command=preview_window.destroy).pack(pady=10)
    
    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    if getattr(sys, 'frozen', False):
        os.chdir(os.path.dirname(sys.executable))
    
    app = OverlaySettings()
    app.run()