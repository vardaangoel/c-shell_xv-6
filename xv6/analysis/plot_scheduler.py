#!/usr/bin/env python3
"""Generate an MLFQ queue-transition plot from scheduler observations."""

import argparse
import os
import sys

import matplotlib.pyplot as plt
import pandas as pd

def generate_mlfq_plot(csv_file: str, output_image: str, watermark: str):
    try:
        df = pd.read_csv(csv_file)
    except Exception as error:
        print(f"Error reading {csv_file}: {error}")
        sys.exit(1)

    df.columns = df.columns.str.strip().str.lower()
    column_mapping = {}
    for column in df.columns:
        if "tick" in column:
            column_mapping[column] = "tick"
        elif "pid" in column or "process" in column:
            column_mapping[column] = "pid"
        elif "queue" in column:
            column_mapping[column] = "queue"
    df = df.rename(columns=column_mapping)

    required_columns = {"tick", "pid", "queue"}
    if not required_columns.issubset(df.columns):
        print(
            "Error: CSV must contain tick, pid, and queue columns. "
            f"Found: {list(df.columns)}"
        )
        sys.exit(1)

    df = df.sort_values(by="tick")
    if not df.empty:
        df["tick"] = df["tick"] - df["tick"].min()

    figure, axis = plt.subplots(figsize=(12, 6))
    unique_pids = df["pid"].unique()
    colors = plt.cm.tab10.colors

    for index, pid in enumerate(unique_pids):
        pid_data = df[df["pid"] == pid]
        color = colors[index % len(colors)]
        axis.scatter(
            pid_data["tick"],
            pid_data["queue"],
            label=f"PID {pid}",
            color=color,
            s=25,
            alpha=0.8,
            zorder=3,
        )
        axis.step(
            pid_data["tick"],
            pid_data["queue"],
            where="post",
            color=color,
            alpha=0.45,
            linewidth=1.2,
            zorder=2,
        )

    max_tick = int(df["tick"].max()) if not df.empty else 150
    for boost_tick in range(48, max_tick + 1, 48):
        axis.axvline(
            x=boost_tick,
            color="purple",
            linestyle="--",
            alpha=0.6,
            linewidth=1.5,
            label="Priority Boost (48 ticks)" if boost_tick == 48 else "",
            zorder=1,
        )

    axis.set_ylim(-0.5, 3.5)
    axis.set_yticks([0, 1, 2, 3])
    axis.set_yticklabels(["Q0 (Highest)", "Q1", "Q2", "Q3 (Lowest)"])
    axis.invert_yaxis()
    axis.set_xlabel("Time Elapsed (ticks)", fontsize=11)
    axis.set_ylabel("MLFQ Priority Level", fontsize=11)
    axis.set_title("MLFQ Process Queue Transitions & Priority Boosts", fontsize=13)
    axis.grid(True, linestyle=":", alpha=0.6, zorder=0)
    axis.legend(loc="lower right", frameon=True, facecolor="white", framealpha=0.9)
    plt.text(
        0.95,
        0.95,
        watermark,
        ha="right",
        va="top",
        transform=axis.transAxes,
        fontsize=10,
        color="gray",
        alpha=0.7,
    )

    figure.tight_layout()
    figure.savefig(output_image, dpi=300)
    print(f"Successfully saved MLFQ timeline plot to: {output_image}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate MLFQ scheduler timeline plot from CSV."
    )
    parser.add_argument("input_csv", help="Path to input CSV file")
    parser.add_argument("output_png", help="Path to output PNG image file")
    parser.add_argument(
        "--watermark",
        default=os.environ.get("IIIT_USERNAME", "vardaan.goel"),
        help="Part before @ in the IIIT email address",
    )
    args = parser.parse_args()
    generate_mlfq_plot(args.input_csv, args.output_png, args.watermark)

