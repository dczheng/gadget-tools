#!/usr/bin/env python3

from astropy.cosmology import LambdaCDM
import astropy.constants  as ac

m_p      =                       ac.m_p.cgs.value
m_e      =                       ac.m_e.cgs.value
k_b      =                       ac.k_B.cgs.value
c        =                       ac.c.cgs.value
sigma_t  =                       ac.sigma_T.cgs.value
G        =                       ac.G.cgs.value
pc       =                       ac.pc.cgs.value
Kpc      =                       pc * 1e3
Mpc      =                       pc * 1e6
c2       =                       c * c
m_ec     =                       m_e * c
m_ec2    =                       m_e * c2
h        =                       ac.h.cgs.value
hbar     =                       ac.hbar.cgs.value
alpha    =                       ac.alpha.value
Msun     =                       ac.M_sun.cgs.value

H0       =                       3.2407789e-18  # /* in h/sec */
Bcmb     =                       (3.24e-6) # // gauss
Xh       =                       0.760
elec_frac =                      (1+Xh) / (2*Xh)
Gamma    =                       5.0 / 3.0
e        =                       4.8032e-10
e2       =                       e**2
e3       =                       e**3
Jy       =                       1e-23 # erg s^-1 cm^-2 Hz^-1
mJy       =                      Jy * 1e-3

yr       =                       3.155e7
Myr      =                       yr * 1e6
Gyr      =                       Myr * 1e6

Omega0      =                    0.302
OmegaLambda =                    0.698
OmegaBaryon =                    0.04751
HubbleParam =                    0.68

gadget_length_in_cm            =              Kpc  / HubbleParam
gadget_mass_in_g               =              1e10 * Msun / HubbleParam
gadget_velocity_in_cm_per_s    =              1e5
gadget_time_in_s               =              gadget_length_in_cm / gadget_velocity_in_cm_per_s
gadget_energy_in_erg           =              gadget_mass_in_g * gadget_length_in_cm**2 / (gadget_time_in_s**2)
gadget_density                 =              gadget_mass_in_g / gadget_length_in_cm**3
gadget_gauss                   =              gadget_mass_in_g / ( gadget_length_in_cm * gadget_time_in_s * gadget_time_in_s )

cosmology = LambdaCDM( H0 = HubbleParam*100, Om0=Omega0, Ode0 = OmegaLambda )

D_c = lambda z: cosmology.comoving_distance( z ).cgs.value
D_a = lambda z: cosmology.angular_diameter_distance( z ).cgs.value
D_l = lambda z: cosmology.luminosity_distance( z ).cgs.value

rho_crit = lambda z: cosmology.critical_density( z ).cgs.value
rho_crit0  = cosmology.critical_density0.cgs.value

rho_crit_in_gadget = lambda z: rho_crit( z ) / gadget_density 
rho_crit0_in_gadget = rho_crit0 / gadget_density

rho_bar = lambda z: rho_crit( z ) * OmegaBaryon
rho_bar0 = rho_crit0 * OmegaBaryon

rho_bar_in_gadget = lambda z: rho_crit_in_gadget( z ) * OmegaBaryon
rho_bar0_in_gadget = rho_crit0_in_gadget * OmegaBaryon

n_p_z = lambda z: rho_bar(z) / m_p * Xh
n_p_0 = rho_bar0 / m_p * Xh


n_e_z =  lambda z: n_p_z( z ) * elec_frac
n_e_0 = n_p_0 * elec_frac

