-- Session 0
basis = scaled_basis(orthogonal_legendre(100), -10., 10.)

wf =   basis:wavefunction(function(x) return complex(math.exp(-(x-5.)*(x-5.)/2.), 0) end, 0)
wf:add(basis:wavefunction(function(x) return complex(math.exp(-(x+5.)*(x+5.)/2.), 0) end, 0))
rho = wf:density_operator()

function plt(rho, time)
	plot:wigner(rho, "Wigner Function/" .. time, 128)

	data = rho:diagonalize()
	for i, v in ipairs(data) do
		plot:wavefunction(v[2], "Diagonalized #" .. i .. "/" .. time, 200)
	end
 end

ham  =   basis:map_density_operator()
ham:add_left(1, 1, function(x) return complex(1,   0) end, 0)
ham:add_left(0, 0, function(x) return complex(x*x, 0) end, 0)
ham:add_right(1, 1, function(x) return complex(-1,   0) end, 0)
ham:add_right(0, 0, function(x) return complex(-x*x, 0) end, 0)
ham:mul(complex(0, -1))

prop = ham:rk_propagator("RK4")

plt(rho, 0.)
for t=1,10 do
	prop:propagate_const_step(rho, 0.01*(t-1), 0.01*t, 1.e-5)
    print("time=" .. 0.01*t)
	plt(rho, 0.01*t)
end

  
