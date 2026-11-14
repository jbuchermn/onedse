-- Tunnelling in a double well
-- A wave packet starts in the left well with less energy than the barrier. Quantum
-- mechanically it oscillates between the wells; while it is under way, the Wigner function
-- shows interference fringes with negative values between the two lobes. The classical
-- ensemble with the same initial phase space distribution stays where it is.

local a, h = 1.8, 2.0                   -- well positions +-a, barrier height h
local function V(x)
    local u = x*x/(a*a) - 1
    return h*u*u
end

local g = grid(-10, 10, 512)
local sys = g:schroedinger()
sys:set_potential(V)

-- Tunnelling time from the splitting of the lowest doublet
local states = sys:eigenstates(4)
local splitting = states[2][1] - states[1][1]
local T = math.pi/splitting
print(string.format("E0 = %.5f, E1 = %.5f (barrier %.1f)", states[1][1], states[2][1], h))
print(string.format("Splitting %.5f -> tunnelling time %.1f", splitting, T))
plot:levels(sys, 8, "Energy levels")

-- Gaussian matching the harmonic approximation of the left well
local omega = math.sqrt(8*h)/a
local sigma_x = 1/math.sqrt(2*omega)
local psi = g:gaussian(-a, 0, sigma_x)
local ensemble = classical_ensemble(30000, -a, 0, sigma_x, 1/(2*sigma_x), 1)
print(string.format("Energy of the packet: %.4f", sys:energy(psi)))

local frames = 240
local t_final = 2*T
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.1f", t)

    plot:wave(sys, psi, "Wave function/" .. label)
    plot:wigner(psi, "Wigner function (quantum)/" .. label, 128)
    plot:ensemble(ensemble, sys, "Phase space density (classical)/" .. label, 128)
    plot:scalar("Probability in the right well", t, "quantum", psi:probability(0, 10))
    plot:scalar("Probability in the right well", t, "classical", ensemble:probability(0, 10))

    if i < frames then
        sys:propagate(psi, t, t + t_final/frames, 0.01)
        ensemble:propagate(sys, t, t + t_final/frames, 0.02)
    end
end
