"""
Render the results of `onedse script.lua` (data.json) with matplotlib.

    python3 plot.py result/folder [--frames 4]

Writes one PNG per panel into the result folder. Animated panels are shown as a
row of evenly spaced frames; use the web frontend to see them move.
"""
import base64
import json
import os
import re
import sys

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402


def decode(b64):
    return np.frombuffer(base64.b64decode(b64), dtype="<f4")


def load(folder):
    with open(os.path.join(folder, "data.json")) as f:
        data = json.load(f)

    panels = {}
    for message in data.get("plots", []):
        title, _, label = message["title"].partition("/")
        panel = panels.setdefault(title, {"title": title, "kind": message["kind"], "frames": []})
        message["label"] = label
        panel["frames"].append(message)
    return list(panels.values())


def pick(frames, count):
    if len(frames) <= count:
        return frames
    return [frames[i] for i in np.linspace(0, len(frames) - 1, count).round().astype(int)]


def draw_wavefunction(ax, frame):
    re, im = decode(frame["re"]), decode(frame["im"])
    x = np.linspace(frame["lower"], frame["upper"], len(re), endpoint=False)
    ax.plot(x, re, lw=0.8, label="Re")
    ax.plot(x, im, lw=0.8, label="Im")
    ax.plot(x, re**2 + im**2, lw=1.5, color="k", label=r"$|\psi|^2$")
    ax.set_xlabel("p" if frame.get("space") == "p" else "x")
    ax.legend(fontsize=7)


def draw_wigner(ax, frame):
    if "data_i8" in frame:
        data = np.frombuffer(base64.b64decode(frame["data_i8"]), dtype=np.int8)*frame["scale"]
    else:
        data = decode(frame["data"])
    w = data.reshape(frame["nx"], frame["np"]).T
    limit = max(np.abs(w).max(), 1e-12)
    extent = [frame["lower_x"], frame["upper_x"], frame["lower_p"], frame["upper_p"]]
    # Diverging map with a neutral midpoint: negative values are the quantum signature
    im = ax.imshow(w, origin="lower", cmap="RdBu_r", vmin=-limit, vmax=limit, extent=extent, aspect="auto")
    ax.set_xlabel("x")
    ax.set_ylabel("p")
    plt.colorbar(im, ax=ax, fraction=0.046)


def draw_image(ax, frame):
    z = decode(frame["data"]).reshape(frame["ny"], frame["nx"])
    extent = [frame["x0"], frame["x1"], frame["y0"], frame["y1"]]
    im = ax.imshow(z, origin="lower", cmap="Blues", extent=extent, aspect="auto")
    ax.set_xlabel(frame.get("xlabel", "x"))
    ax.set_ylabel(frame.get("ylabel", "y"))
    plt.colorbar(im, ax=ax, fraction=0.046)


def draw_field(ax, frame):
    shape = (frame["ny"], frame["nx"])
    if "re_i8" in frame:
        re, im = (np.frombuffer(base64.b64decode(frame[k]), dtype=np.int8)*frame["scale"] for k in ("re_i8", "im_i8"))
    else:
        re, im = decode(frame["re"]), decode(frame["im"])
    psi = re.reshape(shape) + 1j*im.reshape(shape)
    density = np.abs(psi)**2
    # Hue: phase, brightness: density
    hsv = np.stack([(np.angle(psi)/(2*np.pi)) % 1, np.full(shape, 0.85), (density/max(density.max(), 1e-30))**0.45], axis=-1)
    extent = [frame["x0"], frame["x1"], frame["y0"], frame["y1"]]
    ax.imshow(matplotlib.colors.hsv_to_rgb(hsv), origin="lower", extent=extent, aspect="auto")
    ax.set_xlabel("x")
    ax.set_ylabel("y")


def draw_curves(ax, frame):
    for series in frame["series"]:
        y = decode(series["y"])
        ax.plot(np.linspace(frame["lower"], frame["upper"], len(y), endpoint=False), y, label=series["name"])
    ax.set_xlabel(frame.get("xlabel", "x"))
    ax.legend(fontsize=7)


def draw_paths(ax, frame):
    t = decode(frame["t"])
    for path in frame["paths"]:
        ax.plot(t, decode(path), lw=0.6, color="C0")
    ax.set_xlabel(frame.get("xlabel", "t"))
    ax.set_ylabel(frame.get("ylabel", "x"))


def draw_levels(ax, frame):
    V = decode(frame["V"])
    x = np.linspace(frame["lower"], frame["upper"], len(V), endpoint=False)
    energies = frame["energies"]
    spacing = (energies[-1] - energies[0])/max(len(energies) - 1, 1) or 1.
    states = [decode(s) for s in frame["states"]]
    amplitude = 0.38*spacing/max(np.abs(s).max() for s in states)
    ax.plot(x, V, color="gray")
    for E, psi in zip(energies, states):
        ax.axhline(E, color="lightgray", lw=0.5)
        ax.plot(np.linspace(frame["lower"], frame["upper"], len(psi), endpoint=False), E + amplitude*psi, color="C0")
    ax.set_ylim(min(V.min(), energies[0]) - 0.3*spacing, energies[-1] + 1.6*spacing)
    ax.set_xlabel("x")
    ax.set_ylabel("E")


DRAW = {
    "wavefunction": draw_wavefunction,
    "wigner": draw_wigner,
    "image": draw_image,
    "field": draw_field,
    "curves": draw_curves,
    "paths": draw_paths,
    "levels": draw_levels,
}


def plot_panel(panel, folder, count):
    filename = os.path.join(folder, re.sub(r"[^A-Za-z0-9_.-]+", "_", panel["title"]).strip("_") + ".png")

    if panel["kind"] == "scalar":
        fig, ax = plt.subplots(figsize=(6, 3.5))
        series = {}
        for frame in panel["frames"]:
            t, v = series.setdefault(frame["name"], ([], []))
            t.append(frame["t"])
            v.append(frame["value"])
        for name, (t, v) in series.items():
            ax.plot(t, v, label=name)
        ax.set_xlabel("t")
        ax.legend(fontsize=8)
        ax.set_title(panel["title"])
    elif panel["kind"] in DRAW:
        frames = pick(panel["frames"], count)
        fig, axes = plt.subplots(1, len(frames), figsize=(4.5*len(frames), 3.8), squeeze=False)
        for ax, frame in zip(axes[0], frames):
            DRAW[panel["kind"]](ax, frame)
            ax.set_title(frame["label"] or panel["title"], fontsize=9)
        fig.suptitle(panel["title"])
    else:
        print("Skipping \"%s\": unknown kind %s" % (panel["title"], panel["kind"]))
        return

    fig.tight_layout()
    fig.savefig(filename, dpi=130)
    plt.close(fig)
    print("Wrote %s" % filename)


def main():
    args = sys.argv[1:]
    count = 4
    if "--frames" in args:
        i = args.index("--frames")
        count = int(args[i + 1])
        del args[i:i + 2]
    if len(args) != 1 or args[0].startswith("--"):
        print(__doc__)
        sys.exit(1)

    try:
        panels = load(args[0])
    except (OSError, ValueError) as e:
        print("Could not read data: %s" % e)
        sys.exit(1)

    for panel in panels:
        plot_panel(panel, args[0], count)


if __name__ == "__main__":
    main()
