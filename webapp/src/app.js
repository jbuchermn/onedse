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
            plots: [],
            working: false
        };

        this.server = new Server();

        this.execute = this.execute.bind(this);
    }

    execute(lua){
        this.setState({ working: true });
        this.server.call(lua).then(res=>{
            this.setState({ working: false });
            this.setState({ plots: (res.data && res.data.plots) ? res.data.plots : [] });
            this.setState({ stdout: (res.data && res.data.stdout) ? res.data.stdout : "" });
            this.setState({ error: res.error ? res.error : "" });
        }).catch(err=>{
            this.setState({ working: false, error: JSON.stringify(err), stdout: '', plots: [] });
        });
    }

    render(){
        return (
            <div style={styles.container}>
                <h1 style={styles.heading} align="center">1D Schrödinger QM in Wigner picture</h1>
                <LuaEditor execute={this.execute} working={this.state.working} />
                <Message msg={this.state.error} />
                <Message msg={this.state.stdout} />
                {this.state.plots.map(plot=>(
                    <div>
                        <h1 align="center">{plot.title}</h1>
                        {plot.wigner && <WignerPlot wigner={plot.wigner} />}
                        {plot.wavefunction && <WavefunctionPlot wavefunction={plot.wavefunction} />}
                    </div>
                ))}
            </div>
        );
    }
}

const styles = {
    heading: {
        marginTop: '50px',
        marginBottom: '40px'
    },
    container: {
        maxWidth: '1000px',
        margin: '0 auto',
        padding: '1em',
    }
};
