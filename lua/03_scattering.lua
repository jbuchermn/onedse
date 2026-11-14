-- Scattering off a barrier
-- A wave packet hits a barrier slightly higher than its mean energy and splits into a
-- reflected and a transmitted part. The Bohmian trajectories never cross: those starting in
-- front are transmitted, those in the back turn around - without ever touching the barrier.

local V0, width = 2.2, 0.6
local x0, p0, sigma = -15, 2, 2.5       -- mean kinetic energy p0^2/2 = 2

local g = grid(-40, 40, 1024)
local sys = g:schroedinger()
sys:set_potential(function(x) return V0*math.exp(-x*x/(2*width*width)) end)
sys:set_absorber(8, 2)

local psi = g:gaussian(x0, p0, sigma)
local bohm = psi:bohmian(48, 0)

plot:momentum(psi, "Momentum distribution/before", 5)

local dt, t_final, frames = 0.01, 16, 80
local steps_per_frame = math.floor(t_final/dt/frames + 0.5)
local t = 0
for i = 0, frames do
    local label = string.format("t = %.1f", t)
    plot:wave(sys, psi, "Wave function/" .. label)
    plot:wigner(psi, "Wigner function/" .. label, 256)
    plot:scalar("Probabilities", t, "reflected", psi:probability(-30, -2))
    plot:scalar("Probabilities", t, "transmitted", psi:probability(2, 30))

    if i < frames then
        for _ = 1, steps_per_frame do
            sys:step(psi, t, dt)
            bohm:advance(psi, 1, t, dt)
            t = t + dt
        end
    end
end

plot:momentum(psi, "Momentum distribution/after", 5)
plot:trajectories(bohm, "Bohmian trajectories")
print(string.format("Reflected %.3f, transmitted %.3f", psi:probability(-30, -2), psi:probability(2, 30)))
