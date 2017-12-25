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
        this.call = this.call.bind(this);
    }

    execute(lua){
        this.setState({ working: true });
        this.server.call("execute", lua).then(res=>{
            this.setState({ working: false });
            this.setState({ plots: (res.data && res.data.plots) ? res.data.plots : [] });
            this.setState({ stdout: (res.data && res.data.stdout) ? res.data.stdout : "" });
            this.setState({ error: res.error ? res.error : "" });
        }).catch(err=>{
            this.setState({ working: false, error: JSON.stringify(err), stdout: '', plots: [] });
        });
    }

    call(command){
        return this.server.call(command, "");
    }

    render(){
        const basename = name => name.split("/")[0];

        let plots = [];
        for(let p of this.state.plots){
            let newPlot = true;
            for(let q of plots){
                if(basename(p.title) === basename(q.title)){
                    newPlot = false;
                    q.plots.push(p);
                }
            }
            if(newPlot){
                plots.push({ title: basename(p.title), kind: p.wigner ? 'wigner' : (p.wavefunction ? 'wavefunction' : 'unknown'), plots: [p] });
            }
        }

        return (
            <div style={styles.container}>
                <h1 style={styles.heading} align="center">1D Schrödinger QM in Wigner picture</h1>
                <LuaEditor call={this.call} execute={this.execute} working={this.state.working} />
                <Message msg={this.state.error} />
                <Message msg={this.state.stdout} />
                {plots.map(plot=>(
                    <div>
                        <h1 align="center">{plot.title}</h1>
                        {plot.kind === 'wigner' && <WignerPlot plots={plot.plots} />}
                        {plot.kind === 'wavefunction' && <WavefunctionPlot plots={plot.plots} />}
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
