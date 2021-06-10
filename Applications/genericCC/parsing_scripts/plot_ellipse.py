# File copied gratefully from: https://github.com/joferkington/oost_paper_code/blob/master/error_ellipse.py

import numpy as np

import matplotlib.pyplot as plt
from matplotlib.patches import Ellipse

def plot_point_cov(points, weights=None, nstd=2, ax=None, **kwargs):
    """
    Plots an `nstd` sigma ellipse based on the mean and covariance of a point
    "cloud" (points, an Nx2 array).

    Parameters
    ----------
        points : An Nx2 array of the data points.
        nstd : The radius of the ellipse in numbers of standard deviations.
            Defaults to 2 standard deviations.
        ax : The axis that the ellipse will be plotted on. Defaults to the 
            current axis.
        weights : in case mean and covariance are to be weighted, A Nx1 array
        
        Additional keyword arguments are passed on to the ellipse patch.

    Returns
    -------
        A matplotlib ellipse artist
    """
    #weights = None
    if weights is None:
        pos = points.mean(axis=0)
        cov = np.cov(points, rowvar=False)
    else:
        pos = [0.0, 0.0]
        cov = [[0.0, 0.0], [0.0, 0.0]]
        pos_denom = [0.0, 0.0]
        cov_denom = [[0.0, 0.0], [0.0, 0.0]]
        for pt, wt in zip(points, weights):
            pos[0], pos[1] = pos[0] + pt[0]*wt[0], pos[1] + pt[1]*wt[1]
            pos_denom[0] += wt[0]
            pos_denom[1] += wt[1]
        pos[0] /= pos_denom[0]
        pos[1] /= pos_denom[1]

        for pt, wt in zip(points, weights):
            cov[0][0] += (wt[0]*(pt[0] - pos[0]))**2
            cov[1][1] += (wt[1]*(pt[1] - pos[1]))**2
            cov[0][1] += wt[0]*wt[1]*(pt[0] - pos[0])*(pt[1] - pos[1])

            cov_denom[0][0] += wt[0]**2
            cov_denom[1][1] += wt[1]**2
            cov_denom[0][1] += wt[0]*wt[1]
        cov_denom[1][0] = cov_denom[0][1]
        cov[1][1] /= cov_denom[1][1]
        cov[0][0] /= cov_denom[0][0]
        cov[0][1] /= cov_denom[0][1]

        cov[1][0] = cov[0][1]
    return plot_cov_ellipse(cov, pos, nstd, ax, **kwargs)

def plot_cov_ellipse(cov, pos, nstd=2, ax=None, **kwargs):
    """
    Plots an `nstd` sigma error ellipse based on the specified covariance
    matrix (`cov`). Additional keyword arguments are passed on to the 
    ellipse patch artist.

    Parameters
    ----------
        cov : The 2x2 covariance matrix to base the ellipse on
        pos : The location of the center of the ellipse. Expects a 2-element
            sequence of [x0, y0].
        nstd : The radius of the ellipse in numbers of standard deviations.
            Defaults to 2 standard deviations.
        ax : The axis that the ellipse will be plotted on. Defaults to the 
            current axis.
        Additional keyword arguments are pass on to the ellipse patch.

    Returns
    -------
        A matplotlib ellipse artist
    """
    def eigsorted(cov):
        vals, vecs = np.linalg.eigh(cov)
        order = vals.argsort()[::-1]
        return vals[order], vecs[:,order]

    if ax is None:
        ax = plt.gca()

    vals, vecs = eigsorted(cov)
    theta = np.degrees(np.arctan2(*vecs[:,0][::-1]))

    # Width and height are "full" widths, not radius
    width, height = 2 * nstd * np.sqrt(vals)
    ellip = Ellipse(xy=pos, width=width, height=height, angle=theta, **kwargs)

    ax.add_artist(ellip)
    return ellip

if __name__ == '__main__':
    #-- Example usage -----------------------
    # Generate some random, correlated data
    points = np.random.multivariate_normal(
            mean=(1,1), cov=[[0.4, 9],[9, 10]], size=1000
            )
    # Plot the raw points...
    x, y = points.T
    plt.plot(x, y, 'ro')

    # Plot a transparent 3 standard deviation covariance ellipse
    plot_point_cov(points, nstd=3, alpha=0.5, color='green')

    plt.show()