import React from 'react';
import Plot from './plot';

export default class WavefunctionPlot extends React.Component{
    render(){
        let data = null;
        if(this.props.wavefunction){
            let x_size = this.props.wavefunction.data.length;
            let { lower, upper } = this.props.wavefunction;

            let x = [];
            for(let i=0; i<x_size; i++) x.push(lower + 1.*i/x_size*(upper-lower));

            let re = this.props.wavefunction.data.map(d=>d[0]);
            let im = this.props.wavefunction.data.map(d=>d[1]);
            let normsq = this.props.wavefunction.data.map(d=>(d[0]*d[0] + d[1]*d[1]));

            data = [{
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
        }
        return <Plot data={data} />;

    }
}
