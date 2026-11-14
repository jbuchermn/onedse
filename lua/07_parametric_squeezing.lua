-- Parametric squeezing
-- A time-dependent Hamiltonian: the trap frequency is modulated at twice its own
-- frequency, omega(t)^2 = 1 + eps sin(2t). Like a child on a swing, the ground state gets
-- amplified - but only in one quadrature. The Wigner function becomes a rotating ellipse
-- whose area stays constant while it gets thinner and thinner.

local eps = 0.25

local g = grid(-16, 16, 512)
local sys = g:schroedinger()
sys:set_potential_td(function(x, t) return 0.5*(1 + eps*math.sin(2*t))*x*x end)

local psi = g:gaussian(0, 0, 1/math.sqrt(2))

local frames, t_final = 300, 24
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)

    plot:wigner(psi, "Wigner function/" .. label, 128)
    plot:wave(sys, psi, "Wave function/" .. label)
    plot:scalar("Uncertainty", t, "sigma_x", psi:sigma_x())
    plot:scalar("Uncertainty", t, "sigma_p", psi:sigma_p())
    plot:scalar("Energy", t, "<H>", sys:energy(psi))

    if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.004) end
end
