import React from 'react';
import Plot from './plot';

export default class WavefunctionPlot extends React.Component{
    render(){
        const getData = idx =>{
            let wavefunction = this.props.plots[idx].wavefunction;

            let x_size = wavefunction.data.length;
            let { lower, upper } = wavefunction;

            let x = [];
            for(let i=0; i<x_size; i++) x.push(lower + 1.*i/x_size*(upper-lower));

            let re = wavefunction.data.map(d=>d[0]);
            let im = wavefunction.data.map(d=>d[1]);
            let normsq = wavefunction.data.map(d=>(d[0]*d[0] + d[1]*d[1]));

            return { x, re, im, normsq };
        };

        let steps = this.props.plots.map(p=>{
            let arr = p.title.split("/");
            return (arr.length>1) ? arr.slice(1).join("/") : arr[0];
        });
        
        let {x, re, im, normsq} = getData(0);
        let data = [{
            type: 'scatter',
            x,
            y: re,
            name: 'Re'
        },{
            type: 'scatter',
            x,
            y: im,
            name: 'Im'
        },{
            type: 'scatter',
            x,
            y: normsq,
            name: 'Norm^2'
        }];

        let frames = [];
        for (let i=0; i<this.props.plots.length; i++) {
            let {x, re, im, normsq} = getData(i);
            let data = [{
                x,
                y: re,
            },{
                x,
                y: im,
            },{
                x,
                y: normsq,
            }];

            frames.push({
                name: steps[i],
                data
            });
        }

        return <Plot data={data} layout={{}} frames={frames} />;
    }

}
