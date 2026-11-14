# onedse — quantum mechanics in phase space

Simulate and *watch* quantum mechanics: wave functions coloured by their phase, Wigner
functions with their negative regions, density matrices losing their coherence. Scripted in
Lua, computed in C++, shown live in the browser.

What started as a one-dimensional Schrödinger / Liouville–von Neumann solver now covers

| | |
|---|---|
| **Closed systems** | coherent and squeezed states, tunnelling, scattering, revivals and quantum carpets, time-dependent Hamiltonians, eigenstates of arbitrary potentials |
| **Open systems** | Lindblad master equations, decoherence of cat states, damped oscillators |
| **Measurement and decoherence** | a pointer entangling with a superposition (exact, two particles) and the Born weights appearing in the reduced state; recoherence when the environment is a single degree of freedom; single runs of a continuous measurement collapsing while their average follows the Lindblad equation |
| **Quantum vs. classical** | the same system evolved both ways, side by side in phase space; the classical limit with an adjustable ħ and finite resolution; classical flow of a Wigner function with its negative parts; tunnelling vs. ħ; the kicked rotor (chaos and dynamical localisation); Bohmian trajectories |
| **Two dimensions** | double slit, quantised vortices |
| **Beyond linear QM** | Gross–Pitaevskii equation (solitons, condensates) |
| **Beyond non-relativistic QM** | Dirac equation in 1+1 dimensions (Zitterbewegung, Klein paradox) |

## Run it

With [Nix](https://nixos.org) (flakes enabled) — Linux and macOS:

```sh
nix run github:jbuchermn/onedse          # or, in a checkout: nix run
```

Open <http://localhost:8080>, pick an example on the left, edit the script, press
<kbd>Ctrl/Cmd</kbd>+<kbd>Enter</kbd>. Results stream in while the script is still running.

```sh
nix run .#script -- lua/02_tunnelling.lua --plot   # headless: data.json + PNGs via matplotlib
nix flake check                                     # unit tests + every example, in the sandbox
```

### Development

```sh
nix develop
cmake -S cpp -B build && cmake --build build -j
ctest --test-dir build --output-on-failure          # 57 unit tests, 24 example scripts
./build/main/server/onedse_server                   # serves web/ and lua/ straight from the checkout
./build/main/script/onedse --check lua/03_scattering.lua
```

Without Nix you need CMake, a C++17 compiler, Boost (headers), Eigen 3, FFTW 3, Lua ≥ 5.3,
nlohmann_json, cpp-httplib and (for the tests) GoogleTest. The frontend has no build step
and no dependencies.

## Layout

```
cpp/main/core/discretization   Galerkin bases (Legendre, Hermite, Chebyshev): arbitrary operators
cpp/main/core/state, map, propagation
                               wave functions, density operators, superoperators, RK4 / exact propagators
cpp/main/core/grid             Fourier grids: split-operator Schrödinger / Gross–Pitaevskii in 1D and 2D,
                               Dirac, classical ensembles, Bohmian trajectories
cpp/main/core/lua              the Lua API
cpp/main/script                onedse: run a script headless
cpp/main/server                onedse_server: web UI, streams results as NDJSON
cpp/test                       unit tests against analytic results
lua/                           examples (shown in the web UI)
web/                           frontend: plain ES modules + canvas
python/plot.py                 matplotlib export of data.json
docs/api.md                    Lua API reference and message format
```

Two discretisations coexist on purpose. The **spectral basis** assembles any operator from
terms `<e_i| d^l/dx^l V(x) d^r/dx^r |e_j>` and handles density operators and Lindblad
equations, at dense-matrix cost. The **Fourier grid** only knows `p²/2m + V(x, t) + g|ψ|²`,
but is fast enough for long runs, 2D and two particles. Both produce the same plots.

## Writing scripts

```lua
local g = grid(-10, 10, 512)                         -- periodic Fourier grid
local sys = g:schroedinger()
sys:set_potential(function(x) return x^4/16 end)

local psi = g:gaussian(4, 0, 0.6)                    -- x0, p0, sigma
for i = 0, 100 do
    local t = 0.2*i
    plot:wave(sys, psi, "Wave function/t = " .. t)   -- "Panel/frame label"
    plot:wigner(psi, "Wigner function/t = " .. t, 128)
    plot:scalar("Energy", t, "<H>", sys:energy(psi))
    sys:propagate(psi, t, t + 0.2, 0.005)
end
```

Plots with the same panel name become frames of one animation; all panels share one
timeline. Units are ħ = 1 unless a grid is created with `grid_hbar`. The full API is in [docs/api.md](docs/api.md).

## Notes

- The server executes Lua on your machine. It listens on `127.0.0.1` only, rejects
  cross-origin requests, and runs scripts in a sandbox without `io`, `os.execute`, `require`.
  `--host 0.0.0.0` exposes it to your network — think twice.
- The Wigner function of a grid state is sampled on `points × points` cells covering the whole
  grid in x and `|p| < π/(2 Δx)` with `Δx = L/points`. States with larger momenta alias; use more points.
- Grids are periodic. Use `set_absorber` when something should leave for good.
