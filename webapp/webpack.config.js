/* global require __dirname module */
var webpack = require('webpack');
var path = require('path');

var BUILD_DIR = path.resolve(__dirname, 'www/');
var APP_DIR = path.resolve(__dirname, 'src/');

var config = {
    entry: [APP_DIR + '/index.js'],
    output: {
        path: BUILD_DIR,
        filename: 'bundle.js'
    },
    devtool: 'source-map',
    module: {
        loaders: [
            {
                test: /\.js$/,
                exclude: /node_modules/,
                loader: 'babel-loader',
            },
            {
                /* Required by Plotly */
                test: /\.js$/,
                loader: 'ify-loader'
            }
        ]
    }
};

module.exports = config;
