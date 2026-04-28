"""
Mini OS Kernel Simulator — Analysis & Visualization Suite
Generates all charts and figures for the technical report.

Usage:
    python analysis/generate_report_figures.py
"""

import json
import os
import sys

# Try importing visualization libraries
try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] matplotlib not found. Install with: pip install matplotlib numpy")
    print("          Will generate text-based reports instead.")


OUTPUT_DIR = "output/figures"
LOG_DIR = "output/logs"
SCHEDULERS = ["fcfs", "rr", "priority"]
SCHEDULER_LABELS = {"fcfs": "FCFS", "rr": "Round Robin", "priority": "Priority (P)"}
COLORS = {
    "fcfs": "#FF6B6B",
    "rr": "#4ECDC4",
    "priority": "#45B7D1"
}


def load_json(filepath):
    """Load a JSON file and return the parsed data."""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"  [WARN] Could not load {filepath}: {e}")
        return None


def load_all_scheduling_metrics():
    """Load scheduling metrics for all schedulers."""
    metrics = {}
    for sched in SCHEDULERS:
        path = os.path.join(LOG_DIR, sched, "scheduling_metrics.json")
        data = load_json(path)
        if data:
            metrics[sched] = data
    return metrics


def load_all_memory_metrics():
    """Load memory metrics for all schedulers."""
    metrics = {}
    for sched in SCHEDULERS:
        path = os.path.join(LOG_DIR, sched, "memory_metrics.json")
        data = load_json(path)
        if data:
            metrics[sched] = data
    return metrics


def load_scheduling_csv(sched):
    """Load scheduling log CSV for a given scheduler."""
    path = os.path.join(LOG_DIR, sched, "scheduling_log.csv")
    events = []
    try:
        with open(path, 'r') as f:
            header = f.readline().strip().split(',')
            for line in f:
                parts = line.strip().split(',')
                if len(parts) >= 5:
                    events.append({
                        'tick': int(parts[0]),
                        'event': parts[1],
                        'pid': int(parts[2]),
                        'process_name': parts[3],
                        'scheduler': parts[4],
                        'detail': parts[5] if len(parts) > 5 else ''
                    })
    except FileNotFoundError:
        pass
    return events


# ======================================================================
# Figure 1: Scheduling Algorithm Comparison (Bar Charts)
# ======================================================================
def plot_scheduling_comparison(metrics):
    """Generate bar charts comparing scheduling algorithms."""
    if not HAS_MATPLOTLIB or not metrics:
        print("  [SKIP] No matplotlib or no data for scheduling comparison")
        return

    fig, axes = plt.subplots(2, 3, figsize=(18, 10))
    fig.suptitle('CPU Scheduling Algorithm Comparison', fontsize=16, fontweight='bold', y=0.98)

    chart_data = {
        'avg_waiting_time': ('Avg Waiting Time', 'Ticks'),
        'avg_turnaround_time': ('Avg Turnaround Time', 'Ticks'),
        'avg_response_time': ('Avg Response Time', 'Ticks'),
        'cpu_utilization': ('CPU Utilization', '%'),
        'throughput': ('Throughput', 'Processes/Tick'),
        'context_switches': ('Context Switches', 'Count')
    }

    for idx, (key, (title, ylabel)) in enumerate(chart_data.items()):
        ax = axes[idx // 3][idx % 3]
        names = []
        values = []
        colors = []

        for sched in SCHEDULERS:
            if sched in metrics:
                names.append(SCHEDULER_LABELS[sched])
                values.append(metrics[sched].get(key, 0))
                colors.append(COLORS[sched])

        bars = ax.bar(names, values, color=colors, edgecolor='white', linewidth=1.5)
        ax.set_title(title, fontsize=12, fontweight='bold')
        ax.set_ylabel(ylabel, fontsize=10)
        ax.grid(axis='y', alpha=0.3)

        # Add value labels on bars
        for bar, val in zip(bars, values):
            ax.text(bar.get_x() + bar.get_width()/2., bar.get_height() + 0.5,
                    f'{val:.2f}' if isinstance(val, float) else str(val),
                    ha='center', va='bottom', fontsize=9, fontweight='bold')

    plt.tight_layout(rect=[0, 0, 1, 0.95])
    filepath = os.path.join(OUTPUT_DIR, "scheduling_comparison.png")
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  [OK] Generated: {filepath}")


# ======================================================================
# Figure 2: Per-Process Metrics (Grouped Bar Chart)
# ======================================================================
def plot_per_process_metrics(metrics):
    """Generate per-process comparison chart."""
    if not HAS_MATPLOTLIB or not metrics:
        return

    fig, axes = plt.subplots(1, 3, figsize=(18, 6))
    fig.suptitle('Per-Process Scheduling Metrics', fontsize=16, fontweight='bold')

    metric_keys = [
        ('waiting', 'Waiting Time (ticks)'),
        ('turnaround', 'Turnaround Time (ticks)'),
        ('response', 'Response Time (ticks)')
    ]

    for ax_idx, (key, ylabel) in enumerate(metric_keys):
        ax = axes[ax_idx]
        all_pids = set()
        for sched in SCHEDULERS:
            if sched in metrics and 'per_process' in metrics[sched]:
                for p in metrics[sched]['per_process']:
                    all_pids.add(p['pid'])
        pids = sorted(all_pids)

        x = np.arange(len(pids))
        width = 0.25

        for i, sched in enumerate(SCHEDULERS):
            if sched in metrics and 'per_process' in metrics[sched]:
                pid_map = {p['pid']: p[key] for p in metrics[sched]['per_process']}
                values = [pid_map.get(pid, 0) for pid in pids]
                ax.bar(x + i * width, values, width, label=SCHEDULER_LABELS[sched],
                       color=COLORS[sched], edgecolor='white')

        ax.set_xlabel('Process ID')
        ax.set_ylabel(ylabel)
        ax.set_title(ylabel.split('(')[0].strip())
        ax.set_xticks(x + width)
        ax.set_xticklabels([f'P{pid}' for pid in pids])
        ax.legend()
        ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    filepath = os.path.join(OUTPUT_DIR, "per_process_metrics.png")
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  [OK] Generated: {filepath}")


# ======================================================================
# Figure 3: Gantt Chart for each scheduler
# ======================================================================
def plot_gantt_charts(metrics):
    """Generate Gantt charts showing scheduling timeline."""
    if not HAS_MATPLOTLIB:
        return

    fig, axes = plt.subplots(len(SCHEDULERS), 1, figsize=(16, 4 * len(SCHEDULERS)))
    fig.suptitle('CPU Scheduling Gantt Charts', fontsize=16, fontweight='bold')

    process_colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8',
                      '#F7DC6F', '#BB8FCE', '#85C1E9', '#F0B27A', '#82E0AA']

    for idx, sched in enumerate(SCHEDULERS):
        ax = axes[idx] if len(SCHEDULERS) > 1 else axes
        events = load_scheduling_csv(sched)

        if not events:
            ax.set_title(f'{SCHEDULER_LABELS[sched]} (no data)')
            continue

        # Build timeline from events
        processes = {}
        current_pid = -1
        current_start = 0

        for event in events:
            if event['event'] == 'DISPATCHED':
                current_pid = event['pid']
                current_start = event['tick']
                if current_pid not in processes:
                    processes[current_pid] = {'name': event['process_name'], 'runs': []}
            elif event['event'] in ('PREEMPTED', 'COMPLETED') and current_pid == event['pid']:
                if current_pid in processes:
                    processes[current_pid]['runs'].append((current_start, event['tick']))
                current_pid = -1

        # Draw bars
        pid_list = sorted(processes.keys())
        for i, pid in enumerate(pid_list):
            color = process_colors[i % len(process_colors)]
            for start, end in processes[pid]['runs']:
                ax.barh(i, end - start, left=start, height=0.6,
                       color=color, edgecolor='white', linewidth=0.5)

        ax.set_yticks(range(len(pid_list)))
        ax.set_yticklabels([processes[pid]['name'] for pid in pid_list])
        ax.set_xlabel('Time (ticks)')
        ax.set_title(f'{SCHEDULER_LABELS[sched]} Scheduler', fontweight='bold')
        ax.grid(axis='x', alpha=0.3)
        ax.invert_yaxis()

    plt.tight_layout(rect=[0, 0, 1, 0.95])
    filepath = os.path.join(OUTPUT_DIR, "gantt_charts.png")
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  [OK] Generated: {filepath}")


# ======================================================================
# Figure 4: Memory Metrics Comparison
# ======================================================================
def plot_memory_comparison(mem_metrics):
    """Generate memory metrics comparison chart."""
    if not HAS_MATPLOTLIB or not mem_metrics:
        return

    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
    fig.suptitle('Memory Management Metrics', fontsize=16, fontweight='bold')

    chart_items = [
        ('total_page_faults', 'Total Page Faults', 'Count'),
        ('page_fault_rate', 'Page Fault Rate', 'Rate'),
        ('frame_utilization', 'Frame Utilization', '%')
    ]

    for ax_idx, (key, title, ylabel) in enumerate(chart_items):
        ax = axes[ax_idx]
        names = []
        values = []
        colors = []
        for sched in SCHEDULERS:
            if sched in mem_metrics:
                names.append(SCHEDULER_LABELS[sched])
                val = mem_metrics[sched].get(key, 0)
                if key == 'page_fault_rate':
                    val *= 100  # convert to percentage
                values.append(val)
                colors.append(COLORS[sched])

        bars = ax.bar(names, values, color=colors, edgecolor='white', linewidth=1.5)
        ax.set_title(title, fontweight='bold')
        ax.set_ylabel(ylabel)
        ax.grid(axis='y', alpha=0.3)

        for bar, val in zip(bars, values):
            ax.text(bar.get_x() + bar.get_width()/2., bar.get_height() + 0.3,
                    f'{val:.2f}', ha='center', va='bottom', fontsize=9, fontweight='bold')

    plt.tight_layout()
    filepath = os.path.join(OUTPUT_DIR, "memory_comparison.png")
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  [OK] Generated: {filepath}")


# ======================================================================
# Figure 5: Page Replacement Policy Comparison (FIFO vs LRU)
# ======================================================================
def plot_page_replacement_comparison():
    """Generate FIFO vs LRU comparison for the textbook reference string."""
    if not HAS_MATPLOTLIB:
        return

    # Classic textbook reference string
    ref_string = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]
    num_frames = 3

    def simulate_fifo(refs, frames):
        memory = []
        faults = []
        states = []
        for page in refs:
            if page in memory:
                faults.append(0)
            else:
                faults.append(1)
                if len(memory) >= frames:
                    memory.pop(0)
                memory.append(page)
            states.append(list(memory))
        return faults, states

    def simulate_lru(refs, frames):
        memory = []
        faults = []
        states = []
        for page in refs:
            if page in memory:
                faults.append(0)
                memory.remove(page)
                memory.append(page)
            else:
                faults.append(1)
                if len(memory) >= frames:
                    memory.pop(0)
                memory.append(page)
            states.append(list(memory))
        return faults, states

    fifo_faults, fifo_states = simulate_fifo(ref_string, num_frames)
    lru_faults, lru_states = simulate_lru(ref_string, num_frames)

    fig, axes = plt.subplots(3, 1, figsize=(18, 10))
    fig.suptitle('Page Replacement Policy Comparison\nReference String: ' +
                 ' '.join(map(str, ref_string)), fontsize=14, fontweight='bold')

    # Plot 1: Reference string with fault markers
    ax = axes[0]
    x = range(len(ref_string))
    ax.bar(x, ref_string, color=['#FF6B6B' if fifo_faults[i] else '#4ECDC4' for i in x],
           edgecolor='white', linewidth=1)
    ax.set_ylabel('Page Number')
    ax.set_title('Reference String (Red = Page Fault in FIFO)')
    ax.set_xticks(x)
    ax.set_xticklabels([str(r) for r in ref_string])
    ax.grid(axis='y', alpha=0.3)

    # Plot 2: Cumulative faults comparison
    ax = axes[1]
    fifo_cumulative = np.cumsum(fifo_faults)
    lru_cumulative = np.cumsum(lru_faults)
    ax.plot(x, fifo_cumulative, 'o-', color='#FF6B6B', linewidth=2, markersize=6, label=f'FIFO ({sum(fifo_faults)} faults)')
    ax.plot(x, lru_cumulative, 's-', color='#4ECDC4', linewidth=2, markersize=6, label=f'LRU ({sum(lru_faults)} faults)')
    ax.set_xlabel('Access Number')
    ax.set_ylabel('Cumulative Page Faults')
    ax.set_title('Cumulative Page Faults: FIFO vs LRU')
    ax.legend(fontsize=12)
    ax.grid(alpha=0.3)

    # Plot 3: Frame state visualization
    ax = axes[2]
    frame_colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8',
                    '#F7DC6F', '#BB8FCE', '#85C1E9']

    cell_text_fifo = []
    for i in range(num_frames):
        row = []
        for state in fifo_states:
            if i < len(state):
                row.append(str(state[i]))
            else:
                row.append('-')
        cell_text_fifo.append(row)

    table = ax.table(cellText=cell_text_fifo,
                     rowLabels=[f'Frame {i}' for i in range(num_frames)],
                     colLabels=[str(r) for r in ref_string],
                     loc='center', cellLoc='center')
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1, 1.5)

    # Color fault columns
    for col_idx in range(len(ref_string)):
        for row_idx in range(num_frames):
            cell = table[row_idx + 1, col_idx]
            if fifo_faults[col_idx]:
                cell.set_facecolor('#FFE0E0')
            else:
                cell.set_facecolor('#E0FFE0')

    ax.set_title('FIFO Frame States (Red=Fault, Green=Hit)', fontweight='bold')
    ax.axis('off')

    plt.tight_layout(rect=[0, 0, 1, 0.93])
    filepath = os.path.join(OUTPUT_DIR, "page_replacement_comparison.png")
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  [OK] Generated: {filepath}")


# ======================================================================
# Generate summary text report
# ======================================================================
def generate_text_summary(sched_metrics, mem_metrics):
    """Generate a text summary of all metrics."""
    filepath = os.path.join(OUTPUT_DIR, "metrics_summary.txt")
    with open(filepath, 'w') as f:
        f.write("=" * 60 + "\n")
        f.write("  MINI OS KERNEL SIMULATOR - METRICS SUMMARY\n")
        f.write("=" * 60 + "\n\n")

        f.write("SCHEDULING METRICS COMPARISON\n")
        f.write("-" * 60 + "\n")
        f.write(f"{'Metric':<25} {'FCFS':>10} {'RR':>10} {'Priority':>10}\n")
        f.write("-" * 60 + "\n")

        if sched_metrics:
            for key in ['avg_waiting_time', 'avg_turnaround_time', 'avg_response_time',
                        'cpu_utilization', 'throughput', 'context_switches']:
                vals = []
                for s in SCHEDULERS:
                    if s in sched_metrics:
                        v = sched_metrics[s].get(key, 0)
                        vals.append(f"{v:.2f}" if isinstance(v, float) else str(v))
                    else:
                        vals.append("N/A")
                label = key.replace('_', ' ').title()
                f.write(f"{label:<25} {vals[0]:>10} {vals[1]:>10} {vals[2]:>10}\n")

        f.write("\n\nMEMORY METRICS COMPARISON\n")
        f.write("-" * 60 + "\n")
        if mem_metrics:
            for key in ['total_page_faults', 'total_page_replacements', 'page_fault_rate',
                        'frame_utilization']:
                vals = []
                for s in SCHEDULERS:
                    if s in mem_metrics:
                        v = mem_metrics[s].get(key, 0)
                        if key == 'page_fault_rate':
                            v *= 100
                        vals.append(f"{v:.2f}" if isinstance(v, float) else str(v))
                    else:
                        vals.append("N/A")
                label = key.replace('_', ' ').title()
                f.write(f"{label:<25} {vals[0]:>10} {vals[1]:>10} {vals[2]:>10}\n")

        f.write("\n" + "=" * 60 + "\n")

    print(f"  [OK] Generated: {filepath}")


# ======================================================================
# Main
# ======================================================================
def main():
    print("\n" + "=" * 50)
    print("  Mini OS Kernel Simulator - Report Generator")
    print("=" * 50 + "\n")

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("[1/6] Loading scheduling metrics...")
    sched_metrics = load_all_scheduling_metrics()
    print(f"  Loaded data for: {list(sched_metrics.keys())}")

    print("[2/6] Loading memory metrics...")
    mem_metrics = load_all_memory_metrics()
    print(f"  Loaded data for: {list(mem_metrics.keys())}")

    print("[3/6] Generating scheduling comparison charts...")
    plot_scheduling_comparison(sched_metrics)
    plot_per_process_metrics(sched_metrics)

    print("[4/6] Generating Gantt charts...")
    plot_gantt_charts(sched_metrics)

    print("[5/6] Generating memory/paging charts...")
    plot_memory_comparison(mem_metrics)
    plot_page_replacement_comparison()

    print("[6/6] Generating text summary...")
    generate_text_summary(sched_metrics, mem_metrics)

    print("\n" + "=" * 50)
    print("  All figures generated in: " + OUTPUT_DIR)
    print("=" * 50 + "\n")


if __name__ == "__main__":
    main()
