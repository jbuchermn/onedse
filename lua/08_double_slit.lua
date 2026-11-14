-- The double slit in two dimensions
-- A wave packet runs against a wall with two slits. Behind the wall the two partial waves
-- interfere. Brightness shows the probability density, colour the phase of the wave function.

local slit_distance, slit_width = 4, 1.2
local wall_height, wall_thickness = 60, 0.5
local p0 = 5                            -- wavelength 2 pi / p0 = 1.26

local gx = grid(-20, 20, 256)
local gy = grid(-20, 20, 256)
local g = grid2d(gx, gy)

local sys = g:schroedinger()
sys:set_potential(function(x, y)
    if math.abs(x) > wall_thickness/2 then return 0 end
    if math.abs(math.abs(y) - slit_distance/2) < slit_width/2 then return 0 end
    return wall_height
end)
sys:set_absorber(4, 4)

local packet_x = gx:gaussian(-9, p0, 2)
local packet_y = gy:gaussian(0, 0, 4)
local psi = g:product(packet_x, packet_y)

local screen_x = 10
local screen = spacetime(256)

local frames, t_final = 135, 5.4
for i = 0, frames do
    local t = i*t_final/frames
    local label = string.format("t = %.2f", t)
    plot:wave2d(sys, psi, "Wave function/" .. label)
    screen:add(psi:slice_x(screen_x), t)
    if i == 95 then
        plot:wavefunction(psi:slice_x(screen_x), string.format("Cut along the screen at x = %d, t = %.2f", screen_x, t), 0)
    end
    plot:scalar("Probability", t, "behind the wall", psi:probability(0.5, 20, -20, 20))
    plot:scalar("Probability", t, "total (rest is absorbed)", psi:norm()^2)

    if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.004) end
end

-- What a screen behind the slits records over time
plot:carpet(screen, string.format("Density arriving at the screen x = %d, over y and t", screen_x))
