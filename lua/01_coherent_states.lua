-- Coherent and squeezed states
-- A displaced Gaussian in a harmonic trap. With the ground state width it is a coherent
-- state: the Wigner function is a rigid blob circling the origin like a classical particle.
-- Any other width gives a squeezed state, which breathes twice per period.

local squeeze = 0.5                     -- 1 = coherent state, try 0.3 or 2
local sigma0 = 1/math.sqrt(2)           -- ground state width for m = omega = 1

local g = grid(-12, 12, 512)
local sys = g:schroedinger()
sys:set_potential(function(x) return 0.5*x*x end)

local psi = g:gaussian(4, 0, squeeze*sigma0)

local T = 2*math.pi
local frames = 192
for i = 0, frames do
    local t = i*T/frames
    local label = string.format("t = %.2f", t)

    plot:wave(sys, psi, "Wave function/" .. label)
    plot:wigner(psi, "Wigner function/" .. label, 128)
    plot:scalar("Expectation values", t, "<x>", psi:expect_x())
    plot:scalar("Expectation values", t, "<p>", psi:expect_p())
    plot:scalar("Uncertainty", t, "sigma_x", psi:sigma_x())
    plot:scalar("Uncertainty", t, "sigma_p", psi:sigma_p())
    plot:scalar("Uncertainty", t, "sigma_x sigma_p", psi:sigma_x()*psi:sigma_p())

    if i < frames then sys:propagate(psi, t, t + T/frames, 0.005) end
end

print(string.format("Energy: %.6f, norm: %.12f", sys:energy(psi), psi:norm()))
