-- Tunnelling disappears as hbar goes to zero
-- A packet in the left well of a double well, below the barrier. Classically it stays
-- there. Quantum mechanically it tunnels, but the rate depends exponentially on 1/hbar:
-- a modest reduction of hbar makes the tunnelling time grow by orders of magnitude. This
-- is why footballs do not tunnel.

local a, h = 1.8, 2.0
local function V(x)
    local u = x*x/(a*a) - 1
    return h*u*u
end
local omega = math.sqrt(8*h)/a           -- frequency at the bottom of the wells

local frames, t_final, dt = 120, 90, 0.01

for _, hbar in ipairs({ 1, 0.75, 0.5 }) do
    local g = grid_hbar(-6, 6, 512, hbar)
    local sys = g:schroedinger()
    sys:set_potential(V)

    local states = sys:eigenstates(2)
    local splitting = states[2][1] - states[1][1]
    print(string.format("hbar = %.2f: ground state %.4f, splitting %.2e, tunnelling time %.0f",
        hbar, states[1][1], splitting, math.pi*hbar/splitting))

    local sigma_x = math.sqrt(hbar/(2*omega))
    local psi = g:gaussian(-a, 0, sigma_x)
    local ensemble = classical_ensemble(20000, -a, 0, sigma_x, hbar/(2*sigma_x), 3)

    local name = string.format("hbar = %g", hbar)
    for i = 0, frames do
        local t = i*t_final/frames
        local label = string.format("t = %.1f", t)

        plot:wave(sys, psi, "Wave function, " .. name .. "/" .. label)
        if hbar ~= 0.75 then
            plot:wigner_p(psi, "Wigner function, " .. name .. "/" .. label, 128, 4)
        end
        plot:scalar("Probability in the right well: quantum", t, name, psi:probability(0, 6))
        plot:scalar("Probability in the right well: classical", t, name, ensemble:probability(0, 6))

        if i < frames then
            sys:propagate(psi, t, t + t_final/frames, dt)
            ensemble:propagate(sys, t, t + t_final/frames, 2*dt)
        end
    end
end
