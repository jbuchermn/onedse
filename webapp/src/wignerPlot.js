import React from 'react';
import Plot from './plot';

export default class WignerPlot extends React.Component{
    render(){
        let data = null;
        let layout = null;
        if(this.props.wigner){
            let x_size = this.props.wigner.data.length;
            let p_size = this.props.wigner.data[0].length;
            let { lower_x, lower_p, upper_x, upper_p } = this.props.wigner;

            let x = [];
            let p = [];
            for(let i=0; i<x_size; i++) x.push(lower_x + 1.*i/x_size*(upper_x-lower_x));
            for(let i=0; i<p_size; i++) p.push(lower_p + 1.*i/p_size*(upper_p-lower_p));

            let z = [];
            for(let p=p_size-1; p>=0; p--){
                z.push([]);
                for(let x=0; x<x_size; x++){
                    z[z.length - 1].push(this.props.wigner.data[x][p]);
                }
            }

            data = [{
                type: 'contour',
                x: x,
                y: p,
                z,
            }];

            layout = {
                yaxis: {
                    scaleanchor: "x"
                },
                height: 1000
            };
        }
        return <Plot data={data} layout={layout} />;
    }
}
