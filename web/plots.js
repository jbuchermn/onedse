// Canvas renderers for all plot kinds. No dependencies.
//
// A panel is { title, kind, frames: [{ label, ...payload }], state: {} }. Every renderer
// draws one frame and returns { hover(px, py) -> text | null, legend: [...] }.
// Scales are kept fixed across the frames of a panel (panel.state), so that an
// animation shows the physics and not a rescaling axis.

const MARGIN = { left: 52, right: 12, top: 10, bottom: 30 };

/*
 * Theme
 */
export function readTheme() {
    const style = getComputedStyle(document.documentElement);
    const v = name => style.getPropertyValue(name).trim();
    return {
        surface: v('--surface-1'),
        text: v('--text-primary'),
        textSecondary: v('--text-secondary'),
        muted: v('--text-muted'),
        grid: v('--grid'),
        potential: v('--potential'),
        series: [1, 2, 3, 4, 5, 6].map(i => v('--series-' + i)),
        divNeg: hexToRgb(v('--div-neg')),
        divMid: hexToRgb(v('--div-mid')),
        divPos: hexToRgb(v('--div-pos')),
        seqLow: hexToRgb(v('--seq-low')),
        seqHigh: hexToRgb(v('--seq-high')),
    };
}

function hexToRgb(hex) {
    const n = parseInt(hex.replace('#', ''), 16);
    return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
}

/// Phase as hue: 0 -> red, pi/2 -> yellow-green, pi -> cyan, -pi/2 -> violet
function phaseRgb(phase, value) {
    const h = ((phase / (2 * Math.PI)) % 1 + 1) % 1 * 6;
    const k = n => {
        const t = (n + h) % 6;
        return value * (1 - 0.82 * Math.max(0, Math.min(t, 4 - t, 1)));
    };
    return [255 * k(5), 255 * k(3), 255 * k(1)];
}

export function phaseGradientCss() {
    const stops = [];
    for (let i = 0; i <= 12; i++) {
        const [r, g, b] = phaseRgb(-Math.PI + i / 12 * 2 * Math.PI, 1);
        stops.push(`rgb(${r | 0},${g | 0},${b | 0})`);
    }
    return `linear-gradient(90deg, ${stops.join(',')})`;
}

/*
 * Axes
 */
function niceTicks(lo, hi, count) {
    if (!(hi > lo)) return [lo];
    const raw = (hi - lo) / Math.max(1, count);
    const mag = Math.pow(10, Math.floor(Math.log10(raw)));
    const norm = raw / mag;
    const step = (norm < 1.5 ? 1 : norm < 3.5 ? 2 : norm < 7.5 ? 5 : 10) * mag;
    const ticks = [];
    for (let t = Math.ceil(lo / step - 1e-9) * step; t <= hi + step * 1e-9; t += step) ticks.push(Math.abs(t) < step * 1e-9 ? 0 : t);
    return ticks;
}

export function fmt(v) {
    if (v === 0) return '0';
    const a = Math.abs(v);
    if (a >= 1e4 || a < 1e-3) return v.toExponential(1);
    return String(parseFloat(v.toPrecision(3)));
}

class Frame {
    constructor(ctx, w, h, x0, x1, y0, y1, margin = MARGIN) {
        Object.assign(this, { ctx, w, h, x0, x1, y0, y1 });
        this.left = margin.left;
        this.top = margin.top;
        this.pw = Math.max(10, w - margin.left - margin.right);
        this.ph = Math.max(10, h - margin.top - margin.bottom);
    }
    px(x) { return this.left + (x - this.x0) / (this.x1 - this.x0) * this.pw; }
    py(y) { return this.top + (1 - (y - this.y0) / (this.y1 - this.y0)) * this.ph; }
    x(px) { return this.x0 + (px - this.left) / this.pw * (this.x1 - this.x0); }
    y(py) { return this.y0 + (1 - (py - this.top) / this.ph) * (this.y1 - this.y0); }
    inside(px, py) { return px >= this.left && px <= this.left + this.pw && py >= this.top && py <= this.top + this.ph; }

    axes(theme, xlabel, ylabel, { grid = true } = {}) {
        const ctx = this.ctx;
        ctx.save();
        ctx.font = '11px system-ui, sans-serif';
        ctx.lineWidth = 1;

        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        for (const t of niceTicks(this.x0, this.x1, this.pw / 70)) {
            const px = Math.round(this.px(t)) + 0.5;
            if (grid) {
                ctx.strokeStyle = theme.grid;
                ctx.beginPath(); ctx.moveTo(px, this.top); ctx.lineTo(px, this.top + this.ph); ctx.stroke();
            }
            ctx.fillStyle = theme.muted;
            ctx.fillText(fmt(t), px, this.top + this.ph + 5);
        }
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        for (const t of niceTicks(this.y0, this.y1, this.ph / 45)) {
            const py = Math.round(this.py(t)) + 0.5;
            if (grid) {
                ctx.strokeStyle = theme.grid;
                ctx.beginPath(); ctx.moveTo(this.left, py); ctx.lineTo(this.left + this.pw, py); ctx.stroke();
            }
            ctx.fillStyle = theme.muted;
            ctx.fillText(fmt(t), this.left - 6, py);
        }

        ctx.fillStyle = theme.textSecondary;
        if (xlabel) {
            ctx.textAlign = 'right';
            ctx.textBaseline = 'bottom';
            ctx.fillText(xlabel, this.left + this.pw, this.h - 1);
        }
        if (ylabel) {
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            ctx.fillText(ylabel, 2, 0);
        }
        ctx.restore();
    }

    clip() {
        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.rect(this.left, this.top, this.pw, this.ph);
        this.ctx.clip();
    }
    unclip() { this.ctx.restore(); }

    line(n, xOf, yOf, color, width = 2) {
        const ctx = this.ctx;
        ctx.beginPath();
        for (let i = 0; i < n; i++) {
            const px = this.px(xOf(i)), py = this.py(yOf(i));
            if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
        }
        ctx.strokeStyle = color;
        ctx.lineWidth = width;
        ctx.lineJoin = 'round';
        ctx.stroke();
    }
}

function maxOf(arr) {
    let m = -Infinity;
    for (let i = 0; i < arr.length; i++) if (arr[i] > m) m = arr[i];
    return m;
}
function minOf(arr) {
    let m = Infinity;
    for (let i = 0; i < arr.length; i++) if (arr[i] < m) m = arr[i];
    return m;
}
function maxAbs(arr) {
    let m = 0;
    for (let i = 0; i < arr.length; i++) { const a = Math.abs(arr[i]); if (a > m) m = a; }
    return m;
}

/// Running maximum over all frames of a panel: scales only ever grow
function grow(state, key, value) {
    if (!(state[key] >= value)) state[key] = value;
    return state[key];
}

/*
 * Potential overlay: drawn on its own (annotated) scale as context behind the data.
 * The scale is chosen from the region where the state actually lives, otherwise a
 * confining potential's values at the edge of the grid would flatten everything.
 */
function potentialScale(state, V, density) {
    const dmax = maxOf(density);
    let lo = Infinity, hi = -Infinity;
    for (let i = 0; i < V.length; i++) {
        if (density[i] > 2e-3 * dmax) {
            if (V[i] < lo) lo = V[i];
            if (V[i] > hi) hi = V[i];
        }
    }
    const vmin = minOf(V);
    if (!(hi > vmin + 1e-9)) hi = maxOf(V);
    state.vmin = Math.min(state.vmin ?? vmin, vmin);
    grow(state, 'vmax', vmin + 1.25 * (hi - vmin));
    return [state.vmin, state.vmax];
}

function drawPotential(f, theme, V, lower, upper, vmin, vmax) {
    if (!(vmax > vmin)) return;
    const ctx = f.ctx;
    const n = V.length;
    const yOf = i => f.y0 + 0.92 * (f.y1 - f.y0) * Math.min(1.2, (V[i] - vmin) / (vmax - vmin));
    ctx.save();
    ctx.globalAlpha = 0.9;
    f.line(n, i => lower + i * (upper - lower) / n, yOf, theme.potential, 1.25);
    ctx.globalAlpha = 0.10;
    ctx.lineTo(f.px(upper), f.py(f.y0));
    ctx.lineTo(f.px(lower), f.py(f.y0));
    ctx.fillStyle = theme.potential;
    ctx.fill();
    ctx.restore();
}

/*
 * Wave function: |psi|^2 as area, coloured by the phase of psi
 */
function renderWavefunction(ctx, w, h, panel, frame, theme) {
    const { re, im, lower, upper, V } = frame;
    const n = re.length;
    const state = panel.state;

    if (!frame.density) {
        frame.density = new Float32Array(n);
        for (let i = 0; i < n; i++) frame.density[i] = re[i] * re[i] + im[i] * im[i];
        frame.dmax = maxOf(frame.density);
    }
    const density = frame.density;
    const ymax = grow(state, 'ymax', frame.dmax * 1.08) || 1;
    const xlabel = frame.space === 'p' ? 'p' : 'x';

    const f = new Frame(ctx, w, h, lower, upper, 0, ymax);
    f.axes(theme, xlabel, frame.space === 'p' ? '|φ(p)|²' : '|ψ(x)|²');
    f.clip();

    let vscale = null;
    if (V) {
        vscale = potentialScale(state, V, density);
        drawPotential(f, theme, V, lower, upper, vscale[0], vscale[1]);
    }

    // One vertical strip per sample, coloured by phase
    const dx = (upper - lower) / n;
    const base = f.py(0);
    for (let i = 0; i < n; i++) {
        if (density[i] < 1e-5 * ymax) continue;
        const x0 = f.px(lower + (i - 0.5) * dx), x1 = f.px(lower + (i + 0.5) * dx);
        const [r, g, b] = phaseRgb(Math.atan2(im[i], re[i]), 1);
        ctx.fillStyle = `rgb(${r | 0},${g | 0},${b | 0})`;
        const top = f.py(density[i]);
        ctx.fillRect(x0, top, Math.max(1, x1 - x0 + 0.6), base - top);
    }
    f.line(n, i => lower + i * dx, i => density[i], theme.text, 1.25);
    f.unclip();

    const legend = [{ gradient: phaseGradientCss(), label: 'phase −π … π' }];
    if (vscale && vscale[1] > vscale[0]) legend.push({ color: theme.potential, label: `V (own scale, ${fmt(vscale[0])} … ${fmt(vscale[1])})` });

    return {
        legend,
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const i = Math.max(0, Math.min(n - 1, Math.round((f.x(px) - lower) / dx)));
            let text = `${xlabel} = ${fmt(lower + i * dx)}\n|ψ|² = ${fmt(density[i])}\nphase = ${fmt(Math.atan2(im[i], re[i]) / Math.PI)} π`;
            if (V) text += `\nV = ${fmt(V[i])}`;
            return text;
        },
    };
}

/*
 * Heatmaps
 */
const scratch = document.createElement('canvas');

function drawImage(f, nx, ny, colorOf, crop = null) {
    scratch.width = nx;
    scratch.height = ny;
    const sctx = scratch.getContext('2d');
    const img = sctx.createImageData(nx, ny);
    const data = img.data;
    for (let iy = 0; iy < ny; iy++) {
        // Row 0 of the image is the top, i.e. the largest y
        const row = (ny - 1 - iy) * nx * 4;
        for (let ix = 0; ix < nx; ix++) {
            const c = colorOf(ix, iy);
            const o = row + ix * 4;
            data[o] = c[0]; data[o + 1] = c[1]; data[o + 2] = c[2]; data[o + 3] = 255;
        }
    }
    sctx.putImageData(img, 0, 0);
    f.ctx.imageSmoothingEnabled = true;
    if (crop) {
        // crop = [ix0, ix1, iy0, iy1] in (fractional) cells, iy counted from the bottom
        f.ctx.drawImage(scratch, crop[0], ny - crop[3], crop[1] - crop[0], crop[3] - crop[2], f.left, f.top, f.pw, f.ph);
    } else {
        f.ctx.drawImage(scratch, f.left, f.top, f.pw, f.ph);
    }
}

function lerpRgb(a, b, t) {
    return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
}
function rgbCss(c) { return `rgb(${c[0] | 0},${c[1] | 0},${c[2] | 0})`; }

// Panels showing the same region of phase space (e.g. quantum next to classical) zoom
// together, otherwise they could not be compared at a glance.
const sharedZoom = new Map();
export function resetSharedState() { sharedZoom.clear(); }

/// Wigner function W(x, p): diverging scale, neutral at zero, symmetric limits
function renderWigner(ctx, w, h, panel, frame, theme) {
    const { data, nx, np, lower_x, upper_x, lower_p, upper_p } = frame;
    const k = frame.scale ?? 1;          // 8 bit frames: value = data * scale
    if (frame.amax === undefined) frame.amax = maxAbs(data) * k;
    const amax = grow(panel.state, 'amax', frame.amax) || 1;
    const cellX = (upper_x - lower_x) / nx, cellP = (upper_p - lower_p) / np;

    // Zoom to the part of phase space that is occupied at any time (the view only ever grows)
    if (!frame.extent) {
        let xa = nx, xb = -1, pext = 0;
        const threshold = 0.05 * frame.amax / k;
        for (let ix = 0; ix < nx; ix++) {
            for (let ip = 0; ip < np; ip++) {
                if (Math.abs(data[ix * np + ip]) > threshold) {
                    if (ix < xa) xa = ix;
                    if (ix > xb) xb = ix;
                    pext = Math.max(pext, Math.abs(lower_p + ip * cellP));
                }
            }
        }
        frame.extent = xb < 0 ? null : [lower_x + xa * cellX, lower_x + (xb + 1) * cellX, pext + cellP];
    }
    const key = `${lower_x}|${upper_x}`;
    let zoom = sharedZoom.get(key);
    if (!zoom) sharedZoom.set(key, zoom = { xa: Infinity, xb: -Infinity, pext: 0 });
    if (frame.extent) {
        zoom.xa = Math.min(zoom.xa, frame.extent[0]);
        zoom.xb = Math.max(zoom.xb, frame.extent[1]);
        zoom.pext = Math.max(zoom.pext, frame.extent[2]);
    }
    let xa = lower_x, xb = upper_x, pa = lower_p, pb = upper_p;
    if (zoom.xb > zoom.xa) {
        const padX = Math.max(4 * cellX, 0.25 * (zoom.xb - zoom.xa));
        xa = Math.max(lower_x, zoom.xa - padX);
        xb = Math.min(upper_x, zoom.xb + padX);
        const half = Math.max(8 * cellP, 1.3 * zoom.pext);
        pa = Math.max(lower_p, -half);
        pb = Math.min(upper_p, half);
    }

    const f = new Frame(ctx, w, h, xa, xb, pa, pb);
    drawImage(f, nx, np, (ix, ip) => {
        const v = data[ix * np + ip] * k / amax;
        // A mild gamma keeps weak interference fringes visible next to strong peaks
        const t = Math.pow(Math.min(1, Math.abs(v)), 0.6);
        return lerpRgb(theme.divMid, v < 0 ? theme.divNeg : theme.divPos, t);
    }, [(xa - lower_x) / cellX, (xb - lower_x) / cellX, (pa - lower_p) / cellP, (pb - lower_p) / cellP]);
    f.axes(theme, 'x', 'p', { grid: false });

    const legend = [frame.classical ? {
        gradient: `linear-gradient(90deg, ${rgbCss(theme.divMid)}, ${rgbCss(theme.divPos)})`,
        label: `density of particles: 0 … ${fmt(amax)}`,
    } : {
        gradient: `linear-gradient(90deg, ${rgbCss(theme.divNeg)}, ${rgbCss(theme.divMid)}, ${rgbCss(theme.divPos)})`,
        label: `W(x, p): −${fmt(amax)} … 0 … +${fmt(amax)}`,
    }];
    if (!frame.classical) {
        if (frame.wmin === undefined) frame.wmin = minOf(data) * k;
        legend.push({ label: `most negative value: ${fmt(Math.min(0, frame.wmin))}` });
    }

    return {
        legend,
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const ix = Math.max(0, Math.min(nx - 1, Math.floor((f.x(px) - lower_x) / (upper_x - lower_x) * nx)));
            const ip = Math.max(0, Math.min(np - 1, Math.floor((f.y(py) - lower_p) / (upper_p - lower_p) * np)));
            return `x = ${fmt(f.x(px))}\np = ${fmt(f.y(py))}\nW = ${fmt(data[ix * np + ip] * k)}`;
        },
    };
}

/// Non-negative scalar field: sequential single-hue scale
function renderImage(ctx, w, h, panel, frame, theme) {
    const { data, nx, ny, x0, x1, y0, y1 } = frame;
    if (frame.vmax === undefined) frame.vmax = maxOf(data);
    const vmax = grow(panel.state, 'vmax', frame.vmax) || 1;

    const f = new Frame(ctx, w, h, x0, x1, y0, y1);
    drawImage(f, nx, ny, (ix, iy) => {
        const t = Math.pow(Math.max(0, Math.min(1, data[iy * nx + ix] / vmax)), 0.7);
        return lerpRgb(theme.seqLow, theme.seqHigh, t);
    });
    f.axes(theme, frame.xlabel, frame.ylabel, { grid: false });

    return {
        legend: [{ gradient: `linear-gradient(90deg, ${rgbCss(theme.seqLow)}, ${rgbCss(theme.seqHigh)})`, label: `0 … ${fmt(vmax)}` }],
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const ix = Math.max(0, Math.min(nx - 1, Math.floor((f.x(px) - x0) / (x1 - x0) * nx)));
            const iy = Math.max(0, Math.min(ny - 1, Math.floor((f.y(py) - y0) / (y1 - y0) * ny)));
            return `${frame.xlabel || 'x'} = ${fmt(f.x(px))}\n${frame.ylabel || 'y'} = ${fmt(f.y(py))}\nvalue = ${fmt(data[iy * nx + ix])}`;
        },
    };
}

/// Complex field psi(x, y): brightness is |psi|^2, hue is the phase
function renderField(ctx, w, h, panel, frame, theme) {
    const { re, im, nx, ny, x0, x1, y0, y1, V } = frame;
    const k2 = (frame.scale ?? 1) ** 2;  // 8 bit frames: psi = (re + i im) * scale
    if (frame.dmax === undefined) {
        let m = 0;
        for (let i = 0; i < re.length; i++) m = Math.max(m, re[i] * re[i] + im[i] * im[i]);
        frame.dmax = m * k2;
    }
    // Brightness is relative to the current maximum (a spreading wave would fade to black
    // otherwise), but never amplified by more than a factor of 50
    const dmax = Math.max(frame.dmax, 0.02 * grow(panel.state, 'dmax', frame.dmax)) || 1;
    const vmax = V ? maxOf(V) : 0;
    const vmin = V ? minOf(V) : 0;

    const f = new Frame(ctx, w, h, x0, x1, y0, y1);
    drawImage(f, nx, ny, (ix, iy) => {
        const i = iy * nx + ix;
        const d = (re[i] * re[i] + im[i] * im[i]) * k2 / dmax;
        const c = phaseRgb(Math.atan2(im[i], re[i]), Math.pow(Math.min(1, d), 0.45));
        if (V && vmax > vmin && V[i] > vmin + 0.5 * (vmax - vmin)) {
            // Walls
            return [c[0] * 0.5 + 70, c[1] * 0.5 + 70, c[2] * 0.5 + 70];
        }
        return c;
    });
    f.axes(theme, 'x', 'y', { grid: false });

    const legend = [{ gradient: phaseGradientCss(), label: 'phase −π … π' }, { label: 'brightness: |ψ|², relative to its current maximum' }];
    if (V) legend.push({ color: 'rgb(110,110,110)', label: 'potential wall' });

    return {
        legend,
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const ix = Math.max(0, Math.min(nx - 1, Math.floor((f.x(px) - x0) / (x1 - x0) * nx)));
            const iy = Math.max(0, Math.min(ny - 1, Math.floor((f.y(py) - y0) / (y1 - y0) * ny)));
            const i = iy * nx + ix;
            return `x = ${fmt(f.x(px))}\ny = ${fmt(f.y(py))}\n|ψ|² = ${fmt((re[i] * re[i] + im[i] * im[i]) * k2)}\nphase = ${fmt(Math.atan2(im[i], re[i]) / Math.PI)} π`;
        },
    };
}

/*
 * Several real curves over x (spinor components, marginals)
 */
function renderCurves(ctx, w, h, panel, frame, theme) {
    const { series, lower, upper, V } = frame;
    const state = panel.state;
    if (frame.ymax === undefined) frame.ymax = Math.max(...series.map(s => maxOf(s.y)));
    const ymax = grow(state, 'ymax', frame.ymax * 1.08) || 1;

    const f = new Frame(ctx, w, h, lower, upper, 0, ymax);
    f.axes(theme, frame.xlabel || 'x', 'density');
    f.clip();
    let vscale = null;
    if (V) {
        vscale = potentialScale(state, V, series[0].y);
        drawPotential(f, theme, V, lower, upper, vscale[0], vscale[1]);
    }
    series.forEach((s, k) => {
        const n = s.y.length;
        f.line(n, i => lower + i * (upper - lower) / n, i => s.y[i], theme.series[k % theme.series.length], 2);
    });
    f.unclip();

    const legend = series.map((s, k) => ({ color: theme.series[k % theme.series.length], label: s.name }));
    if (vscale && vscale[1] > vscale[0]) legend.push({ color: theme.potential, label: `V (own scale, ${fmt(vscale[0])} … ${fmt(vscale[1])})` });

    return {
        legend,
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const x = f.x(px);
            let text = `x = ${fmt(x)}`;
            for (const s of series) {
                const i = Math.max(0, Math.min(s.y.length - 1, Math.round((x - lower) / (upper - lower) * s.y.length)));
                text += `\n${s.name} = ${fmt(s.y[i])}`;
            }
            return text;
        },
    };
}

/*
 * Time series of scalar observables. The cursor marks the frame currently shown.
 */
function renderScalar(ctx, w, h, panel, frame, theme, fraction) {
    const names = Object.keys(panel.series);
    let t0 = Infinity, t1 = -Infinity, y0 = Infinity, y1 = -Infinity;
    for (const name of names) {
        const s = panel.series[name];
        for (let i = 0; i < s.t.length; i++) {
            t0 = Math.min(t0, s.t[i]); t1 = Math.max(t1, s.t[i]);
            y0 = Math.min(y0, s.v[i]); y1 = Math.max(y1, s.v[i]);
        }
    }
    if (!(t1 > t0)) t1 = t0 + 1;
    // Include zero if it is close: bars of meaning rather than a zoomed-in wiggle
    if (y0 > 0 && y0 < 0.6 * y1) y0 = 0;
    if (y1 < 0 && y1 > 0.6 * y0) y1 = 0;
    // A conserved quantity should look conserved: never zoom in further than 2% of its value
    const minSpan = 0.02 * Math.max(Math.abs(y0), Math.abs(y1)) || 1;
    if (y1 - y0 < minSpan) {
        const mid = (y0 + y1) / 2;
        y0 = mid - minSpan / 2; y1 = mid + minSpan / 2;
    }
    const pad = (y1 - y0) * 0.06;
    y0 -= (y0 === 0 ? 0 : pad); y1 += pad;

    const f = new Frame(ctx, w, h, t0, t1, y0, y1);
    f.axes(theme, 't', '');
    f.clip();
    names.forEach((name, k) => {
        const s = panel.series[name];
        f.line(s.t.length, i => s.t[i], i => s.v[i], theme.series[k % theme.series.length], 2);
    });

    // Cursor
    const first = panel.series[names[0]];
    const ci = Math.round(fraction * (first.t.length - 1));
    const tc = first.t[Math.max(0, Math.min(first.t.length - 1, ci))];
    ctx.strokeStyle = theme.muted;
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath(); ctx.moveTo(f.px(tc), f.top); ctx.lineTo(f.px(tc), f.top + f.ph); ctx.stroke();
    ctx.setLineDash([]);
    names.forEach((name, k) => {
        const s = panel.series[name];
        const i = Math.max(0, Math.min(s.t.length - 1, Math.round(fraction * (s.t.length - 1))));
        ctx.beginPath();
        ctx.arc(f.px(s.t[i]), f.py(s.v[i]), 4, 0, 2 * Math.PI);
        ctx.fillStyle = theme.series[k % theme.series.length];
        ctx.fill();
        ctx.strokeStyle = theme.surface;
        ctx.lineWidth = 2;
        ctx.stroke();
    });
    f.unclip();

    return {
        legend: names.map((name, k) => {
            const s = panel.series[name];
            const i = Math.max(0, Math.min(s.t.length - 1, Math.round(fraction * (s.t.length - 1))));
            return { color: theme.series[k % theme.series.length], label: `${name}: ${fmt(s.v[i])}` };
        }),
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const t = f.x(px);
            let text = null;
            for (const name of names) {
                const s = panel.series[name];
                let best = 0;
                for (let i = 1; i < s.t.length; i++) if (Math.abs(s.t[i] - t) < Math.abs(s.t[best] - t)) best = i;
                if (text === null) text = `t = ${fmt(s.t[best])}`;
                text += `\n${name} = ${fmt(s.v[best])}`;
            }
            return text;
        },
    };
}

/*
 * Trajectories x(t)
 */
function renderPaths(ctx, w, h, panel, frame, theme) {
    const { t, paths } = frame;
    if (frame.lo === undefined) {
        frame.lo = Math.min(...paths.map(minOf));
        frame.hi = Math.max(...paths.map(maxOf));
    }
    const pad = (frame.hi - frame.lo) * 0.04 || 1;
    const f = new Frame(ctx, w, h, t[0], t[t.length - 1], frame.lo - pad, frame.hi + pad);
    f.axes(theme, frame.xlabel, frame.ylabel);
    f.clip();
    ctx.globalAlpha = 0.75;
    for (const path of paths) f.line(t.length, i => t[i], i => path[i], theme.series[0], 1.25);
    ctx.globalAlpha = 1;
    f.unclip();

    return {
        legend: [{ color: theme.series[0], label: `${paths.length} trajectories, started at the quantiles of |ψ|²` }],
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            return `t = ${fmt(f.x(px))}\nx = ${fmt(f.y(py))}`;
        },
    };
}

/*
 * Energy levels: eigenfunctions drawn at the height of their energy
 */
function renderLevels(ctx, w, h, panel, frame, theme) {
    const { V, energies, states, lower, upper } = frame;
    const n = V.length;
    const dx = (upper - lower) / n;
    const vmin = minOf(V);
    const e0 = energies[0], e1 = energies[energies.length - 1];
    const spacing = energies.length > 1 ? (e1 - e0) / (energies.length - 1) : Math.abs(e0) || 1;
    const ylo = Math.min(vmin, e0) - 0.3 * spacing;
    const yhi = e1 + 1.6 * spacing;

    // Zoom to where the eigenfunctions live
    let xa = upper, xb = lower;
    for (const psi of states) {
        const m = maxAbs(psi), step = (upper - lower) / psi.length;
        for (let i = 0; i < psi.length; i++) {
            if (Math.abs(psi[i]) > 0.02 * m) { xa = Math.min(xa, lower + i * step); xb = Math.max(xb, lower + i * step); }
        }
    }
    if (!(xb > xa)) { xa = lower; xb = upper; }
    const padx = (xb - xa) * 0.08;
    xa = Math.max(lower, xa - padx); xb = Math.min(upper, xb + padx);

    const f = new Frame(ctx, w, h, xa, xb, ylo, yhi);
    f.axes(theme, 'x', 'E');
    f.clip();

    // Potential on the same (energy) axis
    f.line(n, i => lower + i * dx, i => Math.min(V[i], yhi + spacing), theme.potential, 1.5);

    const amp = 0.38 * spacing / Math.max(...states.map(maxAbs));
    energies.forEach((E, k) => {
        ctx.strokeStyle = theme.grid;
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(f.left, f.py(E)); ctx.lineTo(f.left + f.pw, f.py(E)); ctx.stroke();
        const psi = states[k];
        f.line(psi.length, i => lower + i * (upper - lower) / psi.length, i => E + amp * psi[i], theme.series[0], 1.75);
    });
    f.unclip();

    return {
        legend: [
            { color: theme.series[0], label: 'eigenfunctions ψₙ(x), offset by Eₙ' },
            { color: theme.potential, label: 'V(x)' },
        ],
        hover(px, py) {
            if (!f.inside(px, py)) return null;
            const E = f.y(py);
            let best = 0;
            energies.forEach((e, k) => { if (Math.abs(e - E) < Math.abs(energies[best] - E)) best = k; });
            return `n = ${best}\nE = ${fmt(parseFloat(energies[best].toPrecision(6)))}`;
        },
    };
}

const RENDERERS = {
    wavefunction: renderWavefunction,
    wigner: renderWigner,
    image: renderImage,
    field: renderField,
    curves: renderCurves,
    scalar: renderScalar,
    paths: renderPaths,
    levels: renderLevels,
};

export const SQUARE_KINDS = new Set(['wigner', 'image', 'field']);

/// Layout order: phase space and fields first, then curves over x, then time series
export function kindRank(kind) {
    return SQUARE_KINDS.has(kind) ? 0 : kind === 'scalar' ? 2 : 1;
}

export function render(canvas, panel, frameIndex, fraction, theme) {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
        canvas.width = Math.round(w * dpr);
        canvas.height = Math.round(h * dpr);
    }
    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);

    const renderer = RENDERERS[panel.kind];
    if (!renderer) return { legend: [{ label: `Unknown plot kind "${panel.kind}"` }], hover: () => null };
    return renderer(ctx, w, h, panel, panel.frames[frameIndex], theme, fraction);
}
