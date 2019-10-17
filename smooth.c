#include "allvars.h"

//#define SMOOTH_DEBUG
//#define SMOOTH_DEBUG2
#define HIGH_PRECISION

void smooth() {

#if defined(BSMOOTH) || defined(CRESMOOTH)
    int ngbnum, k, n;
    long i, j;
    double h_i, h2_i, h_j, h2_j, r, cm[3], r2, t, hinv_i, hinv3_i, hinv4_i, u,
           wk, dwk, m_j, hinv_j, hinv3_j, hinv4_j;
#ifdef HIGH_PRECISION
    long double B[3], densitynorm, fac;
#else
     double B[3], densitynorm, fac;
#endif
#ifdef SMOOTH_DEBUG2
     long debug_i=99;
#endif

    mytimer_start();
    put_header( "smooth" );
    writelog(
#ifdef BSMOOTH
    "B smooth\n"
#endif
#ifdef CRESMOOTH
    "CRE smooth\n"
#endif
    );

#ifdef HIGH_PRECISION
    writelog( "long double: %li, double: %li\n",
        sizeof(long double), sizeof(double) );
#endif

    (void)h2_i;

    mymalloc1( Ngblist, NumPart * sizeof(long) );

    for( i=0; i<N_Gas; i++ ) {

#ifdef SMOOTH_DEBUG2
        i = debug_i;
        if (ThisTask_Local)
            continue;
#else
        if ( i % NTask_Local != ThisTask_Local )
            continue;
#endif

        h_i = SphP[i].Hsml;
        h2_i = h_i*h_i;
        kernel_hinv( h_i, &hinv_i, &hinv3_i, &hinv4_i );
        densitynorm = 0;
        for( k=0; k<3; k++ ) {
            cm[k] = P[i].Pos[k];
#ifdef BSMOOTH
#ifdef HIGH_PRECISION
            B[k] = 0;
#else
            SphP[i].SmoothB[k] = 0;
#endif
#endif
        }
#ifdef CRESMOOTH
        SphP[i].SmoothCRE_C =
        SphP[i].SmoothCRE_Alpha =
        SphP[i].SmoothCRE_qmin =
        SphP[i].SmoothCRE_qmax =
        SphP[i].SmoothCRE_n =
        SphP[i].SmoothCRE_e = 0;
#endif

        ngbnum = ngb( cm, h_i, 0 );
        if ( ngbnum == 0 )
            continue;

        for( n=0; n<ngbnum; n++ ) {

            j = Ngblist[n];
            if ( P[j].Type != 0 )
                continue;
            for( k=0, r2=0; k<3; k++ ) {
                t = P[j].Pos[k] - cm[k];
                t = PERIODIC_HALF( t );
                r2 += t*t;
            }

            r = sqrt( r2 );

            h_j = SphP[j].Hsml; 
            //h_j = h_i;
            h2_j = h_j * h_j;

            if ( r2<h2_j ) {

                kernel_hinv( h_j, &hinv_j, &hinv3_j, &hinv4_j );
                m_j = P[j].Mass;

                u = r * hinv_j;
                kernel_main( u, hinv3_j, hinv4_j, &wk, &dwk, -1 );
                wk /= SphP[j].Density;

                fac = m_j * wk;
                for( k=0; k<3; k++ ) {
#ifdef BSMOOTH
                     B[k] += fac * SphP[j].B[k];
#endif
                }

#ifdef CRESMOOTH
                SphP[i].SmoothCRE_C      += fac * SphP[j].CRE_C;
                SphP[i].SmoothCRE_Alpha  += fac * SphP[j].CRE_Alpha;
                SphP[i].SmoothCRE_qmin   += fac * SphP[j].CRE_qmin;
                SphP[i].SmoothCRE_qmax   += fac * SphP[j].CRE_qmax;
                SphP[i].SmoothCRE_n      += fac * SphP[j].CRE_n;
                SphP[i].SmoothCRE_e      += fac * SphP[j].CRE_e;
#endif
                densitynorm += fac; 
            }
#ifdef SMOOTH_DEBUG2
            printf( "(%g %g %g): %g\n",
            P[j].Pos[0],
            P[j].Pos[1],
            P[j].Pos[2],
            get_B(j)
            );
#endif

        } // for n

        if ( densitynorm > 0 ) {
            for( k=0; k<3; k++ ) {
#ifdef BSMOOTH
                SphP[i].SmoothB[k] = B[k] / densitynorm;
#endif
            }
#ifdef CRESMOOTH
            SphP[i].CRE_C      /= densitynorm; 
            SphP[i].CRE_Alpha  /= densitynorm; 
            SphP[i].CRE_qmin   /= densitynorm; 
            SphP[i].CRE_qmax   /= densitynorm; 
            SphP[i].CRE_n      /= densitynorm; 
            SphP[i].CRE_e      /= densitynorm; 
#endif
        }
#ifdef SMOOTH_DEBUG2
        break;
#endif
    } // for i

    do_sync_local( "smooth" );

    for ( i=0; i<N_Gas; i++ ) {
        if ( i % NTask_Local != ThisTask_Local )
            continue;
#ifdef BSMOOTH
        for( k=0; k<3; k++ )
            SphP[i].B[k] = SphP[i].SmoothB[k];
#endif

#ifdef CRESMOOTH
/*
if (!ThisTask_Local) 
    printf(
       "%li c:%g[%g], "
       "a:%g[%g], "
       "qmin:%g[%g], "
       "qmax:%g[%g], "
       "n:%g[%g], "
       "e:%g[%g]\n ", 
        i,
        SphP[i].CRE_C      ,  SphP[i].SmoothCRE_C,
        SphP[i].CRE_Alpha  ,  SphP[i].SmoothCRE_Alpha,
        SphP[i].CRE_qmin   ,  SphP[i].SmoothCRE_qmin,
        SphP[i].CRE_qmax   ,  SphP[i].SmoothCRE_qmax,
        SphP[i].CRE_n      ,  SphP[i].SmoothCRE_n,
        SphP[i].CRE_e      ,  SphP[i].SmoothCRE_e
        );
*/
        SphP[i].CRE_C      =  SphP[i].SmoothCRE_C;
        SphP[i].CRE_Alpha  =  SphP[i].SmoothCRE_Alpha;
        SphP[i].CRE_qmin   =  SphP[i].SmoothCRE_qmin;
        SphP[i].CRE_qmax   =  SphP[i].SmoothCRE_qmax;
        SphP[i].CRE_n      =  SphP[i].SmoothCRE_n;
        SphP[i].CRE_e      =  SphP[i].SmoothCRE_e;
#endif
    }

    myfree( Ngblist );
    mytimer_end();

    do_sync_local( "smooth" );

#ifdef SMOOTH_DEBUG
    if ( !ThisTask_Local ) {
#ifdef SMOOTH_DEBUG2
        printf( "[%li] %g\n", debug_i, get_B(debug_i) );
#else
        for( i=0; i<100; i++ )
            printf( "[%li] %g\n", i, get_B(i) );
#endif
        endruns( "smooth-test" );
    }
#endif
    put_end();

#endif

}
