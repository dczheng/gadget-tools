#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
from scipy.integrate import quad
import scipy.special as ss

e_e = 4.8032e-10
m_e = 9.10953e-28
LS  = 2.9979e10


def K( x ):
    return ss.kv( 5.0/3.0, x )

def F( x ):
    r = quad( K, x, np.inf )
    #print( r )
    return x * r[0]

def generate_F_x_array( xmin, xmax, N ):

    xa = np.linspace( np.log10( xmin ), np.log10( xmax ), N )
    xa = np.power( 10, xa )

    Fa = []

    for xx in xa:
        Fa.append( F(xx) )

    Fa = np.array( Fa )
    return ( xa, Fa )



def plot_F():

    xa, Fa = generate_F_x_array( 1e-5, 10, 300 )

    x0 = xa[ Fa == Fa.max() ]

    plt.plot( xa, Fa )

    plt.axvline( x=x0, linestyle='-.', label="x=%.2f"%x0 )
    plt.legend()
    plt.show()

def FF( x, xa, Fa ):

    if x < xa[0]:
        return 0

    if x >= xa[-1]:
        return 0

    n = len(xa)
    dlogx = np.log10( xa[-1] / xa[0] ) / ( n - 1 )
    r = np.log10( x / xa[0] ) / dlogx
    ri = int( r )
    r = r - ri

    return Fa[ri] * ( 1-r ) + Fa[ri+1] * r

def test_FF():


    N = 300
    xmin = 1e-5
    xmax = 10

    xa, Fa = generate_F_x_array( xmin, xmax, N)

    NN = 100
    dx = np.log10( xmax/xmin ) / N

    err_max = -1;
    for i in range( NN ):
        x = np.power( 10, i * dx + np.log10( xmin ) )
        f = F( x )
        ff = FF( x, xa, Fa )

        err = np.abs( ( f-ff )/f )
        print( "x: %g, f: %g, ff: %g, err: %g"%(x, f, ff, err))

        if ( err > err_max ):
            err_max = err

    print( "err_max: ", err_max )

def df( alpha, pmin, pmax, p ):
    if p<pmin or p>pmax:
        return 0
    return np.power( p, -alpha )

def P_inte( nu, B, alpha, pmin, pmax, p ):

    fac = B * 3.0 / 16.0 * e_e / m_e / LS

    #print (fac)

    f = lambda p: df( alpha, pmin, pmax, p )

    nu_c = lambda p: fac * ( 1+p*p )

    #print( "nu: %g"%nu )
    #print( "nu_c: %g"%nu_c( p ) )

    if ( f(p) == 0 ):
        return 0

    return f( p ) * F( nu/nu_c(p) )

def test_P_inte():

    pmin = 1
    alpha = 2.4
    nu = 1e7
    nu_a = np.array( [1e6, 1e7, 1e8] )
    B = 1e-6
    pmax_a = np.array( [2.7, 3, 4] )
    pmax_a = np.power( 10, pmax_a )

    x = np.linspace( -2, 5, 200 )
    x = np.power( 10, x )

    for nu in nu_a:
        for pmax in pmax_a:
            y = []
            f = lambda p: P_inte( nu, B, alpha, pmin, pmax, p )

            for xx in x:
                y.append( f(xx) )

            y = np.array( y )
            y[ y<y.max()*1e-5 ] = 0
            plt.loglog( x, y, label=r'$p_{max}=%.1e,\nu=%.1e$'%(pmax, nu) )
            plt.loglog( x, y, '.' )

    plt.legend()
    plt.show()


def P( nu, B, alpha, pmin, pmax ):

    f = lambda p: P_inte( nu, B, alpha, pmin, pmax, p )

    r = quad( f, 0, np.inf )

    print( r )

    return r[0]

def test_P():

    c = 1
    pmin = 1
    alpha = 2.1
    B = 1e-6

    fac = c * 3**0.5 * e_e**3 * np.pi / 4.0 / ( m_e * LS**2 )

    print( fac )

    nu = np.linspace( 6, 7, 30 )
    nu = np.power( 10, nu )

    pmax_a = np.linspace( 4, 5, 5 )
    pmax_a = np.power( 10, pmax_a )

    for pmax in pmax_a:
        P_nu = []
        for x in nu:
            P_nu.append( P( x, B, alpha, pmin, pmax ) )

        P_nu = np.array( P_nu )
        P_nu = P_nu * fac
        plt.loglog( nu, P_nu, label=r'$p_{max}=%e$'%pmax )
        plt.loglog( nu, P_nu, '.' )

    plt.legend()
    plt.show()


def main():
    #plot_F()
    test_FF()

if __name__ == '__main__':
    main()
