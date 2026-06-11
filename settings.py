import os
import sys
import tkinter as tk
from tkinter import ttk, colorchooser, messagebox

CONFIG_FILE = "overlay.ini"

DEFAULT_CONFIG = {
    "width": 200, "height": 110, "alpha": 0.88, "margin": 14, "corner": "top_right",
    "bg_r": 10, "bg_g": 10, "bg_b": 10,
    "fps_ok_r": 0, "fps_ok_g": 220, "fps_ok_b": 120,
    "fps_warn_r": 255, "fps_warn_g": 170, "fps_warn_b": 0,
    "fps_crit_r": 255, "fps_crit_g": 50, "fps_crit_b": 70,
    "text_dim_r": 60, "text_dim_g": 100, "text_dim_b": 75,
    "sep_r": 40, "sep_g": 70, "sep_b": 50,
    "show_fps": True, "show_cpu": True, "show_gpu": True, "show_title": True,
    "font_fps": 18, "font_val": 14, "font_small": 10,
    "update_metrics": 800, "update_overlay": 33
}

COLOR_KEYS = [
    ("Background", "bg", "bg_r", "bg_g", "bg_b"),
    ("FPS OK (≥70%)", "fps_ok", "fps_ok_r", "fps_ok_g", "fps_ok_b"),
    ("FPS Warning (45-70%)", "fps_warn", "fps_warn_r", "fps_warn_g", "fps_warn_b"),
    ("FPS Critical (<45%)", "fps_crit", "fps_crit_r", "fps_crit_g", "fps_crit_b"),
    ("Dim Text", "text_dim", "text_dim_r", "text_dim_g", "text_dim_b"),
    ("Separator Line", "separator", "sep_r", "sep_g", "sep_b"),
]

class OverlaySettings:
    def __init__(self):
        self.config = DEFAULT_CONFIG.copy()
        self.load_config()
        
        self.root = tk.Tk()
        self.root.title("Overlay Settings")
        self.root.geometry("500x650")
        self.root.configure(bg='#2b2b2b')
        
        style = ttk.Style()
        style.theme_use('clam')
        for s in ('TLabel', 'TFrame', 'TLabelframe', 'TLabelframe.Label'):
            style.configure(s, background='#2b2b2b', foreground='white')
        style.configure('TButton', background='#3c3c3c', foreground='white')
        style.map('TButton', background=[('active', '#4c4c4c')])
        
        self.create_widgets()
    
    def load_config(self):
        if not os.path.exists(CONFIG_FILE):
            return
        with open(CONFIG_FILE, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line[0] in (';', '#', '[') or '=' not in line:
                    continue
                key, val = line.split('=', 1)
                key, val = key.strip(), val.strip()
                
                if key in self.config:
                    if isinstance(self.config[key], bool):
                        self.config[key] = val.lower() in ('true', '1')
                    elif isinstance(self.config[key], int):
                        self.config[key] = int(val)
                    elif isinstance(self.config[key], float):
                        self.config[key] = float(val)
                    else:
                        self.config[key] = val
                
                # Parse RGB tuples
                for _, _, rk, gk, bk in COLOR_KEYS:
                    if key == rk[:-2]:
                        parts = val.split(',')
                        if len(parts) == 3:
                            self.config[rk], self.config[gk], self.config[bk] = map(int, parts)
                        break
    
    def save_config(self):
        with open(CONFIG_FILE, 'w') as f:
            f.write("; Overlay Configuration File\n\n[Window]\n")
            f.write(f"width={self.config['width']}\nheight={self.config['height']}\n")
            f.write(f"alpha={self.config['alpha']}\nmargin={self.config['margin']}\n")
            f.write(f"corner={self.config['corner']}\n\n[Colors]\n")
            
            for label, key, rk, gk, bk in COLOR_KEYS:
                f.write(f"{key}={self.config[rk]},{self.config[gk]},{self.config[bk]}\n")
            
            f.write("\n[Display]\n")
            for k in ('show_fps', 'show_cpu', 'show_gpu', 'show_title'):
                f.write(f"{k}={'true' if self.config[k] else 'false'}\n")
            f.write(f"font_fps={self.config['font_fps']}\nfont_val={self.config['font_val']}\n")
            f.write(f"font_small={self.config['font_small']}\n\n[Update]\n")
            f.write(f"update_metrics={self.config['update_metrics']}\n")
            f.write(f"update_overlay={self.config['update_overlay']}\n")
        
        messagebox.showinfo("Success", "Settings saved!\nRestart overlay to apply changes.")
    
    def preview(self):
        win = tk.Toplevel(self.root)
        win.title("Overlay Preview")
        win.geometry(f"{self.config['width']}x{self.config['height']}")
        win.attributes('-alpha', self.config['alpha'], '-topmost', True)
        
        bg = f'#{self.config["bg_r"]:02x}{self.config["bg_g"]:02x}{self.config["bg_b"]:02x}'
        win.configure(bg=bg)
        
        def rgb(key):
            return f'#{self.config[f"{key}_r"]:02x}{self.config[f"{key}_g"]:02x}{self.config[f"{key}_b"]:02x}'
        
        font_fps = ('Consolas', self.config['font_fps'], 'bold')
        font_val = ('Consolas', self.config['font_val'])
        font_small = ('Consolas', self.config['font_small'])
        
        if self.config['show_title']:
            tk.Label(win, text="MONITOR", font=font_small, fg=rgb('text_dim'), bg=bg).pack(anchor='nw', padx=8, pady=2)
        
        tk.Frame(win, height=1, bg=rgb('separator')).pack(fill='x', padx=8, pady=(0, 8))
        
        if self.config['show_fps']:
            tk.Label(win, text="FPS  60", font=font_fps, fg=rgb('fps_ok'), bg=bg).pack(anchor='nw', padx=8, pady=(0, 5))
        if self.config['show_cpu']:
            tk.Label(win, text="CPU  45.2%", font=font_val, fg=rgb('fps_ok'), bg=bg).pack(anchor='nw', padx=8, pady=(0, 5))
        if self.config['show_gpu']:
            tk.Label(win, text="GPU  78.5%", font=font_val, fg=rgb('fps_warn'), bg=bg).pack(anchor='nw', padx=8, pady=(0, 5))
        
        tk.Label(win, text="RMB menu  |  LMB drag", font=font_small, fg=rgb('text_dim'), bg=bg).pack(anchor='nw', padx=8, pady=(0, 5))
        tk.Button(win, text="Close", command=win.destroy).pack(pady=10)
    
    def create_widgets(self):
        nb = ttk.Notebook(self.root)
        nb.pack(fill='both', expand=True, padx=10, pady=10)
        
        # Display tab
        tab1 = ttk.Frame(nb)
        nb.add(tab1, text="Display")
        
        f1 = ttk.LabelFrame(tab1, text="Show/Hide", padding=10)
        f1.pack(fill='x', padx=10, pady=5)
        for k in ('show_fps', 'show_cpu', 'show_gpu', 'show_title'):
            var = tk.BooleanVar(value=self.config[k])
            var.trace('w', lambda *a, k=k, v=var: self.config.update({k: v.get()}))
            ttk.Checkbutton(f1, text=k[5:].upper(), variable=var).pack(anchor='w')
        
        f2 = ttk.LabelFrame(tab1, text="Font Sizes", padding=10)
        f2.pack(fill='x', padx=10, pady=5)
        for i, (label, key) in enumerate([("FPS:", "font_fps"), ("Value:", "font_val"), ("Small:", "font_small")]):
            ttk.Label(f2, text=label).grid(row=i, column=0, sticky='w', pady=3)
            var = tk.IntVar(value=self.config[key])
            var.trace('w', lambda *a, k=key, v=var: self.config.update({k: v.get()}))
            ttk.Spinbox(f2, from_=8, to=40, textvariable=var, width=8).grid(row=i, column=1, padx=10)
        
        # Colors tab
        tab2 = ttk.Frame(nb)
        nb.add(tab2, text="Colors")
        
        canvas = tk.Canvas(tab2, bg='#2b2b2b', highlightthickness=0)
        scroll = ttk.Scrollbar(tab2, orient='vertical', command=canvas.yview)
        inner = ttk.Frame(canvas)
        inner.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=inner, anchor='nw')
        canvas.configure(yscrollcommand=scroll.set)
        canvas.pack(side='left', fill='both', expand=True)
        scroll.pack(side='right', fill='y')
        
        self.color_btns = {}
        for label, key, rk, gk, bk in COLOR_KEYS:
            frame = ttk.Frame(inner)
            frame.pack(fill='x', pady=5, padx=10)
            ttk.Label(frame, text=f"{label}:", width=22).pack(side='left')
            rgb = (self.config[rk], self.config[gk], self.config[bk])
            btn = tk.Button(frame, bg=f'#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}', width=10,
                           command=lambda k=key, r=rk, g=gk, b=bk: self.choose_color(k, r, g, b))
            btn.pack(side='left', padx=5)
            lbl = ttk.Label(frame, text=f"RGB({rgb[0]},{rgb[1]},{rgb[2]})")
            lbl.pack(side='left')
            self.color_btns[key] = (btn, lbl, (rk, gk, bk))
        
        # Advanced tab
        tab3 = ttk.Frame(nb)
        nb.add(tab3, text="Advanced")
        
        f3 = ttk.LabelFrame(tab3, text="Window", padding=10)
        f3.pack(fill='x', padx=10, pady=5)
        for i, (label, key, from_, to_) in enumerate([
            ("Width:", "width", 150, 400), ("Height:", "height", 80, 300),
            ("Margin:", "margin", 0, 50)
        ]):
            ttk.Label(f3, text=label).grid(row=i, column=0, sticky='w', pady=3)
            var = tk.IntVar(value=self.config[key])
            var.trace('w', lambda *a, k=key, v=var: self.config.update({k: v.get()}))
            ttk.Spinbox(f3, from_=from_, to=to_, textvariable=var, width=8).grid(row=i, column=1, padx=10)
        
        ttk.Label(f3, text="Alpha:").grid(row=3, column=0, sticky='w', pady=3)
        self.alpha_var = tk.DoubleVar(value=self.config['alpha'])
        self.alpha_var.trace('w', lambda *a: self.config.update({'alpha': self.alpha_var.get()}))
        ttk.Scale(f3, from_=0.3, to=1.0, variable=self.alpha_var, orient='horizontal').grid(row=3, column=1, padx=10, sticky='ew')
        
        ttk.Label(f3, text="Corner:").grid(row=4, column=0, sticky='w', pady=3)
        corner_var = tk.StringVar(value=self.config['corner'])
        corner_var.trace('w', lambda *a: self.config.update({'corner': corner_var.get()}))
        ttk.Combobox(f3, textvariable=corner_var, values=["top_left","top_right","bottom_left","bottom_right"], state='readonly', width=12).grid(row=4, column=1, padx=10)
        
        f4 = ttk.LabelFrame(tab3, text="Update (ms)", padding=10)
        f4.pack(fill='x', padx=10, pady=5)
        for i, (label, key, from_, to_) in enumerate([("Metrics:", "update_metrics", 200, 2000), ("Overlay:", "update_overlay", 16, 100)]):
            ttk.Label(f4, text=label).grid(row=i, column=0, sticky='w', pady=3)
            var = tk.IntVar(value=self.config[key])
            var.trace('w', lambda *a, k=key, v=var: self.config.update({k: v.get()}))
            ttk.Spinbox(f4, from_=from_, to=to_, textvariable=var, width=8).grid(row=i, column=1, padx=10)
        
        # Buttons
        btn_frame = ttk.Frame(self.root)
        btn_frame.pack(fill='x', padx=10, pady=(0, 10))
        ttk.Button(btn_frame, text="Preview", command=self.preview).pack(side='right', padx=5)
        ttk.Button(btn_frame, text="Save", command=self.save_config).pack(side='right', padx=5)
        ttk.Button(btn_frame, text="Reset", command=self.reset_config).pack(side='right', padx=5)
    
    def choose_color(self, key, rk, gk, bk):
        color = colorchooser.askcolor(
            title=f"Choose {key}",
            initialcolor=f'#{self.config[rk]:02x}{self.config[gk]:02x}{self.config[bk]:02x}'
        )
        if color[0]:
            rgb = tuple(map(int, color[0]))
            self.config[rk], self.config[gk], self.config[bk] = rgb
            btn, lbl, _ = self.color_btns[key]
            btn.configure(bg=f'#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}')
            lbl.configure(text=f"RGB({rgb[0]},{rgb[1]},{rgb[2]})")
    
    def reset_config(self):
        if messagebox.askyesno("Reset", "Reset all settings to default?"):
            self.config = DEFAULT_CONFIG.copy()
            messagebox.showinfo("Reset", "Settings reset to default.")
            self.root.destroy()
            self.__init__()
            self.run()
    
    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    if getattr(sys, 'frozen', False):
        os.chdir(os.path.dirname(sys.executable))
    OverlaySettings().run()