import { render, readTheme, resetSharedState, kindRank, SQUARE_KINDS } from './plots.js';

const $ = id => document.getElementById(id);
const el = {
    examples: $('examples'), title: $('title'), description: $('description'),
    code: $('code'), run: $('run'), stop: $('stop'), reset: $('reset'), status: $('status'),
    console: $('console'), toggleEditor: $('toggle-editor'),
    transport: $('transport'), play: $('play'), prev: $('prev'), next: $('next'), slider: $('slider'),
    frameLabel: $('frame-label'), speed: $('speed'), loop: $('loop'),
    panels: $('panels'), tooltip: $('tooltip'), theme: $('theme'),
};

const state = {
    examples: [],
    current: null,          // example shown in the editor
    panels: new Map(),      // title -> panel
    position: 0,            // index on the global timeline
    follow: true,           // show the newest frame while the script is running
    playing: false,
    running: false,
    abort: null,
    dirty: false,
    theme: readTheme(),
};

const storage = {
    get(key) { try { return localStorage.getItem('onedse.' + key); } catch { return null; } },
    set(key, value) { try { localStorage.setItem('onedse.' + key, value); } catch { /* private mode */ } },
    remove(key) { try { localStorage.removeItem('onedse.' + key); } catch { /* private mode */ } },
};

/*
 * Examples
 */
const GROUPS = [
    [/^0[1-57]_|^13_/, 'One dimension'],
    [/^06_|^3\d_/, 'Classical versus quantum'],
    [/^08_|^12_/, 'Two dimensions'],
    [/^09_|^1[456]_|^22_/, 'Measurement and decoherence'],
    [/^1[01]_/, 'Relativistic'],
    [/^2\d_/, 'Open systems, spectral basis'],
];

async function loadExamples() {
    try {
        state.examples = await (await fetch('/api/examples')).json();
    } catch (e) {
        el.title.textContent = 'Could not reach the server';
        el.description.textContent = String(e);
        return;
    }

    el.examples.textContent = '';
    const groups = new Map();
    for (const example of state.examples) {
        const match = GROUPS.find(([pattern]) => pattern.test(example.name));
        const name = match ? match[1] : 'Other';
        if (!groups.has(name)) groups.set(name, []);
        groups.get(name).push(example);
    }
    const order = [...GROUPS.map(g => g[1]), 'Other'];
    for (const name of order) {
        if (!groups.has(name)) continue;
        const heading = document.createElement('h3');
        heading.textContent = name;
        el.examples.append(heading);
        for (const example of groups.get(name)) {
            const button = document.createElement('button');
            button.textContent = example.title;
            button.dataset.name = example.name;
            button.addEventListener('click', () => selectExample(example, true));
            el.examples.append(button);
        }
    }

    const last = state.examples.find(e => e.name === storage.get('example')) || state.examples[0];
    if (last) selectExample(last, false);
}

function selectExample(example, autorun) {
    state.current = example;
    storage.set('example', example.name);
    el.title.textContent = example.title;
    el.description.textContent = example.description;
    el.code.value = storage.get('code.' + example.name) ?? example.code;
    for (const button of el.examples.querySelectorAll('button')) {
        if (button.dataset.name === example.name) button.setAttribute('aria-current', 'true');
        else button.removeAttribute('aria-current');
    }
    if (autorun) run();
}

/*
 * Running scripts: the server streams one JSON message per line
 */
function decode(b64) {
    const bin = atob(b64);
    const bytes = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
    return new Float32Array(bytes.buffer);
}

function decodeInt8(b64) {
    const bin = atob(b64);
    const values = new Int8Array(bin.length);
    for (let i = 0; i < bin.length; i++) values[i] = bin.charCodeAt(i);
    return values;
}

const ARRAY_FIELDS = ['re', 'im', 'V', 'data', 't'];

function addPlot(message) {
    const slash = message.title.indexOf('/');
    const title = slash < 0 ? message.title : message.title.slice(0, slash);
    const label = slash < 0 ? '' : message.title.slice(slash + 1);

    let panel = state.panels.get(title);
    if (!panel) {
        panel = { title, kind: message.kind, frames: [], series: {}, state: {}, card: null };
        state.panels.set(title, panel);
        createCard(panel);
    }

    if (message.kind === 'scalar') {
        const s = panel.series[message.name] ??= { t: [], v: [] };
        s.t.push(message.t);
        s.v.push(message.value);
        if (!panel.frames.length) panel.frames.push({ label: '' });
        return;
    }

    const frame = { ...message, label };
    for (const key of ARRAY_FIELDS) if (typeof frame[key] === 'string') frame[key] = decode(frame[key]);
    // 8 bit quantised frames stay compact in memory, the renderers multiply by frame.scale
    for (const key of ['data', 're', 'im']) {
        if (typeof frame[key + '_i8'] === 'string') {
            frame[key] = decodeInt8(frame[key + '_i8']);
            delete frame[key + '_i8'];
        }
    }
    if (frame.series) frame.series = frame.series.map(s => ({ name: s.name, y: decode(s.y) }));
    if (frame.paths) frame.paths = frame.paths.map(decode);
    if (frame.states) frame.states = frame.states.map(decode);
    panel.frames.push(frame);
}

function handle(message) {
    if (message.type === 'plot') {
        addPlot(message);
    } else if (message.type === 'stdout' || message.type === 'stderr') {
        const span = document.createElement('span');
        span.textContent = message.text;
        if (message.type === 'stderr') span.className = 'err';
        el.console.append(span);
        el.console.hidden = false;
        el.console.scrollTop = el.console.scrollHeight;
        if (message.type === 'stderr') state.failed = true;
    } else if (message.type === 'done') {
        state.finished = message;
    }
}

async function run() {
    if (state.running) await stop();

    // Reset
    state.panels.clear();
    resetSharedState();
    visible.clear();
    el.panels.textContent = '';
    el.console.textContent = '';
    el.console.hidden = true;
    state.position = 0;
    state.follow = true;
    state.failed = false;
    state.finished = null;
    setPlaying(false);
    setRunning(true);
    setStatus('Running...');

    const abort = new AbortController();
    state.abort = abort;
    const started = performance.now();

    try {
        const response = await fetch('/api/run', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ lua: el.code.value }),
            signal: abort.signal,
        });
        if (!response.ok) throw new Error(`Server answered ${response.status}: ${await response.text()}`);

        const reader = response.body.getReader();
        const decoder = new TextDecoder();
        let buffer = '';
        for (;;) {
            const { value, done } = await reader.read();
            if (done) break;
            buffer += decoder.decode(value, { stream: true });
            let newline;
            while ((newline = buffer.indexOf('\n')) >= 0) {
                const line = buffer.slice(0, newline);
                buffer = buffer.slice(newline + 1);
                if (line) handle(JSON.parse(line));
            }
            state.dirty = true;
            setStatus(`Running... ${((performance.now() - started) / 1000).toFixed(1)} s, ${countFrames()} frames`);
        }

        const ok = state.finished?.ok && !state.failed;
        setStatus(ok ? `Finished in ${state.finished.elapsed.toFixed(2)} s, ${countFrames()} frames`
                     : 'Stopped with an error, see the output below the script', !ok);
    } catch (e) {
        if (e.name !== 'AbortError') setStatus(String(e.message || e), true);
        else setStatus('Stopped');
    } finally {
        if (state.abort === abort) {
            state.abort = null;
            setRunning(false);
            // Play the result from the start once it is complete
            if (maxFrames() > 1 && state.follow && !state.failed) {
                state.position = 0;
                setPlaying(true);
            }
            state.follow = false;
            state.dirty = true;
        }
    }
}

async function stop() {
    if (!state.running) return;
    try { await fetch('/api/stop', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{}' }); } catch { /* server gone */ }
    state.abort?.abort();
    state.abort = null;
    setRunning(false);
    setStatus('Stopped');
}

function setRunning(running) {
    state.running = running;
    el.stop.disabled = !running;
}

function setStatus(text, error = false) {
    el.status.textContent = text;
    el.status.classList.toggle('error', error);
}

/*
 * Timeline. Panels may have different numbers of frames, they all share one
 * position: a panel with n frames shows frame round(fraction * (n - 1)).
 */
function maxFrames() {
    let n = 0;
    for (const panel of state.panels.values()) {
        const count = panel.kind === 'scalar' ? Math.max(0, ...Object.values(panel.series).map(s => s.t.length)) : panel.frames.length;
        n = Math.max(n, count);
    }
    return n;
}

function countFrames() {
    let n = 0;
    for (const panel of state.panels.values()) if (panel.kind !== 'scalar') n += panel.frames.length;
    return n;
}

function setPlaying(playing) {
    state.playing = playing;
    el.play.textContent = playing ? 'Pause' : 'Play';
    if (playing) state.follow = false;
}

let lastTick = 0;
function tick(now) {
    requestAnimationFrame(tick);

    const total = maxFrames();
    if (state.playing && total > 1) {
        const interval = 1000 / Number(el.speed.value);
        if (now - lastTick >= interval) {
            lastTick = now;
            if (state.position >= total - 1 && !el.loop.checked) {
                setPlaying(false);
            } else {
                state.position = (state.position + 1) % total;
                state.dirty = true;
            }
        }
    }
    if (state.follow && state.running && total > 0 && state.position !== total - 1) {
        state.position = total - 1;
        state.dirty = true;
    }

    if (!state.dirty) return;
    state.dirty = false;
    draw();
}

function draw() {
    const total = maxFrames();
    state.position = Math.max(0, Math.min(total - 1, state.position));
    const fraction = total > 1 ? state.position / (total - 1) : 0;

    el.transport.hidden = total <= 1;
    el.slider.max = Math.max(0, total - 1);
    el.slider.value = state.position;

    let label = '';
    let longest = 0;
    for (const panel of state.panels.values()) {
        const index = Math.round(fraction * (panel.frames.length - 1));
        if (panel.kind !== 'scalar' && panel.frames.length > longest) {
            longest = panel.frames.length;
            label = panel.frames[index]?.label ?? '';
        }
        drawPanel(panel, index, fraction);
    }
    el.frameLabel.textContent = label || `${state.position + 1} / ${total}`;
}

// Only panels on screen are drawn: a dozen heatmaps per tick would otherwise stall playback
const visible = new Set();
const observer = new IntersectionObserver(entries => {
    for (const entry of entries) {
        if (entry.isIntersecting) visible.add(entry.target); else visible.delete(entry.target);
    }
    state.dirty = true;
}, { rootMargin: '200px' });

function drawPanel(panel, index, fraction) {
    if (!panel.frames.length) return;
    if (!visible.has(panel.card.card) && panel.drawnOnce && !state.drawAll) return;
    panel.drawnOnce = true;
    const result = render(panel.card.canvas, panel, Math.max(0, index), fraction, state.theme);
    panel.hover = result.hover;

    const key = JSON.stringify(result.legend);
    if (key !== panel.legendKey) {
        panel.legendKey = key;
        panel.card.legend.textContent = '';
        for (const item of result.legend) {
            const span = document.createElement('span');
            span.className = 'item';
            if (item.color || item.gradient) {
                const swatch = document.createElement('span');
                swatch.className = item.gradient ? 'bar' : 'swatch';
                swatch.style.background = item.gradient || item.color;
                span.append(swatch);
            }
            span.append(item.label);
            panel.card.legend.append(span);
        }
    }
}

function createCard(panel) {
    const card = document.createElement('article');
    card.className = 'card' + (SQUARE_KINDS.has(panel.kind) ? ' square' : '');
    card.style.order = kindRank(panel.kind);
    const header = document.createElement('header');
    const heading = document.createElement('h3');
    heading.textContent = panel.title;
    const expand = document.createElement('button');
    expand.className = 'ghost expand';
    expand.title = 'Enlarge / shrink this panel';
    expand.setAttribute('aria-label', 'Enlarge or shrink ' + panel.title);
    expand.textContent = '\u2922';
    expand.addEventListener('click', () => {
        card.classList.toggle('expanded');
        state.dirty = true;
    });
    header.append(heading, expand);
    const canvas = document.createElement('canvas');
    canvas.setAttribute('role', 'img');
    canvas.setAttribute('aria-label', panel.title);
    const legend = document.createElement('div');
    legend.className = 'legend';
    card.append(header, canvas, legend);
    el.panels.append(card);
    panel.card = { card, canvas, legend };
    observer.observe(card);

    canvas.addEventListener('mousemove', event => {
        const rect = canvas.getBoundingClientRect();
        const text = panel.hover?.(event.clientX - rect.left, event.clientY - rect.top);
        if (!text) { el.tooltip.hidden = true; return; }
        el.tooltip.textContent = text;
        el.tooltip.hidden = false;
        const width = el.tooltip.offsetWidth;
        const x = event.clientX + 14 + width > window.innerWidth ? event.clientX - 14 - width : event.clientX + 14;
        el.tooltip.style.left = x + 'px';
        el.tooltip.style.top = (event.clientY + 14) + 'px';
    });
    canvas.addEventListener('mouseleave', () => { el.tooltip.hidden = true; });
}

/*
 * Wiring
 */
el.run.addEventListener('click', run);
el.stop.addEventListener('click', stop);
el.reset.addEventListener('click', () => {
    if (!state.current) return;
    storage.remove('code.' + state.current.name);
    el.code.value = state.current.code;
});
el.code.addEventListener('input', () => {
    if (!state.current) return;
    if (el.code.value === state.current.code) storage.remove('code.' + state.current.name);
    else storage.set('code.' + state.current.name, el.code.value);
});
el.code.addEventListener('keydown', event => {
    if (event.key === 'Tab' && !event.shiftKey && !event.ctrlKey && !event.metaKey) {
        event.preventDefault();
        el.code.setRangeText('    ', el.code.selectionStart, el.code.selectionEnd, 'end');
        el.code.dispatchEvent(new Event('input'));
    }
});
function showEditor(show) {
    el.code.hidden = !show;
    el.toggleEditor.textContent = show ? 'Hide script' : 'Show script';
}
el.toggleEditor.addEventListener('click', () => {
    showEditor(el.code.hidden);
    storage.set('editor', el.code.hidden ? '0' : '1');
});
el.play.addEventListener('click', () => {
    if (!state.playing && state.position >= maxFrames() - 1) state.position = 0;
    setPlaying(!state.playing);
});
function stepFrame(delta) {
    setPlaying(false);
    state.follow = false;
    state.position += delta;
    state.dirty = true;
}
el.prev.addEventListener('click', () => stepFrame(-1));
el.next.addEventListener('click', () => stepFrame(+1));
el.speed.addEventListener('change', () => storage.set('fps', el.speed.value));
el.loop.addEventListener('change', () => storage.set('loop', el.loop.checked ? '1' : '0'));
el.slider.addEventListener('input', () => {
    state.position = Number(el.slider.value);
    state.follow = false;
    setPlaying(false);
    state.dirty = true;
});
el.theme.addEventListener('click', () => {
    const dark = getComputedStyle(document.documentElement).colorScheme.includes('dark');
    document.documentElement.dataset.theme = dark ? 'light' : 'dark';
    storage.set('theme', document.documentElement.dataset.theme);
    state.theme = readTheme();
    for (const panel of state.panels.values()) panel.legendKey = null;
    state.dirty = true;
});
document.addEventListener('keydown', event => {
    if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
        event.preventDefault();
        run();
    } else if (event.key === ' ' && !['TEXTAREA', 'INPUT', 'SELECT', 'BUTTON'].includes(document.activeElement?.tagName)) {
        event.preventDefault();
        el.play.click();
    } else if ((event.key === 'ArrowLeft' || event.key === 'ArrowRight') && !['TEXTAREA', 'INPUT', 'SELECT'].includes(document.activeElement?.tagName)) {
        stepFrame(event.key === 'ArrowLeft' ? -1 : 1);
    }
});
window.addEventListener('resize', () => { state.dirty = true; });
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
    state.theme = readTheme();
    for (const panel of state.panels.values()) panel.legendKey = null;
    state.dirty = true;
});

if (storage.get('editor') === '0') showEditor(false);
if ([...el.speed.options].some(o => o.value === storage.get('fps'))) el.speed.value = storage.get('fps');
if (storage.get('loop') === '0') el.loop.checked = false;

const savedTheme = storage.get('theme');
if (savedTheme) {
    document.documentElement.dataset.theme = savedTheme;
    state.theme = readTheme();
}

requestAnimationFrame(tick);
loadExamples();

// For debugging from the browser console (and automated checks in background tabs,
// where requestAnimationFrame does not fire)
window.onedse = { state, draw, maxFrames, drawAll() { state.drawAll = true; draw(); state.drawAll = false; } };
