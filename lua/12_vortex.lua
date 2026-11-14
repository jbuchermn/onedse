-- A quantised vortex in a Bose-Einstein condensate
-- Gross-Pitaevskii equation in two dimensions. A phase winding of 2 pi is imprinted
-- off-centre onto a trapped condensate. The density has to vanish at the core, and the
-- vortex precesses around the trap centre. Follow the point where all colours meet.

local g_int = 150
local x_v, y_v = 1.5, 0

local gx = grid(-8, 8, 128)
local g = grid2d(gx, gx)

local sys = g:schroedinger()
sys:set_potential(function(x, y) return 0.5*(x*x + y*y) end)
sys:set_nonlinearity(g_int)

-- Find the ground state by imaginary time propagation, then imprint the vortex
local psi = g:wavefunction(function(x, y) return complex(math.exp(-(x*x + y*y)/8), 0) end)
sys:relax(psi, 0.005, 400)
print(string.format("Chemical potential scale (Thomas-Fermi): %.2f, energy: %.4f", math.sqrt(g_int/math.pi), sys:energy(psi)))

local ground = psi
psi = g:wavefunction(function(x, y)
    local dx, dy = x - x_v, y - y_v
    local r = math.sqrt(dx*dx + dy*dy)
    local core = r/math.sqrt(r*r + 0.1)      -- density dip of about one healing length
    return complex(core*dx/(r + 1e-12), core*dy/(r + 1e-12))
end)
psi = g:multiply(ground, psi)
sys:relax(psi, 0.002, 40)

local frames, t_final = 225, 9
for i = 0, frames do
    local t = i*t_final/frames
    plot:field(psi, "Condensate wave function" .. string.format("/t = %.2f", t))
    plot:scalar("Angular momentum per particle", t, "<Lz>", sys:angular_momentum(psi))
    plot:scalar("Energy", t, "E", sys:energy(psi))
    if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.002) end
end
