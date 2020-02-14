basis = scaled_basis(orthogonal_chebyshev(100), -10., 10.)

wf = basis:wavefunction(function(x) return complex(math.exp(-(x-5.)*(x-5.)/2.), 0) end, 0)

function plt(wf, time)
	plot:wavefunction(wf, "Wave Function/" .. time, 200)

	rho = wf:density_operator()
	plot:wigner(rho, "Wigner Function/" .. time, 128)
 end

plt(wf, 0.0)

ham = basis:map_wavefunction()
ham:add_term(1, 1, function(x) return complex(1, 0) end, 0)
ham:add_term(0, 0, function(x) return complex(x*x, 0) end, 0)
-- ham:imul(complex(0, -1))

prop = ham:rk_propagator("RK4")

t=0
t_step = math.pi/100.
t_final = math.pi/50.
while t<t_final do
   	print(t .. ":\n  Norm = " .. tostring(wf:norm()))
    plt(wf, t)

  	t_next = t + t_step
  	if t_next>t_final then t_next = t_final end
	prop:propagate(wf, t, t_next, 0.)
	wf:validate()
  
  	t = t_next
end

plt(wf, t)

  
