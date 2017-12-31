import sys
import json

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as anim
from mpl_toolkits.axes_grid1 import make_axes_locatable


if(len(sys.argv) < 2):
    print("Usage: plot.py result/folder")
    exit(1)

working_dir = sys.argv[1] + "/"

data = None
try:
    data = json.load(open(working_dir + "data.json"))
except Exception:
    print("Could not read data")
    exit(1)

if 'plots' not in data:
    exit(0)

"""
Sort plots by basename
"""
plots = []
for d in data['plots']:
    title = d['title'].split("/")[0]
    for p in plots:
        if(p['title'] == title):
            p['plots'] += [d]
            break
    else:
        plots += [{'title': title, 'plots': [d]}]


def plot_wavefunction(plots):
    print("Plotting wavefunction \"%s\"..." % plots['title'])

    def get_data(i):
        x = np.linspace(plots['plots'][i]['wavefunction']['lower'], plots['plots'][i]['wavefunction']['upper'], len(plots['plots'][i]['wavefunction']['data']))
        y_re = [y[0] for y in plots['plots'][i]['wavefunction']['data']]
        y_im = [y[1] for y in plots['plots'][i]['wavefunction']['data']]
        y_sq = [y[0]**2 + y[1]**2 for y in plots['plots'][i]['wavefunction']['data']]
        return x, y_re, y_im, y_sq

    def plot(x, y_re, y_im, y_sq):
        fig = plt.figure()

        line_re, = plt.plot(x, y_re, label='Re')
        line_im, = plt.plot(x, y_im, label='Im')
        line_sq, = plt.plot(x, y_sq, label='Norm^2')
        plt.legend()

        return fig, line_re, line_im, line_sq

    if(len(plots['plots']) == 1):
        x, y_re, y_im, y_sq = get_data(0)
        fig, l1, l2, l3 = plot(x, y_re, y_im, y_sq)
        plt.title(plots['plots'][0]['title'])
        plt.savefig(working_dir + plots['plots'][0]['title'].replace("/", "_") + '.pdf')
        plt.close(fig)

    else:
        x, y_re, y_im, y_sq = get_data(0)
        fig, line_re, line_im, line_sq = plot(x, y_re, y_im, y_sq)

        def animate(i):
            _, y_re, y_im, y_sq = get_data(i)
            line_re.set_ydata(y_re)
            line_im.set_ydata(y_im)
            line_sq.set_ydata(y_sq)

            return line_re, line_im, line_sq

        ani = anim.FuncAnimation(fig, animate, np.arange(0, len(plots['plots']) - 1), interval=1)

        first_title = plots['plots'][0]['title']
        last_title = plots['plots'][-1]['title']
        first_time = float(first_title.split("/")[1])
        last_time = float(last_title.split("/")[1])
        plt.title('%s: %f - %f' % (plots['title'], first_time, last_time))

        Writer = anim.writers['ffmpeg']
        writer = Writer(fps=5, metadata=dict(artist='onedse'), bitrate=1800)
        ani.save(working_dir + plots['title'] + '.mp4', writer=writer)
        plt.close(fig)


def plot_wigner(plots):
    print("Plotting Wigner function \"%s\"..." % plots['title'])

    def get_data(i):
        Z = np.array(plots['plots'][i]['wigner']['data'])
        Z = np.transpose(Z)

        X = (plots['plots'][i]['wigner']['lower_x'], plots['plots'][i]['wigner']['upper_x'])
        P = (plots['plots'][i]['wigner']['lower_p'], plots['plots'][i]['wigner']['upper_p'])

        return X, P, Z

    def plot(X, P, Z):
        fig = plt.figure()
        ax = fig.add_subplot(111)

        div = make_axes_locatable(ax)
        cax = div.append_axes('right', '5%', '5%')

        # W(p,x) is always in [-1/pi, 1/pi]
        im = ax.imshow(Z, origin='lower', cmap=plt.cm.seismic, aspect='equal', extent=[*X, *P], vmin=-1.1 / np.pi, vmax=1.1 / np.pi, interpolation='bicubic')
        cb = fig.colorbar(im, cax=cax)

        return fig, ax, im, cb

    if(len(plots['plots']) == 1):
        X, P, Z = get_data(0)
        fig, ax, im, cb = plot(X, P, Z)
        ax.set_title(plots['plots'][0]['title'])
        plt.savefig(working_dir + plots['plots'][0]['title'].replace("/", "_") + '.pdf')
        plt.close(fig)

    else:
        X, P, Z = get_data(0)
        fig, ax, im, cb = plot(X, P, Z)

        def animate(i):
            X, P, Z = get_data(i)
            im.set_data(Z)

        ani = anim.FuncAnimation(fig, animate, np.arange(0, len(plots['plots']) - 1), interval=1)

        first_title = plots['plots'][0]['title']
        last_title = plots['plots'][-1]['title']
        first_time = float(first_title.split("/")[1])
        last_time = float(last_title.split("/")[1])
        ax.set_title('%s: %f - %f' % (plots['title'], first_time, last_time))

        Writer = anim.writers['ffmpeg']
        writer = Writer(fps=15, metadata=dict(artist='onedse'), bitrate=1800)
        ani.save(working_dir + plots['title'] + '.mp4', writer=writer)
        plt.close(fig)


"""
Do plot
"""
for p in plots:
    if('wigner' in p['plots'][0]):
        plot_wigner(p)
    elif('wavefunction' in p['plots'][0]):
        plot_wavefunction(p)


