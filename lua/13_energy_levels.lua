-- Energy levels and eigenstates
-- Eigenstates of an arbitrary potential, drawn at the height of their energy. Here: a
-- Morse potential, the standard model of a molecular bond. Levels get denser towards the
-- dissociation limit. Edit V and run again - try a double well: 3*(x*x/4 - 1)^2

local D, a = 8, 0.5
local function V(x)
    local e = 1 - math.exp(-a*x)
    return math.min(D*e*e, 30)
end

local g = grid(-6, 30, 1024)
local sys = g:schroedinger()
sys:set_potential(V)

plot:levels(sys, 10, "Energy levels")

-- Compare with the exact Morse spectrum E_n = w (n + 1/2) - w^2 (n + 1/2)^2 / (4 D)
local w = a*math.sqrt(2*D)
for i, state in ipairs(sys:eigenstates(8)) do
    local n = i - 1
    local exact = w*(n + 0.5) - w*w*(n + 0.5)^2/(4*D)
    print(string.format("n = %d: E = %.6f (exact %.6f)", n, state[1], exact))
end

-- The ground state can also be found by imaginary time propagation
local psi = g:gaussian(0.5, 0, 1)
sys:relax(psi, 0.01, 3000)
print(string.format("Imaginary time propagation: E0 = %.6f", sys:energy(psi)))
