-- Basis and initial state
basis = orthogonal_hermite(30)

wf = basis:wavefunction(
  	function(x) 
    	return complex(math.exp(-(x-5.)*(x-5.)/2.)/math.pi + math.exp(-(x+5.)*(x+5.)/2.)/math.pi, 0) 
	end, 0)
rho = wf:density_operator()


-- Neumann-Liouville time evolution
hamiltonian = basis:map_wavefunction()
hamiltonian:add_term(1, 1, function(x) return complex(1, 0) end, 0)
hamiltonian:add_term(0, 0, function(x) return complex(x*x, 0) end, 2)

op_liouville = basis:map_density_operator()
op_liouville:add_from_wavefunction_left(hamiltonian:mul(complex(0, -1)))
op_liouville:add_from_wavefunction_right(hamiltonian:mul(complex(0, 1)))

-- L1 = a, L2=at
gamma = .1
n = 1

gamma1 = complex(gamma/2.*(n+1), 0)
gamma2 = complex(gamma/2.*n, 0)

a = basis:map_wavefunction()
a:add_term(0, 0, function(x) return complex(x, 0) end, 1)
a:add_term(0, 1, function(x) return complex(1, 0) end, 0)

at = basis:map_wavefunction()
at:add_term(0, 0, function(x) return complex(x, 0) end, 1)
at:add_term(1, 0, function(x) return complex(1, 0) end, 0)

ata = basis:map_wavefunction()
ata:add(hamiltonian)
ata:add_term(0, 0, function(x) return complex(-1, 0) end, 0)
ata:mul(complex(.5, 0))

aat = basis:map_wavefunction()
aat:add(hamiltonian)
aat:add_term(0, 0, function(x) return complex(1, 0) end, 0)
aat:mul(complex(.5, 0))

op1 = basis:map_density_operator()
op1:add_from_wavefunction_both(a, at)
op1:add_from_wavefunction_left(ata:mul(complex(-.5, 0)))
op1:add_from_wavefunction_right(ata:mul(complex(-.5, 0)))
op1:imul(gamma1)

op2 = basis:map_density_operator()
op2:add_from_wavefunction_both(at, a)
op2:add_from_wavefunction_left(aat:mul(complex(-.5, 0)))
op2:add_from_wavefunction_right(aat:mul(complex(-.5, 0)))
op2:imul(gamma2)

-- Finish setup
op = op_liouville:add(op1):add(op2)
-- prop = op:diagonal_propagator()
prop = op:rk_propagator("RK4")

t=0
t_step = math.pi/100.
t_final = math.pi
while t<t_final do
  	print(t .. ": " .. tostring(rho:trace()))
	plot:wigner(rho, "Wigner/" .. t, 128)
    data = rho:diagonalize()
	for i, v in ipairs(data) do
		--- print("WaveFunction: " .. i .. ": Probability: " .. v[1])
		plot:wavefunction(v[2], "Diagonalized #" .. i .. "/" .. t, 200)
	end
  
  	t_next = t + t_step
  	if t_next>t_final then t_next = t_final end
	-- prop:propagate(rho, t, t_next, 0.)
	prop:propagate_const_step(rho, t, t_next, 5.e-6)
	rho:validate()
  
  	t = t_next
end
plot:wigner(rho, "Wigner/" .. t, 128)
