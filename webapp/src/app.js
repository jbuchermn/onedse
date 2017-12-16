import React from 'react';
import Server from './server';
import WignerPlot from './wignerPlot';
import WavefunctionPlot from './wavefunctionPlot';

export default class App extends React.Component{
    constructor(props, context){
        super(props, context);
        this.state = {
            definition: null,
            wigner: null,
            wavefunctions: null
        };

        this.server = new Server();

        this.initialize = this.initialize.bind(this);
        this.get_wigner = this.get_wigner.bind(this);
        this.get_wavefunctions = this.get_wavefunctions.bind(this);
    }

    componentDidMount(){
        this.initialize({basis: '', initial_state: ''}).then(res=>{
            return this.get_wigner({points: 100});
        }).then(res=>{
            return this.get_wavefunctions({points: 400});
        });
    }

    initialize(data){
        return this.server.call('initialize', data).then(res=>{
            if(res.data && res.data.state === "OK") this.setState({ definition: data });
        });
    }

    get_wigner(data){
        return this.server.call('get_wigner', data).then(res=>{
            if(res.data && res.data.state === "OK") this.setState({ wigner: res.data.wigner });
        });
    }

    get_wavefunctions(data){
        return this.server.call('get_wavefunctions', data).then(res=>{
            if(res.data && res.data.state === "OK") this.setState({ wavefunctions: res.data.wavefunctions });
        });
    }

    render(){
        return (
            <div>
                <h1>Wigner</h1>
                <WignerPlot wigner={this.state.wigner} />
                {this.state.wavefunctions && this.state.wavefunctions.map(({probability, wavefunction})=>(
                    <div>
                        <h2>Wavefunction with probability {probability}</h2>
                        <WavefunctionPlot wavefunction={wavefunction} />
                    </div>
                ))}
            </div>
        );
    }
}
