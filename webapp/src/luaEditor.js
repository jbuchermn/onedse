require('codemirror/lib/codemirror.css');
require('codemirror/theme/material.css');
require('codemirror/mode/lua/lua.js');

import React from 'react';
import {Controlled as CodeMirror} from 'react-codemirror2';

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
                <CodeMirror
                    value={this.state.lua}
                    options={{ mode: 'lua', theme: 'material', lineNumbers: true }}
                    onBeforeChange={(editor, data, value) => this.setState({lua: value})}
                    onChange={(editor, value) => {console.log('controlled', {value});}}
                />
                <button style={styles.button} onClick={()=>this.props.execute(this.state.lua)}>{this.props.working ? '...' : 'Run'}</button>
            </div>
        );
    }
}

const styles = {
    button: {
        backgroundColor: '#4CAF50',
        border: 'none',
        color: 'white',
        padding: '15px 32px',
        textAlign: 'center',
        textDecoration: 'none',
        display: 'inline-block',
        fontSize: '16px',
        margin: '4px 0',
        cursor: 'pointer',
        width: '100%'
    }
};
