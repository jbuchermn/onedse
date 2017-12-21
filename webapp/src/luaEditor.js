import React from 'react';

const EXAMPLE_CODE = 
    "basis = root:create_basis(\"Legendre\", -10., 10., 100)\n"+
    "\n"+
    "function coherent(x)\n"+
    "    return complex(math.exp(-x*x), 0)\n"+
    "end\n"+
    "\n"+
    "wf = root:create_wavefunction(basis, coherent, 3)\n"+
    "root:plot_wavefunction(wf, \"t=0\", 200)\n"+
    "\n"+
    "op = root:create_operator_wavefunction(basis, 1, 1, function(x) return complex(1,0) end, 0)\n"+
    "prop = root:create_wavefunction_propagator(\"Diagonal\", op)\n"+
    "\n"+
    "step = 0.001\n"+
    "prop:step(wf, 0.001, step)\n"+
    "\n"+
    "root:plot_wavefunction(wf, \"t=1.e-3\", 200)\n";

export default class LuaEditor extends React.Component{
    constructor(props, context){
        super(props, context);
        this.state = {lua: EXAMPLE_CODE};
    }

    render(){
        return (
            <div>
                <textarea value={this.state.lua} onChange={(evt) => this.setState({lua: evt.target.value})} />
                <button onClick={()=>this.props.execute(this.state.lua)}>Execute</button>
            </div>
        );
    }
}
