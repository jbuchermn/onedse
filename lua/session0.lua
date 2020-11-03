-- Basis and initial state
basis = orthogonal_hermite(30)

-- Superposition of two Gaussians
-- wf = basis:wavefunction(
--   	function(x) 
--     	return complex(math.exp(-(x-5.)*(x-5.)/2.) + math.exp(-(x+5.)*(x+5.)/2.), 0) 
-- 	end, 
--     0
-- )

-- Fourth eigenstate of harmonic oscillator
wf = basis:wavefunction(
    function(x)
        return complex(math.exp(-x*x/2.)*(8*x*x*x - 12*x), 0)
    end,
    0
)


wf:idiv(complex(wf:norm(), 0))
rho = wf:density_operator()

-- We follow the notation used in https://arxiv.org/pdf/0705.2202.pdf
-- Parameters
Dpp = 0.1
Dqq = 0.1
Dpq = 0.05
lambda = 0.1
mu = 0.1

-- These are subject to Dpp>=0, Dqq>=0, Dpp*Dqq - Dpq^2 >= lambda^2/4
if(Dpp<0) then print("WARNING: Dpp<0") end
if(Dqq<0) then print("WARNING: Dqq<0") end
if(Dpp*Dqq - Dpq*Dpq < lambda*lambda/4) then print("WARNING: Dpp*Dqq - Dpq^2 < lambda^2/4") end

-- Terms: alpha_pq pq rho, ...
alpha_pp = complex(-Dqq, 0)
alpha_qq = complex(-Dpp, 0)
alpha_pq = complex(Dpq, (lambda-mu)/2.) 
alpha_qp = complex(Dpq, -(lambda+mu)/2.)

-- Terms: beta_pq rho pq, ...
beta_pp = complex(-Dqq, 0)
beta_qq = complex(-Dpp, 0)
beta_pq = complex(Dpq, (lambda+mu)/2.)
beta_qp = complex(Dpq, -(lambda-mu)/2.)

-- Terms: gamma_pq p rho q, ...
gamma_pp = complex(2*Dpp, 0)
gamma_qq = complex(2*Dqq, 0)
gamma_pq = complex(-2*Dpq, lambda)
gamma_qp = complex(-2*Dpq, -lambda)


-- Liouville-Neumann time evolution
hamiltonian = basis:map_wavefunction()
hamiltonian:add_term(1, 1, function(x) return complex(1, 0) end, 0)
hamiltonian:add_term(0, 0, function(x) return complex(x*x, 0) end, 2)

op_liouville = basis:map_density_operator()
op_liouville:add_from_wavefunction_left(hamiltonian:mul(complex(0, -1)))
op_liouville:add_from_wavefunction_right(hamiltonian:mul(complex(0, 1)))

-- alpha/beta terms
pp = basis:map_wavefunction()
pp:add_term(1, 1, function(x) return complex(1, 0) end, 0)

qq = basis:map_wavefunction()
qq:add_term(0, 0, function(x) return complex(x*x, 0) end, 2)

pq = basis:map_wavefunction()
pq:add_term(1, 0, function(x) return complex(0, x) end, 1)

qp = basis:map_wavefunction()
qp:add_term(0, 1, function(x) return complex(0, -x) end, 1)

op1 = basis:map_density_operator()
op1:add_from_wavefunction_left(pp:mul(alpha_pp))
op1:add_from_wavefunction_left(qq:mul(alpha_qq))
op1:add_from_wavefunction_left(pq:mul(alpha_pq))
op1:add_from_wavefunction_left(qp:mul(alpha_qp))
op1:add_from_wavefunction_right(pp:mul(beta_pp))
op1:add_from_wavefunction_right(qq:mul(beta_qq))
op1:add_from_wavefunction_right(pq:mul(beta_pq))
op1:add_from_wavefunction_right(qp:mul(beta_qp))

-- gamma terms
p_left = basis:map_wavefunction()
p_left:add_term(0, 1, function(x) return complex(0, -1) end, 0)

p_right = basis:map_wavefunction()
p_right:add_term(1, 0, function(x) return complex(0, 1) end, 0)

q = basis:map_wavefunction()
q:add_term(0, 0, function(x) return complex(x, 0) end, 1)

op2 = basis:map_density_operator()
op2:add_from_wavefunction_both(p_left, p_right:mul(gamma_pp))
op2:add_from_wavefunction_both(q,      q      :mul(gamma_qq))
op2:add_from_wavefunction_both(p_left, q      :mul(gamma_pq))
op2:add_from_wavefunction_both(q,      p_right:mul(gamma_qp))

-- Finish setup
op = op_liouville:add(op1):add(op2)
prop = op:diagonal_propagator()
-- prop = op:rk_propagator("RK4")

t=0
t_step = math.pi/100.
t_final = math.pi/2.
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
	prop:propagate(rho, t, t_next, 0.)
	-- prop:propagate_const_step(rho, t, t_next, 5.e-6)
	rho:validate()
  
  	t = t_next
end
plot:wigner(rho, "Wigner/" .. t, 128)