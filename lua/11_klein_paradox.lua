-- Dirac equation: the Klein paradox
-- A relativistic particle (c = m = 1) with energy E runs against a potential step V0.
-- For E - m < V0 < E + m it is reflected, as one would expect. If the step is made even
-- higher (V0 > E + m), the particle gets through again: inside the step it propagates as a
-- negative energy solution - mostly in the lower spinor component.

local p0 = 1.5
local E = math.sqrt(1 + p0*p0)
print(string.format("E = %.3f: reflection for %.2f < V0 < %.2f, Klein tunnelling above", E, E - 1, E + 1))

local g = grid(-50, 50, 2048)

local frames, t_final = 80, 32
for _, V0 in ipairs({ 2.2, 5.0 }) do
    local dirac = g:dirac()
    dirac:set_potential(function(x) return 0.5*V0*(1 + math.tanh(x/0.3)) end)
    dirac:set_absorber(8, 2)

    local packet = g:gaussian(-15, p0, 3)
    local psi = spinor(packet, packet:mul(complex(0, 0)))
    dirac:project(psi, 1)

    local name = string.format("V0 = %.1f", V0)
    for i = 0, frames do
        local t = i*t_final/frames
        plot:spinor(dirac, psi, "Step " .. name .. string.format("/t = %.1f", t))
        plot:scalar("Probability inside the step", t, name, psi:probability(1, 40))
        if i < frames then dirac:propagate(psi, t, t + t_final/frames, 0.01) end
    end
end
