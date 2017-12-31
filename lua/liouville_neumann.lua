basis = orthogonal_hermite(50)

wf =    basis:wavefunction(function(x) return complex(math.exp(-(x-5.)*(x-5.)/2.), 0) end, 0)
wf:iadd(basis:wavefunction(function(x) return complex(math.exp(-(x+5.)*(x+5.)/2.), 0) end, 0))
wf:idiv(complex(wf:norm(), 0))
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
ham:imul(complex(0, -1))

prop = ham:rk_propagator("RK4")

t=0
t_step = math.pi/100.
t_final = math.pi
while t<t_final do
  	print(t .. ":\n  Trace = " .. tostring(rho:trace()))
	plot:wigner(rho, "Wigner/" .. t, 128)
    data = rho:diagonalize()

    print("  Probabilities: ")
	for i, v in ipairs(data) do
		print("    " .. v[1])
        if(v[1] > 0.01) then
		    plot:wavefunction(v[2], "Diagonalized #" .. i .. "/" .. t, 200)
        end
	end
  
  	t_next = t + t_step
  	if t_next>t_final then t_next = t_final end
	prop:propagate_const_step(rho, t, t_next, 1.e-5)
	rho:validate()
  
  	t = t_next
end
plot:wigner(rho, "Wigner/" .. t, 128)

  
