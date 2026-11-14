-- Quantum revivals and the quantum carpet
-- A wave packet in a box spreads, but since all energies are multiples of E1 it reassembles
-- exactly after T = 4 m L^2 / pi. At fractions of T it forms several copies of itself:
-- at T/4 a "cat state", clearly visible as interference fringes in the Wigner function.
-- The density over time forms the quantum carpet.

local L = 8                             -- box from -L/2 to L/2
local wall = 400

local g = grid(-6, 6, 512)
local sys = g:schroedinger()
-- Steep but smooth walls: a discontinuous potential would excite grid-scale noise
sys:set_potential(function(x) return 0.5*wall*(1 + math.tanh((math.abs(x) - L/2)/0.05)) end)

local psi = g:gaussian(-1.5, 0, 0.4)
-- Remove the (tiny) part outside the box
sys:propagate(psi, 0, 0.01, 0.001)
psi:normalize()
local psi0 = psi:copy()

local T = 4*L*L/math.pi
local carpet = spacetime(256)
print(string.format("Revival time T = %.2f", T))

local frames, dt = 240, 0.002
for i = 0, frames do
    local t = i*T/frames
    local label = string.format("t = %.3f T", t/T)

    carpet:add(psi, t/T)
    if i % 2 == 0 then
        plot:wave(sys, psi, "Wave function/" .. label)
        plot:wigner(psi, "Wigner function/" .. label, 256)
    end
    local overlap = psi0:dot(psi)
    plot:scalar("Autocorrelation |<psi(0)|psi(t)>|^2", t/T, "fidelity", overlap:get_re()^2 + overlap:get_im()^2)

    if i < frames then sys:propagate(psi, t, t + T/frames, dt) end
end

local overlap = psi0:dot(psi)
print(string.format("Fidelity after one revival time: %.4f", overlap:get_re()^2 + overlap:get_im()^2))
plot:carpet(carpet, "Quantum carpet: density over x and t/T")
