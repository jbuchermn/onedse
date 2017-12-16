import React from 'react';
import Plot from './plot';

export default class WignerPlot extends React.Component{
    render(){
        let data = null;
        if(this.props.wigner){
            let x_size = this.props.wigner.data.length;
            let p_size = this.props.wigner.data[0].length;
            let { lower_x, lower_p, upper_x, upper_p } = this.props.wigner;

            let x = [];
            let y = [];
            for(let i=0; i<x_size; i++) x.push(lower_x + 1.*i/x_size*(upper_x-lower_x));
            for(let i=0; i<p_size; i++) y.push(lower_p + 1.*i/p_size*(upper_p-lower_p));

            let z = this.props.wigner.data;

            data = [{
                type: 'contour',
                x,
                y,
                z,
            }];
        }
        return <Plot data={data} />;
    }
}
