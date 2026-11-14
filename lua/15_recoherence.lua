-- Measurement II: why it takes a large environment
-- The same measurement, but the pointer sits in a harmonic trap. It is pulled to one side
-- or the other, depending on the system - and swings back. Every time the two pointer
-- states overlap again, the "which path" information is erased and the system's coherence
-- returns: purity goes back to 1 and the fringes reappear. Decoherence by a single degree
-- of freedom is reversible. It becomes final only when the information is spread over so
-- many degrees of freedom that it never comes back (see the Lindblad examples).

local separation = 4
local coupling = 1
local m_system, m_pointer = 200, 1

local g1 = grid(-12, 12, 128)
local g = grid2d(g1, g1)                 -- x: system, y: pointer

-- Pointer oscillator, centred at coupling * x_system
local sys = g:schroedinger()
sys:set_mass(m_system, m_pointer)
sys:set_potential(function(x_system, x_pointer)
    local d = x_pointer - coupling*x_system
    return 0.5*d*d
end)

local system = g1:wavefunction(function(x)
    local a, b = x + separation/2, x - separation/2
    return complex(math.exp(-a*a/2) + math.exp(-b*b/2), 0)
end)
local pointer = g1:gaussian(0, 0, 1/math.sqrt(2))
local psi = g:product(system, pointer)

local frames, t_final = 300, 4*math.pi
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)
    local rho = psi:reduced(1)

    plot:field(psi, "Joint wave function (x: system, y: pointer)/" .. label)
    plot:wigner(rho, "Wigner function of the system/" .. label, 64)
    plot:scalar("System: purity and coherence", t, "purity", rho:purity())
    plot:scalar("System: purity and coherence", t, "coherence |rho(left, right)| / initial", rho:coherence(-separation/2, separation/2)*2*math.sqrt(math.pi))
    plot:marginals(psi, "Densities (particle 1: system, particle 2: pointer)/" .. label)

    if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.005) end
end
