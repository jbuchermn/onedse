-- Solitons: the Gross-Pitaevskii equation
-- Beyond linear quantum mechanics: a Bose-Einstein condensate with attractive interaction
-- obeys i dpsi/dt = [p^2/2 + g |psi|^2] psi with g < 0. The nonlinearity balances
-- dispersion and two wave packets pass through each other unchanged. The same packets
-- without interaction (g = 0) simply spread.

local g_int = -4                        -- interaction strength, try 0 or +4
local x0, v = 10, 1

local g = grid(-30, 30, 1024)

local function two_packets()
    -- Each packet carries half the norm: bright soliton sech(kappa x) with kappa = |g|/4
    local kappa = 1
    return g:wavefunction(function(x)
        local left  = 1/math.cosh(kappa*(x + x0))
        local right = 1/math.cosh(kappa*(x - x0))
        return complex(left*math.cos(v*x) + right*math.cos(v*x), left*math.sin(v*x) - right*math.sin(v*x))
    end)
end

local runs = {
    { name = "interacting (g = " .. g_int .. ")", g = g_int },
    { name = "free (g = 0)", g = 0 },
}

local frames, t_final = 200, 20
for _, run in ipairs(runs) do
    local sys = g:schroedinger()
    sys:set_nonlinearity(run.g)
    local psi = two_packets()
    local carpet = spacetime(512)
    local e0 = sys:energy(psi)

    for i = 0, frames do
        local t = i*t_final/frames
        plot:wavefunction(psi, "Wave function, " .. run.name .. string.format("/t = %.2f", t), 0)
        carpet:add(psi, t)
        if i < frames then sys:propagate(psi, t, t + t_final/frames, 0.002) end
    end

    plot:carpet(carpet, "Density over x and t, " .. run.name)
    print(string.format("%s: energy %.6f -> %.6f, norm %.10f", run.name, e0, sys:energy(psi), psi:norm()))
end
