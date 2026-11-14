-- Dirac equation: Zitterbewegung
-- Relativistic quantum mechanics in 1+1 dimensions (c = m = 1). A wave packet that
-- occupies only the upper spinor component mixes positive and negative energy solutions:
-- their interference makes velocity and position tremble at frequency 2E. After projecting
-- onto positive energies the trembling is gone and the packet moves at v = p/E < c.

local p0, sigma = 1, 1

local g = grid(-40, 40, 1024)
local dirac = g:dirac()

local packet = g:gaussian(-10, p0, sigma)
local zero = packet:mul(complex(0, 0))

local mixed = spinor(packet, zero)
local positive = spinor(packet, zero)
dirac:project(positive, 1)
print(string.format("Positive energy fraction: %.3f (mixed), %.3f (projected)",
    dirac:positive_energy_fraction(mixed), dirac:positive_energy_fraction(positive)))
print(string.format("Expected velocity p/E = %.3f", p0/math.sqrt(1 + p0*p0)))

local frames, t_final = 300, 15
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)

    plot:spinor(dirac, mixed, "Mixed energies/" .. label)
    plot:spinor(dirac, positive, "Positive energies only/" .. label)
    plot:scalar("Velocity <c sigma_x>", t, "mixed", dirac:velocity(mixed))
    plot:scalar("Velocity <c sigma_x>", t, "positive energies", dirac:velocity(positive))
    plot:scalar("Position <x>", t, "mixed", mixed:expect_x())
    plot:scalar("Position <x>", t, "positive energies", positive:expect_x())

    if i < frames then
        dirac:propagate(mixed, t, t + t_final/frames, 0.01)
        dirac:propagate(positive, t, t + t_final/frames, 0.01)
    end
end
