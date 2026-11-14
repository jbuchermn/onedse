-- When are quantum and classical dynamics the same?
-- Take a decidedly non-classical state - a cat state, with negative regions in its Wigner
-- function - and evolve the very same Wigner function in two ways: by the Schroedinger
-- equation, and by classical mechanics (as a cloud of particles, with negative weights for
-- the negative regions). In a harmonic potential the two agree forever: the fringes simply
-- rotate. Quantumness is in the state, not in the dynamics. The equations of motion differ
-- only through the third and higher derivatives of the potential - as the anharmonic
-- oscillator shows.

local g = grid(-10, 10, 512)
local points = 128

local potentials = {
    { name = "harmonic",   V = function(x) return x*x/2 end },
    { name = "anharmonic", V = function(x) return x*x/4 + x^4/32 end },
}

local frames, t_final, dt = 225, 9, 0.005
for _, potential in ipairs(potentials) do
    local sys = g:schroedinger()
    sys:set_potential(potential.V)

    local psi = g:wavefunction(function(x)
        return complex(math.exp(-(x - 3)^2/2) + math.exp(-(x + 3)^2/2), 0)
    end)
    local ensemble = psi:wigner_ensemble(300000, points, 11)

    for i = 0, frames do
        local t = i*t_final/frames
        local label = string.format("t = %.1f", t)

        plot:wigner_p(psi, "Schroedinger equation, " .. potential.name .. "/" .. label, points, 7)
        plot:ensemble_p(ensemble, sys, "Classical flow of the same Wigner function, " .. potential.name .. "/" .. label, points, 7)
        plot:scalar("Distance between the two (the offset is sampling noise)", t, potential.name, ensemble:distance(psi, points))

        if i < frames then
            sys:propagate(psi, t, t + t_final/frames, dt)
            ensemble:propagate(sys, t, t + t_final/frames, 4*dt)
        end
    end
end
