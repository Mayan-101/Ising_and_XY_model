import os
import glob
import csv
import matplotlib.pyplot as plt

def plot_file(csv_path):
    print(f"Reading data from {csv_path}...")
    h_desc, M_desc = [], []
    h_asc, M_asc = [], []
    
    try:
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                h = float(row['h'])
                M = float(row['M'])
                branch = int(row['branch'])
                if branch == 0:
                    h_desc.append(h)
                    M_desc.append(M)
                else:
                    h_asc.append(h)
                    M_asc.append(M)
    except Exception as e:
        print(f"Error reading {csv_path}: {e}")
        return

    if not h_desc or not h_asc:
        print(f"Warning: Missing branch data in {csv_path}. Cannot plot hysteresis.")
        return

    # Create a beautiful premium layout
    plt.figure(figsize=(9, 6), dpi=150)
    plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
    
    # Custom vibrant palettes
    color_desc = '#E31A1C' # Strong vibrant red
    color_asc = '#1F78B4'  # Deep beautiful blue
    
    # Grid and background adjustments
    ax = plt.gca()
    ax.set_facecolor('#F8F9FA')
    ax.grid(True, linestyle='--', alpha=0.6, color='#CCCCCC')
    
    # Plot Descending (Branch 0)
    plt.plot(h_desc, M_desc, color=color_desc, linewidth=2.5, label='Descending Field (Branch 0)')
    plt.scatter(h_desc[::max(1, len(h_desc)//15)], M_desc[::max(1, len(M_desc)//15)], 
                color=color_desc, s=40, edgecolors='black', zorder=5)
    
    # Plot Ascending (Branch 1)
    plt.plot(h_asc, M_asc, color=color_asc, linewidth=2.5, label='Ascending Field (Branch 1)')
    plt.scatter(h_asc[::max(1, len(h_asc)//15)], M_asc[::max(1, len(M_asc)//15)], 
                color=color_asc, s=40, edgecolors='black', zorder=5)
    
    # Draw directional arrows to make the sweep paths intuitive
    def add_arrows(x, y, color):
        # Draw a couple of directional arrows along the path
        mid = len(x) // 2
        quarter = len(x) // 4
        three_quarters = (3 * len(x)) // 4
        step = max(1, len(x) // 15)
        for idx in [quarter, mid, three_quarters]:
            if idx + step < len(x):
                dx = x[idx+step] - x[idx]
                dy = y[idx+step] - y[idx]
                # Normalize direction
                length = (dx**2 + dy**2)**0.5
                if length > 0:
                    ax.annotate('', xy=(x[idx+step], y[idx+step]), xytext=(x[idx], y[idx]),
                                arrowprops=dict(arrowstyle="->", color=color, lw=2.5, mutation_scale=15),
                                zorder=4)

    add_arrows(h_desc, M_desc, color_desc)
    add_arrows(h_asc, M_asc, color_asc)

    # Title & Labels
    basename = os.path.basename(csv_path)
    model_name = "Ising Model" if "ising" in basename.lower() else "XY Model"
    device_name = "GPU" if "gpu" in basename.lower() else "CPU"
    
    plt.title(f"Hysteresis Loop - {model_name} ({device_name})", fontsize=14, fontweight='bold', pad=15, color='#2C3E50')
    plt.xlabel("External Magnetic Field (h)", fontsize=12, fontweight='medium', labelpad=10, color='#2C3E50')
    plt.ylabel("Magnetization (M)", fontsize=12, fontweight='medium', labelpad=10, color='#2C3E50')
    
    # Limits and reference lines
    plt.axhline(0, color='#000000', linewidth=1.0, linestyle='-', alpha=0.3)
    plt.axvline(0, color='#000000', linewidth=1.0, linestyle='-', alpha=0.3)
    
    # Legend styling
    plt.legend(frameon=True, facecolor='white', edgecolor='#E2E8F0', framealpha=0.9, fontsize=10, loc='best')
    
    # Determine output path (handle if run from within plotter/ directory)
    if os.path.basename(os.getcwd()) == 'plotter':
        out_dir = "."
    else:
        out_dir = "plotter"
        
    os.makedirs(out_dir, exist_ok=True)
    plot_name = basename.replace('.csv', '_plot.png')
    if basename == "ising_hysteresis.csv":
        plot_name = "ising_plot.png"
    elif basename == "xy_hysteresis.csv":
        plot_name = "xy_plot.png"
    elif basename == "ising_gpu_hysteresis.csv":
        plot_name = "ising_gpu_plot.png"
        
    out_path = os.path.join(out_dir, plot_name)
    plt.tight_layout()
    plt.savefig(out_path, dpi=200)
    plt.close()
    print(f"Successfully generated and saved plot to: {out_path}")

def main():
    print("=========================================")
    print("      Kawasaki Hysteresis Plotter       ")
    print("=========================================")
    
    # Search in current directory first
    csv_files = glob.glob("*_hysteresis.csv")
    
    # Fallback to parent directory if empty
    if not csv_files:
        csv_files = glob.glob("../*_hysteresis.csv")
        
    if not csv_files:
        print("No *_hysteresis.csv files found in the current or parent directory.")
        return
        
    for csv_file in csv_files:
        plot_file(csv_file)
    print("\nAll plots generated successfully!")

if __name__ == '__main__':
    main()
