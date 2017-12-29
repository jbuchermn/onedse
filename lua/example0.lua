-- Session 0
basis = scaled_basis(orthogonal_legendre(100), -10., 10.)

wf = basis:wavefunction(function(x) return complex(math.exp(-(x-5.)*(x-5.)/2.), 0) end, 0)

function plt(wf, time)
	plot:wavefunction(wf, "Wave Function/" .. time, 200)

	rho = wf:density_operator()
	plot:wigner(rho, "Wigner Function/" .. time, 128)

	data = rho:diagonalize()
	for i, v in ipairs(data) do
		print("WaveFunction: " .. i .. ": Probability: " .. v[1])
		plot:wavefunction(v[2], "Diagonalized #" .. i .. "/" .. time, 200)
	end
 end

ham  =   basis:map_wavefunction(1, 1, function(x) return complex(1,   0) end, 0)
ham:add(0, 0, function(x) return complex(x*x, 0) end, 0)
ham:mul(complex(0, -1))

prop = ham:rk_propagator("RK4")

plt(wf, 0.)
for t=1,310 do
	prop:propagate(wf, 0.01*(t-1), 0.01*t, 0.)
    print("time=" .. t)
	plt(wf, 0.01*t)
end

  
