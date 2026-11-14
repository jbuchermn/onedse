-- Measurement I: a pointer reads the position
-- Von Neumann's measurement model, solved exactly for two particles. The "system" (heavy)
-- is 70% left and 30% right. The "pointer" (light) feels a force whose direction depends
-- on which side the system is on, and starts to move - to both sides at once: they end up
-- entangled, |left>|pointer down> + |right>|pointer up>. Nothing has collapsed. But the
-- system alone is now described by a mixture, its Wigner function has lost the fringes,
-- and the weights of the mixture are the Born probabilities 0.7 and 0.3.

local p_left = 0.7
local separation = 6
local coupling = 3
local m_system, m_pointer = 50, 1

local g1 = grid(-20, 20, 256)
local g = grid2d(g1, g1)                 -- x: system, y: pointer

local sys = g:schroedinger()
sys:set_mass(m_system, m_pointer)
-- The pointer couples to "left or right", not to the precise position
sys:set_potential(function(x_system, x_pointer) return -coupling*math.tanh(x_system)*x_pointer end)

local system = g1:wavefunction(function(x)
    local a, b = x + separation/2, x - separation/2
    return complex(math.sqrt(p_left)*math.exp(-a*a/2) + math.sqrt(1 - p_left)*math.exp(-b*b/2), 0)
end)
local pointer = g1:gaussian(0, 0, 1)
local psi = g:product(system, pointer)

local frames, t_final = 60, 2.4
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)
    local rho = psi:reduced(1)
    local weights = rho:eigenvalues(2)

    plot:field(psi, "Joint wave function (x: system, y: pointer)/" .. label)
    plot:wigner(rho, "Wigner function of the system/" .. label, 128)
    plot:marginals(psi, "Densities (particle 1: system, particle 2: pointer)/" .. label)
    plot:density_matrix(rho, "Density matrix |rho(x, x')| of the system/" .. label, 128)
    plot:scalar("System: weights of the mixture", t, "largest", weights[1])
    plot:scalar("System: weights of the mixture", t, "second", weights[2])
    plot:scalar("System: weights of the mixture", t, "purity", rho:purity())

    if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.004) end
end

local weights = psi:reduced(1):eigenvalues(3)
print(string.format("Mixture describing the system: %.3f, %.3f (then %.4f)", weights[1], weights[2], weights[3]))
print(string.format("Born rule: %.3f, %.3f", p_left, 1 - p_left))
print(string.format("Pointer found below zero: %.3f", psi:probability(-20, 20, -20, 0)))
