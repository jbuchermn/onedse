-- The classical limit: making hbar small
-- The same anharmonic oscillator, the same starting point in phase space, two values of
-- hbar. A minimal-uncertainty packet has area ~hbar in phase space; next to each Wigner
-- function runs a classical ensemble with exactly the same initial distribution.
-- The Wigner function does NOT simply turn into the classical density: for small hbar it
-- hugs the classical filament, but fills its inside with ever finer fringes, positive and
-- negative. The classical limit is a statement about what can be resolved: seen with a
-- fixed finite resolution (bottom panels), the fringes of the hbar = 0.2 state average
-- out and the classical density is back, while for hbar = 1 they are too coarse to vanish.

local function V(x) return x^4/16 end
local x0 = 3.5
local p_window = 6                       -- classically |p| < 4.4 at this energy
local resolution = 0.3                   -- of the "apparatus", in x and in p

local frames, t_final, dt = 80, 16, 0.004

for _, hbar in ipairs({ 1, 0.2 }) do
    local g = grid_hbar(-8, 8, 640, hbar)
    local sys = g:schroedinger()
    sys:set_potential(V)

    -- Round blob of area ~hbar: sigma_x sigma_p = hbar/2
    local sigma_x = 0.8*math.sqrt(hbar)
    local sigma_p = hbar/(2*sigma_x)
    local psi = g:gaussian(x0, 0, sigma_x)
    local ensemble = classical_ensemble(120000, x0, 0, sigma_x, sigma_p, 5)

    local name = string.format("hbar = %g", hbar)
    for i = 0, frames do
        local t = i*t_final/frames
        local label = string.format("t = %.1f", t)

        plot:wigner_p(psi, "Quantum, " .. name .. "/" .. label, 320, p_window)
        plot:ensemble_p(ensemble, sys, "Classical, " .. name .. "/" .. label, 320, p_window)
        plot:scalar("Mean position, " .. name, t, "quantum", psi:expect_x())
        plot:scalar("Mean position, " .. name, t, "classical", ensemble:expect_x())
        plot:wigner_smooth(psi, "Quantum at finite resolution, " .. name .. "/" .. label, 320, p_window, resolution, resolution)
        plot:ensemble_smooth(ensemble, sys, "Classical at finite resolution, " .. name .. "/" .. label, 320, p_window, resolution, resolution)
        plot:scalar("Negativity of the Wigner function", t, name, psi:negativity(320))
        plot:scalar("Negativity left at finite resolution", t, name, psi:negativity_smoothed(320, resolution, resolution))
        plot:scalar("Quantum - classical distance at finite resolution", t, name, ensemble:distance_smoothed(psi, 320, resolution, resolution))

        if i < frames then
            sys:propagate(psi, t, t + t_final/frames, dt)
            ensemble:propagate(sys, t, t + t_final/frames, 4*dt)
        end
    end
    print(string.format("%s: energy %.4f (classical ensemble %.4f)", name, sys:energy(psi), ensemble:energy(sys)))
end
