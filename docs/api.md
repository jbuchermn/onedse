# Lua API

Units: ħ = 1. Complex numbers are created with `complex(re, im)` and have `get_re()`,
`get_im()`, `add/sub/mul/div(other)`. `math.sinh/cosh/tanh` are available.

All plot functions take a title `"Panel/frame label"`: calls with the same panel name
become the frames of one animation. Without a `/` the plot is a single still.

## Fourier grids

```lua
g = grid(lower, upper, n)          -- periodic, n points (powers of two are fastest)
g = grid_hbar(lower, upper, n, hbar)   -- the same with a different value of hbar (1D only)
g:dx()  g:length()  g:k_max()  g:p_max()  g:hbar()
```

With `grid_hbar` everything on that grid uses p = ħk: packets, expectation values, Wigner
functions (axes in x and p, normalised to ∫W dx dp = 1), energies and propagation.

### Wave functions

| | |
|---|---|
| `g:wavefunction(function(x) return complex(..) end)` | sampled and normalised |
| `g:gaussian(x0, p0, sigma)` | minimal uncertainty packet, `sigma` = position uncertainty |
| `psi:norm()`, `psi:normalize()`, `psi:validate()` | |
| `psi:expect_x()`, `psi:expect_p()`, `psi:sigma_x()`, `psi:sigma_p()` | |
| `psi:probability(a, b)` | probability in `[a, b]` |
| `psi:dot(other)` | `<psi|other>`, complex |
| `psi:copy()`, `psi:add(other)`, `psi:sub(other)`, `psi:mul(c)`, `psi:div(c)` | plus in-place `iadd`, `isub`, `imul`, `idiv` |
| `psi:density_matrix()` | pure state as density matrix |
| `psi:negativity(points)` | `∫|W| dx dp − 1`: 0 if the Wigner function could be a classical density |
| `psi:negativity_smoothed(points, sx, sp)` | negativity left after coarse-graining with resolution `(sx, sp)` |
| `psi:wigner_ensemble(n, points, seed)` | classical ensemble sampled from the Wigner function, negative regions as negative weights |
| `psi:bohmian(n, t)` | `n` Bohmian trajectories starting at the quantiles of `|psi|²` |

### Schrödinger / Gross–Pitaevskii equation

`i ∂ψ/∂t = [p²/2m + V(x, t) + g|ψ|² − iW(x)] ψ`, Strang split-operator (second order in `dt`).

| | |
|---|---|
| `sys = g:schroedinger()` | |
| `sys:set_mass(m)` | default 1 |
| `sys:set_potential(function(x) .. end)` | |
| `sys:set_potential_td(function(x, t) .. end)` | time-dependent, evaluated at the midpoint of every step |
| `sys:set_nonlinearity(g)` | Gross–Pitaevskii |
| `sys:set_absorber(width, strength)` | absorbing layer at both ends |
| `sys:step(psi, t, dt)`, `sys:propagate(psi, t0, t1, dt)` | |
| `sys:kick(psi, tau)`, `sys:drift(psi, tau)` | potential part / kinetic part only (kicked systems) |
| `sys:relax(psi, dt, steps)` | imaginary time: converges to the ground state |
| `sys:energy(psi)`, `sys:kinetic_energy(psi)`, `sys:potential_energy(psi)` | |
| `sys:eigenstates(n)` | `{ {E0, psi0}, {E1, psi1}, .. }` by dense diagonalisation (≤ 4096 points) |

### Classical and Bohmian mechanics

| | |
|---|---|
| `ens = classical_ensemble(n, x0, p0, sigma_x, sigma_p, seed)` | Gaussian cloud = Wigner function of a Gaussian packet if `sigma_p = 1/(2 sigma_x)` |
| `ens:propagate(sys, t0, t1, dt)` | velocity Verlet in the potential of `sys` |
| `ens:kick(sys, tau)`, `ens:drift(sys, tau)` | counterparts of `sys:kick`, `sys:drift` |
| `ens:expect_x()`, `ens:expect_p()`, `ens:expect_p2()`, `ens:probability(a, b)`, `ens:energy(sys)` | |
| `ens:distance(psi, points)` | `½∫|W_classical − W_quantum| dx dp` |
| `ens:distance_smoothed(psi, points, sx, sp)` | the same at finite resolution |
| `bohm:advance(psi, mass, t, dt)` | call after every `sys:step` |
| `st = spacetime(max_points)`, `st:add(psi, t)` | record `|psi(x)|²` over time |

### Two dimensions / two particles

```lua
g2 = grid2d(gx, gy)
```

| | |
|---|---|
| `g2:wavefunction(function(x, y) return complex(..) end)` | |
| `g2:product(a, b)` | `a(x) b(y)` |
| `g2:multiply(a, b)` | pointwise product of two 2D wave functions |
| `psi:norm()`, `normalize()`, `expect_x()`, `expect_y()`, `probability(x0, x1, y0, y1)`, `copy()` | |
| `psi:symmetrize(±1)` | bosons / fermions (two particles on identical grids) |
| `psi:reduced(1 or 2)` | reduced density matrix of one particle |
| `psi:slice_x(x)`, `psi:slice_y(y)` | cut, as (unnormalised) 1D wave function |
| `sys = g2:schroedinger()` | `set_mass(mx, my)`, `set_potential(function(x, y))`, `set_potential_td(function(x, y, t))`, `set_nonlinearity`, `set_absorber`, `step`, `propagate`, `relax`, `energy`, `angular_momentum` |
| `rho:trace()`, `rho:purity()`, `rho:entropy()`, `rho:eigenvalues(n)` | density matrix on a grid |
| `rho:coherence(a, b)` | `|ρ(a, b)|` |
| `rho = g:density_matrix()`, `rho:add(weight, psi)` | average pure states into a mixture |

### Continuous measurement (quantum trajectories)

One realisation of `dρ/dt = −i[H, ρ] − κ/2 [x, [x, ρ]]`: conditioned on what the environment
records, the state stays pure and follows a stochastic Schrödinger equation. Averaging
`|ψ⟩⟨ψ|` over many runs gives back ρ.

| | |
|---|---|
| `m = position_measurement(kappa, seed)` | |
| `m:apply(psi, dt)` | call after every `sys:step(psi, t, dt)` |

### Dirac equation (1+1 dimensions)

`i ∂ψ/∂t = [c σx p + m c² σz + V(x)] ψ`; the free part is propagated exactly.

| | |
|---|---|
| `sp = spinor(upper, lower)` | from two wave functions, normalised |
| `dirac = g:dirac()` | `set_mass(m)`, `set_c(c)`, `set_potential(function(x))`, `set_absorber(width, strength)` |
| `dirac:propagate(sp, t0, t1, dt)` | |
| `dirac:project(sp, ±1)` | project onto positive / negative energy solutions |
| `dirac:positive_energy_fraction(sp)`, `dirac:velocity(sp)`, `dirac:energy(sp)` | |
| `sp:norm()`, `sp:expect_x()`, `sp:probability(a, b)` | |

## Spectral bases

```lua
basis = orthogonal_hermite(n)                               -- whole real line
basis = scaled_basis(orthogonal_legendre(n), lower, upper)  -- interval; also orthogonal_chebyshev
```

| | |
|---|---|
| `basis:wavefunction(function(x) .., order)` | `order`: extra quadrature order |
| `wf:density_operator()`, `basis:density_operator()`, `rho:add(p, wf)` | |
| `rho:trace()`, `rho:purity()`, `rho:entropy()`, `rho:diagonalize()` | `diagonalize` returns `{ {p1, wf1}, .. }` |
| `op = basis:map_wavefunction()`, `op:add_term(l, r, V, order)` | adds `<e_i| ∂ˡ V(x) ∂ʳ |e_j>` |
| `sop = basis:map_density_operator()` | `add_left`, `add_right`, `add_both`, `add_from_wavefunction_left/right/both` |
| `op:rk_propagator("RK4")`, `op:diagonal_propagator()` | for `d/dt state = op state` |
| `prop:propagate(state, t0, t1, eps)`, `prop:propagate_const_step(state, t0, t1, dt)`, `prop:step(state, t, dt)` | `eps = 0`: default tolerance |

## Plots

| | |
|---|---|
| `plot:wavefunction(psi, title, points)` | `|ψ|²` coloured by phase; spectral or grid wave function (`points = 0`: all) |
| `plot:wave(sys, psi, title)` | same, with the potential |
| `plot:momentum(psi, title, p_max)` | momentum space |
| `plot:wigner(state, title, points)` | density operator, grid wave function or grid density matrix |
| `plot:ensemble(ens, sys, title, points)` | classical phase space density on the same grid as `plot:wigner` |
| `plot:wigner_p(state, title, points, p_max)`, `plot:ensemble_p(ens, sys, title, points, p_max)` | only `|p| ≤ p_max` (much smaller frames for small ħ) |
| `plot:wigner_smooth(psi, title, points, p_max, sx, sp)`, `plot:ensemble_smooth(ens, sys, ..)` | coarse-grained with a Gaussian of widths `(sx, sp)` |
| `plot:momentum_compare(psi, ens, title, p_max, bins)` | quantum and classical momentum distribution in one plot |
| `plot:density_matrix(rho, title, points)` | `|ρ(x, x')|` |
| `plot:scalar(panel, t, name, value)` | time series; several names per panel share one axis |
| `plot:levels(sys, n, title)` | energy level diagram with eigenfunctions |
| `plot:trajectories(bohm, title)`, `plot:carpet(st, title)` | |
| `plot:field(psi2d, title)`, `plot:wave2d(sys2d, psi2d, title)` | brightness `|ψ|²`, hue phase |
| `plot:marginals(psi2d, title)` | densities of both particles |
| `plot:spinor(dirac, sp, title)` | densities of both components |

## Message format

`onedse_server` answers `POST /api/run {"lua": "..."}` with one JSON object per line while the
script runs; `onedse` collects the same objects in `data.json` under `plots`.

```
{"type": "stdout" | "stderr", "text": ".."}
{"type": "plot", "kind": .., "title": "Panel/frame", ..}
{"type": "done", "ok": true, "elapsed": 1.2}
```

Arrays are base64 encoded little-endian float32.

| kind | fields |
|---|---|
| `wavefunction` | `lower`, `upper`, `re`, `im`, optional `V`, `space: "p"` |
| `wigner` | `lower_x`, `upper_x`, `lower_p`, `upper_p`, `nx`, `np`, `data[ix*np + ip]`, optional `classical`. Frames larger than 128² come as `data_i8` (base64 int8) and `scale` instead of `data` |
| `image` | `x0`, `x1`, `y0`, `y1`, `nx`, `ny`, `data[iy*nx + ix]`, `xlabel`, `ylabel` |
| `field` | like `image` with `re`, `im`, optional `V`. Frames larger than 128² come as `re_i8`, `im_i8` and `scale` |
| `curves` | `lower`, `upper`, `series: [{name, y}]`, optional `V` |
| `scalar` | `t`, `name`, `value` |
| `paths` | `t`, `paths: [..]` |
| `levels` | `lower`, `upper`, `V`, `energies`, `states` |
