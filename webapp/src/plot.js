import React from 'react';
import Plotly from 'plotly.js';

let __id = 0;

export default class Plot extends React.Component{
    constructor(props, context){
        super(props, context);
        this.id = __id;
        __id++;
    }

    componentDidMount(){
        let size = 100, x = new Array(size), y = new Array(size), z = new Array(size);

        for(let i = 0; i < size; i++) {
            x[i] = y[i] = -2 * Math.PI + 4 * Math.PI * i / size;
            z[i] = new Array(size);
        }

        for(let i = 0; i < size; i++) {
            for(let j = 0; j < size; j++) {
                let r2 = x[i]*x[i] + y[j]*y[j];
                z[i][j] = Math.sin(x[i]) * Math.cos(y[j]) * Math.sin(r2) / Math.log(r2+1);
            }
        }

        var data = [{
            z: z,
            x: x,
            y: y,
            type: 'contour'
        }];

        Plotly.newPlot('plot' + this.id, data, {
            margin: { t: 0, r: 0, l: 0, b: 0 },
            xaxis: { gridcolor: 'transparent' }
        }, {
            displayModeBar: false
        });
    }
    render(){
        return <div id={"plot" + this.id}></div>;
    }
}
