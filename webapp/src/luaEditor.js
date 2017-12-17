import React from 'react';

const EXAMPLE_CODE = 
    "basis = root:create_basis(\"ScaledOrthogonalChebyshev\", -10., 10., 100)\n" +
    "function coherent(x)\n" +
    "    return {re=x, im=1}\n" +
    "end\n" + 
    "print(coherent(2))\n"+
    "print(coherent(3), coherent(4))\n"+
    "wf = root:create_wavefunction(basis, coherent, 3)\n"+
    "root:plot_wavefunction(wf, \"coherent state\", 200)\n"+
    "rho = root:create_density_operator(basis)\n"+
    "rho:add_wavefunction(0.5, wf)\n"+
    "root:plot_wigner(rho, \"coherent state Wigner\", 100)\n";

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
