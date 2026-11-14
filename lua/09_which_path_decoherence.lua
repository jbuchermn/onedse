-- Decoherence from entanglement: a which-path measurement
-- Two particles in one dimension. The heavy one is in a superposition of "left" and
-- "right" (a cat state). The light one is reflected off it, at a point that depends on
-- where the heavy one is: the two become entangled. Looking at the heavy particle alone,
-- the interference fringes of its Wigner function disappear and the off-diagonal blocks of
-- its density matrix vanish. Its entropy rises to more than ln 2, since the probe's
-- wavelength resolves the position better than just left or right. No Lindblad equation
-- is involved, only the Schroedinger equation for both particles.

local separation = 6
local m_heavy, m_light = 20, 1
local p_probe = 3
local V0, range = 40, 0.5               -- hard repulsive interaction: the probe is always reflected

local g1 = grid(-20, 20, 256)
local g = grid2d(g1, g1)

local sys = g:schroedinger()
sys:set_mass(m_heavy, m_light)
sys:set_potential(function(x1, x2)
    local d = x1 - x2
    return V0*math.exp(-d*d/(2*range*range))
end)

local cat = g1:wavefunction(function(x)
    local a, b = x - separation/2, x + separation/2
    return complex(math.exp(-a*a/2) + math.exp(-b*b/2), 0)
end)
local probe = g1:gaussian(-12, p_probe, 1.5)
local psi = g:product(cat, probe)

local frames, t_final = 60, 7
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)
    local rho = psi:reduced(1)

    plot:field(psi, "Two-particle wave function (x1, x2)/" .. label)
    plot:wigner(rho, "Wigner function of the heavy particle/" .. label, 128)
    plot:density_matrix(rho, "Density matrix |rho(x, x')| of the heavy particle/" .. label, 128)
    plot:marginals(psi, "Densities/" .. label)
    plot:scalar("Heavy particle: purity and entropy", t, "purity", rho:purity())
    plot:scalar("Heavy particle: purity and entropy", t, "entropy / ln 2", rho:entropy()/math.log(2))

    if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.005) end
end

local rho = psi:reduced(1)
print(string.format("Heavy particle after the collision: purity %.3f, entropy %.3f ln 2", rho:purity(), rho:entropy()/math.log(2)))
print("Its largest eigenvalues (probabilities of the mixture):")
for i, p in ipairs(rho:eigenvalues(4)) do print(string.format("  %.4f", p)) end
