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
        if(this.props.data) this.init(this.props.data, this.props.layout, this.props.frames);
    }

    componentWillReceiveProps(nextProps){
        if(nextProps.data) this.init(nextProps.data, nextProps.layout, nextProps.frames);
    }


    init(data, layout, frames){
        if(frames){
            layout = {
                updatemenus: [{
                    x: 0,
                    y: 0,
                    yanchor: 'top',
                    xanchor: 'left',
                    showactive: false,
                    direction: 'left',
                    type: 'buttons',
                    pad: {t: 87, r: 10},
                    buttons: [{
                        method: 'animate',
                        args: [null, {
                            mode: 'immediate',
                            fromcurrent: true,
                            transition: {duration: 300},
                            frame: {duration: 500, redraw: true}
                        }],
                        label: 'Play'
                    }, {
                        method: 'animate',
                        args: [[null], {
                            mode: 'immediate',
                            transition: {duration: 0},
                            frame: {duration: 0, redraw: true}
                        }],
                        label: 'Pause'
                    }]
                }],

                sliders: [{
                    pad: {l: 130, t: 55},
                    currentvalue: {
                        visible: true,
                        xanchor: 'right',
                        font: {size: 20, color: '#666'}
                    },
                    steps: frames.map(frame => ({
                        method: 'animate',
                        label: frame.name,
                        args: [[frame.name], {
                            mode: 'immediate',
                            transition: {duration: 300},
                            frame: {duration: 300, redraw: true},
                        }]
                    }))
                }],

                ...layout
            };
        }

        Plotly.newPlot('plot' + this.id, { data, layout, frames });
    }

    render(){
        return <div id={"plot" + this.id}></div>;
    }
}
