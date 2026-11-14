-- Measurement III: single runs and the ensemble
-- An environment that monitors position, with everything it learns kept track of. In each
-- single run (a "quantum trajectory", here a stochastic Schroedinger equation) the state
-- stays pure and drifts at random into ONE of the two lobes: an outcome. Averaged over many
-- runs, i.e. when the record is ignored, the result is exactly the density matrix of the
-- Lindblad equation: fringes gone, both lobes still there. And the fraction of runs ending
-- on the left is the Born probability, 70%.

local p_left = 0.7
local separation = 8
local kappa = 0.04                       -- measurement strength
local runs = 300
local seed = 1

local g = grid(-14, 14, 256)
local sys = g:schroedinger()
sys:set_mass(20)                         -- heavy: hardly moves on its own

local function cat()
    return g:wavefunction(function(x)
        local a, b = x + separation/2, x - separation/2
        return complex(math.sqrt(p_left)*math.exp(-a*a/2) + math.sqrt(1 - p_left)*math.exp(-b*b/2), 0)
    end)
end

local frames, t_final, dt = 50, 5, 0.01
local steps = math.floor(t_final/frames/dt + 0.5)

-- The ensemble average, one density matrix per frame
local average = {}
for i = 0, frames do average[i] = g:density_matrix() end

local left = 0
for run = 1, runs do
    local psi = cat()
    local measurement = position_measurement(kappa, seed + run)
    local t = 0

    for i = 0, frames do
        average[i]:add(1/runs, psi)

        if run == 1 then
            local label = string.format("t = %.2f", t)
            plot:wavefunction(psi, "A single run: wave function/" .. label, 0)
            plot:wigner(psi, "A single run: Wigner function/" .. label, 128)
        end
        if run <= 6 then
            plot:scalar("Probability on the left, six single runs", t, "run " .. run, psi:probability(-14, 0))
        end

        if i < frames then
            for _ = 1, steps do
                sys:step(psi, t, dt)
                measurement:apply(psi, dt)
                t = t + dt
            end
        end
    end

    if psi:probability(-14, 0) > 0.5 then left = left + 1 end
    plot:scalar("Fraction of runs ending on the left", run, "observed", left/run)
    plot:scalar("Fraction of runs ending on the left", run, "Born rule", p_left)
end

-- What is left when the record is ignored
local initial = average[0]:coherence(-separation/2, separation/2)
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)
    plot:wigner(average[i], "Average over all runs: Wigner function/" .. label, 128)
    plot:density_matrix(average[i], "Average over all runs: |rho(x, x')|/" .. label, 128)
    plot:scalar("Coherence between the lobes", t, "average over runs", average[i]:coherence(-separation/2, separation/2)/initial)
    plot:scalar("Coherence between the lobes", t, "Lindblad: exp(-kappa d^2 t / 2)", math.exp(-kappa*separation*separation*t/2))
end

print(string.format("%d of %d runs ended on the left: %.3f (Born rule: %.3f)", left, runs, left/runs, p_left))
print(string.format("Purity of the average: %.3f. Below %.3f, the value for a mixture of just two states:", average[frames]:purity(), p_left^2 + (1 - p_left)^2))
print("the environment also learns where within a lobe the particle is.")
