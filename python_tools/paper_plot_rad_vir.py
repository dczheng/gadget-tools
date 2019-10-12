#!/usr/bin/env python3

from my_work_env import *
import pandas as pd

df = pd.read_csv( sys.argv[1] )
fn_out = sys.argv[2]

vr = df['vr']
I =  df['Lum'] * 1e-7
vr = np.array( vr )
I = np.array( I )
print( vr, I )

plt.plot( vr, I, '*' )
for i in range( len(vr) ):
    a = 1.01
    b = 1.01
    if i == 25:
        a = 0.99
        b = 0.99
    plt.text( vr[i]*a, I[i]*b, r'${%i}$'%i )

plt.xlabel( r'$E_k/E_p$' )
plt.ylabel( r'$P_{\rm 1.4GHz} \, [W Hz^{-1}]$' )
plt.yscale( 'log' )
plt.savefig( fn_out )
