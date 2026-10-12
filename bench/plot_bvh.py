#!/usr/bin/env python3
# =============================================================================
# File:     plot_bvh.py
# Author:   Elijah Samuel
# Course:   CS 499 Computer Science Capstone, SNHU
# Date:     October 2026
# Purpose:  Merges the two benchmark CSVs into docs/bench/bvh_results.csv and
#           draws docs/bench/bvh_chart.png.
#
# Design notes:
#   raytracer_bench writes times and raytracer_bench_stats writes tests per
#   ray (see bench/bench_accel.cpp for why they are separate programs). This
#   script joins them on (n, structure) and plots render time against scene
#   size on log-log axes. On those axes O(n) is a straight line of slope 1
#   and O(log n) is nearly flat, so the complexity difference is visible at
#   a glance. Needs only Python 3 and matplotlib.
#
#   python3 bench/plot_bvh.py --timing bvh_timing.csv --counts bvh_counts.csv \
#       --machine docs/bench/machine.txt --out-dir docs/bench
# =============================================================================
import argparse
import csv
import os
import sys

import matplotlib

matplotlib.use("Agg")  # Write a file. No window needed.
import matplotlib.pyplot as plt  # noqa: E402
import matplotlib.ticker  # noqa: E402

STRUCTURES = ["list", "bvh_sah", "bvh_median"]
LABELS = {
    "list": "HittableList (test every shape)",
    "bvh_sah": "BVH, binned SAH build (default)",
    "bvh_median": "BVH, median split build",
}
# Validated categorical slots 1-3 (blue, orange, aqua). Line style and marker
# also differ, so the lines are told apart without color.
COLORS = {"list": "#2a78d6", "bvh_sah": "#eb6834", "bvh_median": "#1baf7a"}
MARKERS = {"list": "o", "bvh_sah": "s", "bvh_median": "^"}
STYLES = {"list": "-", "bvh_sah": "-", "bvh_median": "--"}
TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
GRID = "#e4e3df"
SURFACE = "#fcfcfb"

COLUMNS = [
    "n", "structure", "build_ms", "render_ms", "total_ms",
    "box_tests_per_ray", "primitive_tests_per_ray", "hit_fraction",
    "nodes", "depth", "runs", "matches_list",
]


def read_rows(path):
    with open(path, newline="") as handle:
        return {(int(row["n"]), row["structure"]): row for row in csv.DictReader(handle)}


def merge(timing, counts):
    if set(timing) != set(counts):
        sys.exit("Timing and counts CSVs do not cover the same cases")
    merged = []
    for key in sorted(timing, key=lambda k: (k[0], STRUCTURES.index(k[1]))):
        t, c = timing[key], counts[key]
        total = float(t["build_ms"]) + float(t["render_ms"])
        merged.append({
            "n": key[0],
            "structure": key[1],
            "build_ms": t["build_ms"],
            "render_ms": t["render_ms"],
            "total_ms": f"{total:.2f}",
            "box_tests_per_ray": c["box_tests_per_ray"],
            "primitive_tests_per_ray": c["primitive_tests_per_ray"],
            "hit_fraction": t["hit_fraction"],
            "nodes": t["nodes"],
            "depth": t["depth"],
            "runs": t["runs"],
            "matches_list": t["matches_list"],
        })
    return merged


def plot(rows, machine_lines, path):
    fig, ax = plt.subplots(figsize=(9, 6.2), dpi=150)
    fig.patch.set_facecolor(SURFACE)
    ax.set_facecolor(SURFACE)

    for structure in STRUCTURES:
        points = [(r["n"], float(r["render_ms"])) for r in rows if r["structure"] == structure]
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        ax.plot(xs, ys, STYLES[structure], color=COLORS[structure], linewidth=2,
                marker=MARKERS[structure], markersize=7, label=LABELS[structure])
        # Direct label at the right end of each line, in text ink.
        offset = {"list": 0, "bvh_sah": -9, "bvh_median": 7}[structure]
        short = {"list": "list", "bvh_sah": "SAH", "bvh_median": "median"}[structure]
        ax.annotate(f"{short} {ys[-1]:,.0f} ms", xy=(xs[-1], ys[-1]), xytext=(8, offset),
                    textcoords="offset points", va="center", fontsize=9, color=TEXT_PRIMARY)

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Spheres in the scene (N, log scale)", color=TEXT_PRIMARY)
    ax.set_ylabel("Render time, ms (median of 5 runs, log scale)", color=TEXT_PRIMARY)
    ax.set_xlim(0.7, 40000)
    plain = matplotlib.ticker.FuncFormatter(lambda value, _pos: f"{value:,.0f}")
    ax.xaxis.set_major_formatter(plain)
    ax.yaxis.set_major_formatter(plain)
    ax.grid(True, which="major", color=GRID, linewidth=0.8)
    ax.tick_params(colors=TEXT_SECONDARY)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)
    legend = ax.legend(loc="upper left", frameon=False, fontsize=9)
    for text in legend.get_texts():
        text.set_color(TEXT_PRIMARY)

    fig.suptitle("Closest-hit search: list vs. bounding volume hierarchy",
                 x=0.06, ha="left", fontsize=13, color=TEXT_PRIMARY, fontweight="bold")
    ax.set_title("400x300 primary rays, 1 sample, 1 thread. List grows linearly with N; BVH stays nearly flat.\n"
                 "Below about 10 ms the time is mostly fixed per-frame cost, and runs vary by 1 to 3 ms.",
                 loc="left", fontsize=9, color=TEXT_SECONDARY)
    fig.text(0.06, 0.015, "\n".join(machine_lines), ha="left", va="bottom", fontsize=7.5,
             color=TEXT_SECONDARY)
    fig.subplots_adjust(left=0.1, right=0.88, top=0.86, bottom=0.2)
    fig.savefig(path, facecolor=SURFACE)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timing", required=True)
    parser.add_argument("--counts", required=True)
    parser.add_argument("--machine", required=True, help="Text file, one line per fact")
    parser.add_argument("--out-dir", required=True)
    args = parser.parse_args()

    rows = merge(read_rows(args.timing), read_rows(args.counts))
    with open(args.machine) as handle:
        machine_lines = [line.strip() for line in handle if line.strip()]

    os.makedirs(args.out_dir, exist_ok=True)
    csv_path = os.path.join(args.out_dir, "bvh_results.csv")
    with open(csv_path, "w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    png_path = os.path.join(args.out_dir, "bvh_chart.png")
    plot(rows, machine_lines, png_path)
    print(f"Wrote {csv_path} and {png_path}")


if __name__ == "__main__":
    main()
