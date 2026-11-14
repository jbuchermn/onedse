-- Quantum versus classical phase space flow
-- In a harmonic potential the Wigner function follows the classical Liouville equation
-- exactly. In an anharmonic potential it does not: the classical ensemble winds up into
-- an ever finer whorl, while the quantum state develops interference structures with
-- negative values (blue) and later partially revives.

local g = grid(-10, 10, 512)
local sys = g:schroedinger()
sys:set_potential(function(x) return x^4/16 end)

local x0, sigma = 4, 0.6
local psi = g:gaussian(x0, 0, sigma)
local ensemble = classical_ensemble(60000, x0, 0, sigma, 1/(2*sigma), 7)

local frames, t_final = 120, 24
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.1f", t)

    plot:wigner(psi, "Wigner function (quantum)/" .. label, 128)
    plot:ensemble(ensemble, sys, "Phase space density (classical)/" .. label, 128)
    plot:wave(sys, psi, "Wave function/" .. label)
    plot:scalar("Mean position", t, "quantum", psi:expect_x())
    plot:scalar("Mean position", t, "classical", ensemble:expect_x())

    if i < frames then
        sys:propagate(psi, t, t + t_final/frames, 0.005)
        ensemble:propagate(sys, t, t + t_final/frames, 0.005)
    end
end
