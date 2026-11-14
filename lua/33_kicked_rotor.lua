-- Chaos: the kicked rotor
-- A rotor that is kicked periodically, H = p^2/2 + K cos(x) sum_n delta(t - n). For K = 5
-- the classical motion is chaotic: the momentum performs a random walk and the energy
-- grows without bound. The quantum rotor follows for a few kicks - and then stops absorbing
-- energy ("dynamical localisation", the cousin of Anderson localisation). Its momentum
-- distribution freezes, while the classical one keeps spreading. Halving hbar postpones
-- the break: the quantum system follows the classical diffusion four times as long.

local K = 5
local kicks = 150
local two_pi = 2*math.pi

local function setup(hbar, n)
    local g = grid_hbar(0, two_pi, n, hbar)
    local sys = g:schroedinger()
    sys:set_potential(function(x) return K*math.cos(x) end)
    -- Start at rest, anywhere on the circle
    local psi = g:wavefunction(function(x) return complex(1, 0) end)
    return sys, psi
end

local sys1, psi1 = setup(1, 1024)
local sys2, psi2 = setup(0.5, 2048)
local ensemble = classical_ensemble(100000, math.pi, 0, 30, 0.01, 2)

for n = 0, kicks do
    plot:scalar("Energy <p^2>/2", n, "classical", ensemble:expect_p2()/2)
    plot:scalar("Energy <p^2>/2", n, "quantum, hbar = 1", sys1:kinetic_energy(psi1))
    plot:scalar("Energy <p^2>/2", n, "quantum, hbar = 0.5", sys2:kinetic_energy(psi2))

    local label = string.format("after %d kicks", n)
    if n >= 4 then
        -- (the first kicks are left out: the initial spike at p = 0 would dwarf everything after)
        plot:momentum_compare(psi1, ensemble, "Momentum distribution, hbar = 1/" .. label, 70, 140)
        plot:momentum_compare(psi2, ensemble, "Momentum distribution, hbar = 0.5/" .. label, 70, 140)
    end
    if n <= 8 then
        plot:ensemble_p(ensemble, sys1, "Classical phase space (x, p), first 8 kicks/" .. label, 256, 16)
    end

    if n < kicks then
        sys1:kick(psi1, 1); sys1:drift(psi1, 1)
        sys2:kick(psi2, 1); sys2:drift(psi2, 1)
        ensemble:kick(sys1, 1); ensemble:drift(sys1, 1)
    end
end

print(string.format("After %d kicks: classical energy %.0f, quantum %.0f (hbar = 1), %.0f (hbar = 0.5)",
    kicks, ensemble:expect_p2()/2, sys1:kinetic_energy(psi1), sys2:kinetic_energy(psi2)))
