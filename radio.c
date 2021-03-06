#include "allvars.h"

#define TAB_F_N 1000000
#define GSL_BESSEL_UPPER_LIMIT   ((double)600)
#define GSL_BESSEL_BREAKPOINT    ((double)100)
    /*Knu function will give a very small value ( gsl_sf_bessel_Knu can not work )
     * when x > GSL_BESSEL_UPPER_LIMIT */

#define F_X_MAX              ((double)500)
#define F_X_MIN              ((double)1e-5)

double *tab_F_V, F_V_above_breakpoint, F_V_above_breakpoint_err;

double F_integrand( double x, void *params ) {
    if ( x > GSL_BESSEL_UPPER_LIMIT )
        return 0;
    return gsl_sf_bessel_Knu( 5.0/3.0, x );
}

void F0( double x, double *r, double *err ) {

    gsl_function Func;
    Func.function = &F_integrand;
    Func.params = NULL;
    //printf( "x: %g\n", x );

    if ( x < GSL_BESSEL_BREAKPOINT ) {
        gsl_integration_qag( &Func, x, GSL_BESSEL_BREAKPOINT,
            GSL_INTE_ERR_ABS, GSL_INTE_ERR_REL, GSL_INTE_WS_LEN,
            GSL_INTE_KEY,
            inte_ws, r, err );
        *r += F_V_above_breakpoint;
        if ( *err < F_V_above_breakpoint_err )
            *err = F_V_above_breakpoint_err;
    }
    else {
        gsl_integration_qagiu( &Func, x,
                GSL_INTE_ERR_ABS, GSL_INTE_ERR_REL, GSL_INTE_WS_LEN,
                inte_ws, r, err );
    }
}

void F( double x, double *r, double *err ) {
    F0( x, r, err );
    *err = *err / (*r);
    *r = *r * x;
}

void output_F_x() {

    double logx_min, logx_max, x, dlogx, F_v, err, err_max, err_mean;
    int logx_N, i;
    FILE *fd;

    logx_min = -5;
    logx_max = 1;
    logx_N = 1000;

    writelog( "output F_x\n" );

    if ( ThisTask_Local )
        return;

    dlogx = ( logx_max-logx_min ) / ( logx_N - 1 );

    fd = myfopen( "w", "F_x.dat" );

    err_max = err_mean = 0;

    for( i=0; i<logx_N; i++ ) {
        x = logx_min + i * dlogx;
        x = pow( 10, x );
        F( x, &F_v, &err );
        fprintf( fd, "%g %g %g\n", x, F_v, err );
        if ( err > err_max )
            err_max = err;

        err_mean += err;
    }

    err_mean /= logx_N;

    printf( "[compute F] err_max: %.2f%%, err_mean: %.2f%%\n",
        err_max*100, err_mean*100 );

    fclose( fd );

}

void init_compute_F() {

    gsl_function Func;
    Func.function = &F_integrand;
    Func.params = NULL;

    gsl_integration_qagiu( &Func, GSL_BESSEL_BREAKPOINT,
            GSL_INTE_ERR_ABS, GSL_INTE_ERR_REL, GSL_INTE_WS_LEN,
            inte_ws, &F_V_above_breakpoint, &F_V_above_breakpoint_err );
    writelog( "F0(%g): %g [%g]\n",
            GSL_BESSEL_BREAKPOINT,
            F_V_above_breakpoint,
            F_V_above_breakpoint_err
            );
    output_F_x();

}

double tab_F( double x ) {

    double dx, dlogx, logx, r;
    int i;

    dlogx = log(F_X_MAX/F_X_MIN) / ( TAB_F_N - 1 );
    logx = log( x );
    i = (logx-log(F_X_MIN) ) / dlogx;

    if ( i<0 )
        return tab_F_V[0];
    if ( i>TAB_F_N-1 )
        return tab_F_V[TAB_F_N-1];

    dx = logx - log(F_X_MIN) - i * dlogx;
    r = tab_F_V[i] * ( 1-dx ) + tab_F_V[i+1] * dx;
    return r;

}

void test_tab_F() {

    double x, xmax, xmin, F_v, tab_F_v, err, err_max, err_mean;
    int i, N;
    FILE *fd;

    xmin = F_X_MIN;
    xmax = F_X_MAX - 1;
    N = 1000;

    writelog( "test tab_F\n" );

    if ( ThisTask )
        return;

    fd = myfopen( "w", "test_tab_F.dat" );

    i = 0;
    err_max = err_mean = 0;

    fprintf( fd, "%10s %11s %11s  %5s\n", "x", "F(x)", "tab_F(x)", "err");
    while( i<N ) {
        x = xmin * exp( ((double)rand()) / RAND_MAX * log(xmax/xmin) );
        F( x, &F_v, &err );
        tab_F_v = tab_F( x );
        err = fabs( tab_F_v - F_v ) / F_v * 100;
        fprintf( fd, "%10.3e %11.3e %11.3e %5.2f%%\n", x, F_v, tab_F_v, err);
        if ( err > err_max )
            err_max = err;
        err_mean += err;
        i++;
    }

    fclose( fd );

    err_mean /= N;
    printf( "err_max: %.2f%%, err_mean: %.2f%%\n", err_max, err_mean );

}

void output_tab_F() {
    int i;
    FILE *fd;
    double dlogx, x;

    writelog( "output tab_F\n" );

    if ( ThisTask )
        return;

    dlogx = log(F_X_MAX/F_X_MIN) / ( TAB_F_N - 1 );
    fd = myfopen( "w", "tab_F.dat" );

    for( i=0; i<=TAB_F_N; i++ ) {
        x = log( F_X_MIN ) + dlogx * i;
        fprintf( fd, "%g %g\n", exp(x), tab_F_V[i] );
    }

    fclose( fd );

}

void init_tab_F() {
    double dlogx, x, err, F_v, *buf, err_max, err_max_global, 
    err_mean, err_mean_global;
    int i;

    put_header( "initialize tab_F" );

#ifdef INIT_TAB_F_DEBUG
double err_x;
#endif
    dlogx = log(F_X_MAX/F_X_MIN) / ( TAB_F_N - 1 );
    mymalloc2( tab_F_V, sizeof( double ) * ( TAB_F_N ) );
    err_max = -1;
    err_mean = 0;

    for ( i=0; i<TAB_F_N; i++ ) {
        if ( i % NTask != ThisTask )
            continue;
        x = log( F_X_MIN ) + dlogx * i;
        x = exp( x );
        F( x, &F_v, &err );

        if ( err>err_max ) {
            err_max = err;
#ifdef INIT_TAB_F_DEBUG
            err_x = x;
#endif
        }

        err_mean += err;
        tab_F_V[i] = F_v;
    }

#ifdef INIT_TAB_F_DEBUG
    writelog( "[%03i], x: %g, err_x: %g\n", ThisTask, err_x, err_max );
#endif

    MPI_Reduce( &err_max, &err_max_global, 1, MPI_DOUBLE,
            MPI_MAX, 0, MPI_COMM_WORLD );
    MPI_Reduce( &err_mean, &err_mean_global, 1, MPI_DOUBLE,
            MPI_SUM, 0, MPI_COMM_WORLD );
    writelog( "[error of the integral of f(x)] max: %g, mean: %g\n",
        err_max_global, err_mean_global/TAB_F_N );

    mymalloc2( buf, sizeof( double ) * ( TAB_F_N+1 ) );
    MPI_Allreduce( tab_F_V, buf, TAB_F_N+1, MPI_DOUBLE,
            MPI_SUM, MPI_COMM_WORLD );
    memcpy( tab_F_V, buf, (TAB_F_N+1) * sizeof( double ) );
    myfree( buf );

    output_tab_F();
    test_tab_F();

    do_sync( "" );
    put_end();
}

void free_tab_F() {
    myfree( tab_F_V );
}

/*
        x = nu / nu_c 
        nu_c = 3/16 * ( 1+p^2 ) *e*B / ( m_e*c )
        p = sqrt( nu / x / ( 3/16*e*B / ( m_e*c ) ) - 1 )
        0.1875: 3/16
*/

#define get_x(  B, nu, p ) ( nu / (0.1875 * ( 1+SQR(p) ) * (B) * cuc.e_mec) )

double radio_inte( double p, void *params ) {

    double r, x, err;
    struct radio_inte_struct *ri;
    (void) err;

    ri = params;
    x = get_x( ri->B, ri->nu, p );

#ifdef TABF
        r = tab_F( x );
#else
        F( x, &r, &err );
#endif

    r *= (*(ri->f))( p, ri->params );

/*
    printf( "p: %g, r:%g\n", p, r );
    fflush( stdout );
*/

    return r;

}

double radio( double (*f)( double, void* ), double *params,
        double B, double nu, double pmin, double pmax, double err ) {

    double r, fac, t;
    struct radio_inte_struct ri;

    ri.f = f;
    ri.params = params;
    ri.B = B;
    ri.nu = nu;

#ifdef PART_RADIO_TEST2
        printf( "[%s] B: %g, nu: %g, c: %g, a: %g, qmin: %g[%g], qmax: %g[%g]\n",
        __FUNCTION__,
        B, nu, params[0], params[1],
        pmin, params[2], pmax, params[3]
        );
#endif

    
    t = sqrt( nu / GSL_BESSEL_UPPER_LIMIT / ( 0.1875 * B * cuc.e_mec )-1 );
    if ( pmin < t )
        pmin = t;

#ifdef PART_RADIO_TEST2
    double xmin, xmax;
    xmin = get_x( B, nu, pmax );
    xmax = get_x( B, nu, pmin );
        printf( "[%s] new_qmin: %g, pmax: %g, xmin: %g, xmax: %g\n",
        __FUNCTION__, pmin, pmax, xmin, xmax
        );
#endif

    if ( pmin > pmax * 0.9 ) // avoid bad integration.
        return 0;

    //r = qtrap( &radio_inte, &ri, 1, pmin, pmax, err );
    
    gsl_function F;
    F.function = &radio_inte;
    F.params = &ri;

    gsl_integration_qag( &F, pmin, pmax,
            GSL_INTE_ERR_ABS, GSL_INTE_ERR_REL2, GSL_INTE_WS_LEN,
            GSL_INTE_KEY, inte_ws2, &r, &err );

#ifdef PART_RADIO_TEST2
        printf( "[%s] value of integral: %g, err: %g\n",
        __FUNCTION__, r, err );
        /*
        r = qtrap( &radio_inte, &ri, 1, pmin, pmax, err );
        printf( "[%s] value of integral: %g, err: %g\n",
        __FUNCTION__, r, err );
        */
#endif
    //printf( "%f\n", B );
    fac = sqrt(3) * CUBE( cuc.e ) * B * PI / ( 4.0 * cuc.mec2 );
    r *= fac;

#ifdef PART_RADIO_TEST2
        printf( "[%s] radio: %g\n",
        __FUNCTION__, r );
#endif

    return r;

}

