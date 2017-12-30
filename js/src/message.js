import React from 'react';

/*
* Source: https://codepen.io/stevenpetryk/pen/xGdqxJ
*/
class Terminal extends React.Component{
    render() {
        let lines = this.props.children.map((line, key) => {
            if(line.type === 'kbd'){
                return (
                    <div key={key} style={styles.terminalLine}>
                        <span>&gt; </span>
                        <kbd>{line.props.children}</kbd>
                    </div>
                );
            }else if(line.type === 'pre'){
                return (
                    <div key={key} style={styles.terminalLine}>
                        <pre>{line.props.children}</pre>
                    </div>
                );
            }
        });

        return (
            <div style={styles.terminal}>
                <div style={styles.terminalOutput}>
                    {lines}
                </div>
            </div>
        );
    }
}

export default class Message extends React.Component{
    render(){
        return (
            <Terminal>
                <kbd>./onedse</kbd>
                <pre>{this.props.stdout}</pre>
                <pre>{this.props.stderr}</pre>
            </Terminal>
        );
    }
}

const styles = {
    terminal: {
        width: '100%',
        margin: '0 auto',
        marginTop: '16px',
        borderRadius: 3,
        backgroundColor: '#263238'
    },
    terminalOutput: {
        padding: 10,
        overflowY: 'scroll',
        color: '#FFFFFF',
        fontFamily: 'Source Code Pro'
    },
    terminalLine: {
        lineHeight: 1.5,
        userSelect: 'none',
        pointerEvents: 'none'
    },
};

