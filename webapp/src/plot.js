import React from 'react';
import Plotly from 'plotly.js';

let __id = 0;

export default class Plot extends React.Component{
    constructor(props, context){
        super(props, context);
        this.id = __id;
        __id++;

        this.init = this.init.bind(this);
    }

    componentDidMount(){
        if(this.props.data) this.init(this.props.data);
    }

    componentWillReceiveProps(nextProps){
        if(nextProps.data) this.init(nextProps.data);
    }


    init(data){
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
