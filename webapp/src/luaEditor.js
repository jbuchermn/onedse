require('codemirror/lib/codemirror.css');
require('codemirror/theme/material.css');
require('codemirror/mode/lua/lua.js');

import React from 'react';
import {Controlled as CodeMirror} from 'react-codemirror2';

export default class LuaEditor extends React.Component{
    constructor(props, context){
        super(props, context);
        this.state = {lua: ""};
    }

    componentDidMount(){
        this.props.call("get").then(res => this.setState({ lua: res.data ? res.data.lua : "" }));
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
