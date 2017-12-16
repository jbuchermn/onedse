import React from 'react';
import Server from './server';
import WignerPlot from './wignerPlot';
import WavefunctionPlot from './wavefunctionPlot';

const lua = 
    "basis = root:create_basis(\"ScaledOrthogonalChebyshev\", -10., 10., 100)\n" +
    "function coherent(x)\n" +
    "    return {re=x, im=1}\n" +
    "end\n" + 
    "wf = root:create_wavefunction(basis, coherent, 3)\n";

export default class App extends React.Component{
    constructor(props, context){
        super(props, context);
        this.state = {
            definition: null,
            wigner: null,
            wavefunctions: null
        };

        this.server = new Server();
    }

    componentDidMount(){
        this.server.call(lua);
    }

    render(){
        return (
            <div>
                <h1>Wigner</h1>
            </div>
        );
    }
}
