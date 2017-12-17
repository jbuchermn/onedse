import React from 'react';
import Server from './server';
import LuaEditor from './luaEditor';
import Message from './message';
import WignerPlot from './wignerPlot';
import WavefunctionPlot from './wavefunctionPlot';


export default class App extends React.Component{
    constructor(props, context){
        super(props, context);
        this.state = {
            stdout: "",
            plots: []
        };

        this.server = new Server();

        this.execute = this.execute.bind(this);
    }

    execute(lua){
        this.server.call(lua).then(res=>{
            this.setState({ plots: (res.data && res.data.plots) ? res.data.plots : [] });
            this.setState({ stdout: (res.data && res.data.stdout) ? res.data.stdout : "" });
            this.setState({ error: res.error ? res.error : "" });
        });
    }

    render(){
        return (
            <div>
                <h1>Wigner</h1>
                <LuaEditor execute={this.execute} />
                <Message msg={this.state.error} />
                <Message msg={this.state.stdout} />
                {this.state.plots.map(plot=>(
                    <div>
                        <h1>{plot.title}</h1>
                        {plot.wigner && <WignerPlot wigner={plot.wigner} />}
                        {plot.wavefunction && <WavefunctionPlot wavefunction={plot.wavefunction} />}
                    </div>
                ))}
            </div>
        );
    }
}
