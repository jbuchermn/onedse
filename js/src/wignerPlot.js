import React from 'react';
import Plot from './plot';

export default class WignerPlot extends React.Component{
    render(){
        const getData = idx =>{
            let wigner = this.props.plots[idx].wigner;

            let x_size = wigner.data.length;
            let p_size = wigner.data[0].length;
            let { lower_x, lower_p, upper_x, upper_p } = wigner;

            let x = [];
            let p = [];
            for(let i=0; i<x_size; i++) x.push(lower_x + 1.*i/x_size*(upper_x-lower_x));
            for(let i=0; i<p_size; i++) p.push(lower_p + 1.*i/p_size*(upper_p-lower_p));

            let z = [];
            for(let p=0; p<p_size; p++){
                z.push([]);
                for(let x=0; x<x_size; x++){
                    z[z.length - 1].push(wigner.data[x][p]);
                }
            }

            return { x, p, z };
        };

        let steps = this.props.plots.map(p=>{
            let arr = p.title.split("/");
            return (arr.length>1) ? arr.slice(1).join("/") : arr[0];
        });

        let { x, p, z } = getData(0);
        let data = [{
            type: 'contour',
            x: x,
            y: p,
            z,
        }];

        let frames = [];
        for (let i=0; i<this.props.plots.length; i++) {
            let { x, p, z } = getData(i);
            let data = [{
                x: x,
                y: p,
                z,
            }];

            frames.push({
                name: steps[i],
                data
            });
        }


        let layout = {
            yaxis: {
                scaleanchor: "x"
            },
            height: 1000
        };

        return <Plot data={data} layout={layout} frames={frames} />;
    }
}
