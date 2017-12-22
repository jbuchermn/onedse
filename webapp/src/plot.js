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
        if(this.props.data) this.init(this.props.data, this.props.layout);
    }

    componentWillReceiveProps(nextProps){
        if(nextProps.data) this.init(nextProps.data, nextProps.layout);
    }


    init(data, layout){
        Plotly.newPlot('plot' + this.id, data, layout || {});
    }

    render(){
        return <div id={"plot" + this.id}></div>;
    }
}
