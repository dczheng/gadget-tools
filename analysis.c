#include "allvars.h"

void init_analysis() {
    writelog( "initialize analysis...\n" );
    All.proj_k = All.ProjectDirection;
    All.proj_i = ( All.ProjectDirection + 1 ) % 3;
    All.proj_j = ( All.ProjectDirection + 2 ) % 3;
    All.Sproj = All.ProjectDirection + 'x';
    All.PicSize2 = SQR( All.PicSize );
    slice();
    if ( All.KernelInterpolation )
        init_kernel_matrix();
    inte_ws = gsl_integration_workspace_alloc( GSL_INTE_WS_LEN );
    if ( All.ConvFlag )
        init_conv_kernel();
    init_img();
    writelog( "initialize analysis... done.\n" );
    put_block_line;
}

void free_analysis() {
    writelog( "free analysis ...\n" );
    gsl_integration_workspace_free( inte_ws );
    if ( All.KernelInterpolation )
        free_kernel_matrix();
    if ( All.ConvFlag )
        free_conv_kernel();
    free_img();
    writelog( "free analysis ... done.\n" );
    put_block_line;
}

void gas_density_slice() {
    int num, i;
    char buf[100];
    writelog( "gas density silce ...\n" );
    num = All.SliceEnd[0] - All.SliceStart[0];
    mymalloc( image.data, sizeof( double ) * num );
    mymalloc( image.img, sizeof( double ) * SQR( All.PicSize ) );
    memset( image.data, 0, sizeof( double ) * num );
    memset( image.img, 0, sizeof( double ) * SQR( All.PicSize ) );
    for ( i=All.SliceStart[0]; i<num; i++ ) {
        image.data[i] = SphP[i].Density;
    }

    create_dir( "./gas_density" );
    sprintf( buf, "./gas_density/%s_%03i_%.2f.dat", All.FilePrefix, ThisTask, All.RedShift );

    make_slice_img( 0 );

    for ( i=0; i<SQR(All.PicSize); i++ ) {
        image.img[i] *= All.UnitMass_in_g / pow( All.UnitLength_in_cm, 2 );
    }


    write_img( buf );
    myfree( image.data );
    myfree( image.img );

    writelog( "gas density silce ... done.\n" );
    put_block_line;
}

int compare_gas_rho( const void *a, const void *b ){
    return ((((struct sph_particle_data* )a)->Density) < (((struct sph_particle_data*)b)->Density)) ? 1: -1;
}

void sort_gas_rho(){
    int i;
    qsort( (void*)SphP, N_Gas, sizeof( struct sph_particle_data ), &compare_gas_rho );
    for ( i=0; i<10; i++ ) {
        printf( "%g\n", SphP[i].Density * ( g2c.g / CUBE(g2c.cm) ) );
    }
    printf( "\n" );
    for ( i=0; i<10; i++ ) {
        printf( "%g\n", SphP[N_Gas-10+i].Density * ( g2c.g / CUBE(g2c.cm) ) );
    }
}

void vel_value() {

    writelog( "velocities value analysis ...\n" );
    FILE *fd;
    char buf[20];
    int i;
    double v;
    sprintf( buf, "vel_%i.txt", ThisTask );
    fd = fopen( buf, "w" );
    for ( i=0; i<N_Gas; i++ ) {
        v = sqrt( pow( P[i].Vel[0], 2 ) + pow( P[i].Vel[1], 2 ) + pow( P[i].Vel[2], 2 ) );
        if ( v > 1000 ) {
            fprintf( fd, "%i %g\n", P[i].ID, v );
        }
    }
    fclose( fd );
    writelog( "velocities value analysis ... done\n" );
    put_block_line;
}

double particle_radio( double v, long i ) {

    double C, B, Ub, vl, P;

    C = SphP[i].CRE_C0 * pow( SphP[i].Density, (All.Alpha-1)/3.0 );
    C = C * SphP[i].Density / ( ELECTRON_MASS /(g2c.g) );
    C /= CUBE( g2c.cm );
    B = sqrt( pow( SphP[i].B[0], 2 ) + pow( SphP[i].B[1], 2 ) + pow( SphP[i].B[2], 2 ) );
    Ub = B * B / ( 8 * PI );
    vl = ELECTRON_CHARGE * B / ( 2 * PI * ELECTRON_MASS * LIGHT_SPEED );
    P = C * 2.0 / 3.0 * LIGHT_SPEED * Ub * THOMSON_CROSS_SECTION *
        pow( v / vl-1, (1-All.Alpha) / 2 ) / vl;
    //printf( "%g\n", P );
    return P;

}

double particle_hge_num_dens( long i ) {

    double n;
    n = SphP[i].CRE_C0 * pow( SphP[i].CRE_Q0, 1 - All.Alpha ) / ( All.Alpha - 1 );
    n = n * SphP[i].Density / ( ( ELECTRON_MASS / ( g2c.g ) ) );
    n /= CUBE( g2c.cm );
    return n;

}

/*
void compute_radio( double v ) {

    mymalloc( img, sizeof( double ) * SQR( PicSize ) );
    memset( img, 0, sizeof( double ) * SQR( PicSize ) );

    //com_dis = comoving_distance( header.time ) * ( g2c.cm );
    ang_dis = angular_distance( header.time ) * ( g2c.cm );
    lum_dis = luminosity_distance( header.time ) * ( g2c.cm );
    dy = dx = BoxSize / PicSize;
    h = All.SofteningTable[0] * ( g2c.cm );
    V = 4.0 / 3.0 * PI * pow( h, 3 );
    ang = h / ang_dis / PI * 180.0 * 60;
    beam = pow( 10.0 / 60, 2.0 ) / ( 4.0 * log(2) );
    img_max = DBL_MIN;
    img_min = DBL_MAX;
    for ( i=0; i<N_Gas; i++ ) {
        xi = (int)( P[i].Pos[0] / dx );
        yi = (int)( P[i].Pos[1] / dy );
        tmp = img[xi * PicSize + yi] += SphP[i].P * V / ( 4*PI*pow(lum_dis,2) ) / SQR(ang)
            * beam * 1e25;
        //tmp = img[xi * PicSize + yi] += SphP[i].P * V / ( 4*PI*pow(lum_dis,2) ) / SQR(ang)
        //    * 1e25;
        img_max = ( tmp > img_max ) ? tmp : img_max;
        img_min = ( tmp < img_min && tmp > 0 ) ? tmp : img_min;
    }
}
*/

void magnetic() {
    long i;
    double B, Bmin, Bmax;

    Bmax = -DBL_MAX;
    Bmin = DBL_MAX;
    for ( i=0; i<N_Gas; i++ ) {
        B = sqrt(
                SQR( SphP[i].B[0] ) +
                SQR( SphP[i].B[1] ) +
                SQR( SphP[i].B[2] ) );
        Bmin = vmin( B, Bmin, 0 );
        Bmax = vmax( B, Bmax );
    }
    printf( "Bmin: %g, Bmax: %g\n",
            Bmin, Bmax );

}

void output_rho() {
    long i;
    FILE *fd;
    double rho_max, rho_min;

    fd = fopen( "./rho.txt", "w" );
    rho_max = -DBL_MAX;
    rho_min = DBL_MAX;
    for ( i=0; i<N_Gas; i++ ) {
        fprintf( fd, "%g %g %g %g\n",
                P[i].Pos[0],
                P[i].Pos[1],
                P[i].Pos[2],
                SphP[i].Density );
        rho_max = ( SphP[i].Density > rho_max ) ? SphP[i].Density : rho_max;
        rho_min = ( SphP[i].Density < rho_min ) ? SphP[i].Density : rho_min;
    }
    printf( "rho_max = %g, rho_min = %g \n", rho_max, rho_min );
    fclose( fd );

}

void compute_temperature() {
    double yhelium, u, ne, mu, XH;
    int i;

    writelog( "compute gas temprature...\n" );
    XH = HYDROGEN_MASSFRAC;
    yhelium = ( 1 - XH ) / ( 4 * XH );
    for ( i=0; i<N_Gas; i++ ) {
        u = SphP[i].u * All.UnitPressure_in_cgs / All.UnitDensity_in_cgs;
        ne = SphP[i].elec;
        mu = ( 1 + 4 * yhelium ) / ( 1 + yhelium + ne );
        SphP[i].Temp = GAMMA_MINUS1 / BOLTZMANN * u * PROTONMASS * mu;
    }
    writelog( "compute gas temprature... done.\n" );
    put_block_line;

}

void gas_state() {

    double TempMin, TempMax, DensMin, DensMax,
           LogTempMin, LogTempMax, LogDensMin, LogDensMax,
           *img, DLogTemp, DLogDens, LogDens, LogTemp, sum,
           GlobLogTempMin, GlobLogTempMax, GlobLogDensMin, GlobLogDensMax;
    int i, j, k, PicSize;
    char buf[200];

    TempMin = DensMin = DBL_MAX;
    TempMax = DensMax = DBL_MIN;
    writelog( "plot gas state...\n" );

    //PicSize_tmp = All.PicSize;
    PicSize = All.PicSize;


    mymalloc( img, sizeof( double ) * SQR( PicSize ) );
    memset( img, 0, sizeof( double ) * SQR( PicSize ) );

    for ( i=0; i<N_Gas; i++ ) {
        TempMin = vmin( SphP[i].Temp, TempMin, 1 );
        TempMax = vmax( SphP[i].Temp, TempMax );
        DensMin = vmin( SphP[i].Density/All.RhoBaryon, DensMin, 1 );
        DensMax = vmax( SphP[i].Density/All.RhoBaryon, DensMax );
    }

    LogDensMin = log10( DensMin );
    LogDensMax = log10( DensMax );
    LogTempMin = log10( TempMin );
    LogTempMax = log10( TempMax );
    find_global_value( LogDensMin, GlobLogDensMin, MPI_DOUBLE, MPI_MIN );
    find_global_value( LogDensMax, GlobLogDensMax, MPI_DOUBLE, MPI_MAX );
    find_global_value( LogTempMin, GlobLogTempMin, MPI_DOUBLE, MPI_MIN );
    find_global_value( LogTempMax, GlobLogTempMax, MPI_DOUBLE, MPI_MAX );

    //LogTempMax = 7;

    DLogTemp = ( LogTempMax - LogTempMin ) / PicSize;
    DLogDens = ( LogDensMax - LogDensMin ) / PicSize;
    writelog( "DensMin: %g, DensMax: %g, TempMin: %g, TempMax: %g\n"
            "LogDensMin: %g, LogDensMax: %g, LogTempMIn: %g, LogTempMax: %g\n"
            "GlobLogDensMin: %g, GlobLogDensMax: %g,\nGlobLogTempMIn: %g, GlobLogTempMax: %g\n",
            DensMin, DensMax, TempMin, TempMax,
            LogDensMin, LogDensMax, LogTempMin, LogTempMax,
            GlobLogDensMin, GlobLogDensMax, GlobLogTempMin, GlobLogTempMax );

    ZSPRINTF( 0, "1" );
    for ( k=0; k<N_Gas; k++ ) {
        LogTemp = SphP[k].Temp;
        if ( LogTemp < 0 )
            endrun( 20180514 );
        LogTemp = ( LogTemp == 0 ) ? LogTempMin : log10( LogTemp );

        //if ( LogTemp > LogTempMax )
        //    continue;

        LogDens = SphP[k].Density / All.RhoBaryon;
        if ( LogDens < 0 )
            endrun( 20180514 );
        LogDens = ( LogDens == 0 ) ? LogDensMin : log10( LogDens );

        i = ( LogTemp - LogTempMin ) / DLogTemp;
        check_picture_index( i );

        j = ( LogDens - LogDensMin ) / DLogDens;
        check_picture_index( j );
        if ( i < 0 || i >= All.PicSize || j < 0 || j >= All.PicSize)
            printf( "%i, %i\n", i, j );
        img[ i*PicSize + j ]++;
     //  printf( "%g ", img[ i*PicSize +j ] ) ;
    }
    ZSPRINTF( 0, "2" );

    for ( i=0, sum=0; i<SQR(PicSize); i++ )
        sum += img[i];

    for ( i=0; i<SQR(PicSize); i++ )
        img[i] /= sum;

    create_dir( "./gas_state" );
    sprintf( buf, "./gas_state/%s_%03i_%.2f.dat", All.FilePrefix, ThisTask, All.RedShift );

    ZSPRINTF( 0, "3" );
    image.img = img;
    img_xmin = LogDensMin;
    img_xmax = LogDensMax;
    img_ymin = LogTempMin;
    img_ymax = LogTempMax;
    write_img( buf );
    myfree( img );

    //All.PicSize = PicSize_tmp;
    put_block_line;

}

void gas_temperature_slice() {

    int num, i, PicSize2;
    char buf[100];
    writelog( "gas temperature silce ...\n" );
    PicSize2 = All.PicSize2;
    num = All.SliceEnd[0] - All.SliceStart[0];
    mymalloc( image.data, sizeof( double ) * num );
    mymalloc( image.img, sizeof( double ) * SQR( All.PicSize ) );
    memset( image.img, 0, sizeof( double ) * SQR( All.PicSize ) );
    memset( image.data, 0, sizeof( double ) * num );

    for ( i=All.SliceStart[0]; i<num; i++ ) {
        image.data[i] = SphP[i].Temp;
    }

    create_dir( "./gas_temperature" );
    sprintf( buf, "./gas_temperature/%s_%03i_%.2f.dat", All.FilePrefix, ThisTask, All.RedShift );

    make_slice_img( 0 );

    for ( i=0; i<PicSize2; i++ ){
        image.img[i] *= All.UnitLength_in_cm;
    }

    write_img( buf );

    myfree( image.data );
    myfree( image.img );
    writelog( "gas Temperature silce ... done.\n" );
    put_block_line;

}

void mass_function() {

    int g, *num, i, i_m_min, i_m_max;
    double m_min, m_max;
    char buf[100];
    FILE *fd;
    writelog( "compute mass function ...\n" );

    m_min = DBL_MAX;
    m_max = -DBL_MAX;

    for ( g=0; g<Ngroups; g++ ) {
        m_min = vmin( m_min, Gprops[g].mass, 0 );
        m_max = vmax( m_max, Gprops[g].mass );
    }

    m_min *= 1e10;
    m_max *= 1e10;

    writelog( "m_min: %g, m_max: %g\n", m_min, m_max );

    i_m_min = floor( log10(m_min) );
    i_m_max = floor( log10(m_max) );

    writelog( "i_m_min: %i, i_m_max: %i\n", i_m_min, i_m_max );

    if ( i_m_min == i_m_max ) {
        printf( "some error appear!\n" );
        endrun( 20180724 );
    }

    mymalloc( num, sizeof(int) * ( i_m_max - i_m_min + 1 ) );
    memset( num, 0, sizeof( int ) * ( i_m_max - i_m_min + 1 ) );

    for ( g=0; g<Ngroups; g++ ) {
        num[ ( int ) ( floor( log10( Gprops[g].mass * 1e10 ) ) - i_m_min ) ] ++;
    }

    for ( i=i_m_max-1; i>=i_m_min; i-- ) {
        num[i-i_m_min] += num[i-i_m_min+1];
    }

    /*
    for ( i=m_min; i<=m_max; i++ ) {
        writelog( "%i\n", num[i] );
    }
    */

    create_dir( "./mf" );
    sprintf( buf, "./mf/%s_mf_%.2f.dat", All.FilePrefix, All.RedShift );

    fd = fopen( buf, "w" );

    for ( i=i_m_min; i<=i_m_max; i++ ) {

        fprintf( fd, "%i %g\n", i, num[i-i_m_min] / CUBE( All.BoxSize / All.MpcFlag ) );

    }

    fclose( fd );

    myfree( num );

    writelog( "compute mass function ... done.\n" );
    put_block_line;
}

void group_analysis() {

    long index, i, p, PicSize, x, y, ii, jj,
         xo, yo, PicSize2, npart[6];
    struct group_properties g;
    double *img, *m, L, dL, mass[6], B, PP, nu, n;
    char buf[100];

    if ( All.FoF == 0 ) {
        printf( "FoF is required by Group Analysis!\n" );
        endrun( 20180724 );
    }

    index = All.GroupIndex;
    writelog( "analysis group: %li ...\n", index );

    PicSize = All.PicSize;
    PicSize2 = All.PicSize2;
    x = All.proj_i;
    y = All.proj_j;
    xo = PicSize / 2;
    yo = PicSize / 2;
    g = Gprops[index];
    mymalloc( img, PicSize2 * sizeof( double ) );
    mymalloc( m, PicSize2 * sizeof( double ) );

    create_dir( "./group" );

    writelog( "center of mass: %g %g %g\n",
            g.cm[0], g.cm[1], g.cm[2] );

    p = g.Head;

    for ( i=0; i<6; i++ ) {
        npart[i] = 0;
        mass[i] = 0;
    }

    for ( i=0, L=0; i<g.Len; i++, p=FoFNext[p] ){
        npart[ P[p].Type ] ++;
        mass[ P[p].Type] += P[p].Mass;

        if ( P[p].Type == 0 ) {
            //printf( "%g\n", SphP[p].Star_Mass );
            mass[4] += SphP[p].Star_Mass;
            mass[5] += SphP[p].BH_Mass;
        }

        if ( P[p].Type != 0 )
            continue;
        L = vmax( NGB_PERIODIC( P[p].Pos[x] - g.cm[x] ), L );
        L = vmax( NGB_PERIODIC( P[p].Pos[y] - g.cm[y] ), L );
    }

    dL = 2 * L / PicSize;
    writelog( "npart: " );
    for ( i=0; i<6; i++ )
        writelog( "%li ", npart[i] );
    writelog( "\n" );

    writelog( "mass: " );
    for ( i=0; i<6; i++ ) {
        mass[i] *= 1e10;
        writelog( "%g ", mass[i] );
    }

    writelog( "\n" );
    writelog( "L: %g, dL:%g\n", 2*L, dL );

    for ( i=0; i<6; i++ )
        img_props(i) = mass[i];
    img_xmin =  -L;
    img_xmax =  L;
    img_ymin =  -L;
    img_ymax =  L;

    memset( m, 0, PicSize2 * sizeof( double ) );
    p = g.Head;
    for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {
        if ( P[p].Type != 0 )
            continue;
        ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
        jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
        //printf( "%li, %li\n", ii, jj );
        check_picture_index( ii );
        check_picture_index( jj );
        m[ ii*PicSize + jj ] += P[p].Mass;
    }

/********************temperture*************************/
    writelog( "\ngroup temperature ...\n" );
    memset( img, 0, PicSize2 * sizeof( double ) );
    p = g.Head;
    for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {
        if ( P[p].Type != 0 )
            continue;
        ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
        jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
        //printf( "%li, %li\n", ii, jj );
        check_picture_index( ii );
        check_picture_index( jj );
        img[ ii*PicSize + jj ] += SphP[p].Temp * P[p].Mass;
        //printf( "%g\n", SphP[p].Temp );
    }

    for ( i=0; i<PicSize2; i++ ) {
        if ( m[i] == 0 )
            continue;
        img[i] /= m[i];
    }

    image.img = img;

    if ( All.ConvFlag )
        conv( dL );

    create_dir( "./group/Temperature" );
    sprintf( buf, "./group/Temperature/%s_%03i_Temperature_%.2f_%c.dat", All.FilePrefix, ThisTask, All.RedShift, All.Sproj );
    write_img( buf );

    writelog( "group temperature ... done.\n" );

/********************temperture*************************/

/********************SFR*************************/
    if ( All.SfrFlag ) {
        writelog( "\ngroup SFR ...\n" );
        memset( img, 0, PicSize2 * sizeof( double ) );
        p = g.Head;
        for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {
            if ( P[p].Type != 0 )
                continue;
            ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
            jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
            //printf( "%li, %li\n", ii, jj );
            check_picture_index( ii );
            check_picture_index( jj );
            img[ ii*PicSize + jj ] += SphP[p].sfr * P[p].Mass;
            //printf( "%g\n", SphP[p].Temp );
        }

        for ( i=0; i<PicSize2; i++ ) {
            if ( m[i] == 0 )
                continue;
            img[i] /= m[i];
        }

        image.img = img;

        if ( All.ConvFlag )
            conv( dL );

        create_dir( "./group/SFR" );
        sprintf( buf, "./group/SFR/%s_%03i_SFR_%.2f_%c.dat", All.FilePrefix, ThisTask, All.RedShift, All.Sproj );
        write_img( buf );

        writelog( "group SFR ... done.\n" );
    }

/********************temperture*************************/

/********************magnetic field*************************/

    if ( All.BFlag ) {
        writelog( "\ngroup magnetic field ...\n" );

        memset( img, 0, PicSize2 * sizeof( double ) );
        p = g.Head;
        for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {
            if ( P[p].Type != 0 )
                continue;
            ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
            jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
            //printf( "%li, %li\n", ii, jj );
            check_picture_index( ii );
            check_picture_index( jj );
            B = sqrt( SQR( SphP[p].B[0] ) +
                      SQR( SphP[p].B[1] ) +
                      SQR( SphP[p].B[2] ) );
            B *= 1e6;  // convert G to muG
            img[ ii*PicSize + jj ] += B * P[p].Mass;
        }

        for ( i=0; i<PicSize2; i++ ) {
            if ( m[i] == 0 )
                continue;
            img[i] /= m[i];
        }

        image.img = img;

        if ( All.ConvFlag )
            conv( dL );

        create_dir( "./group/MagneticField" );
        sprintf( buf, "./group/MagneticField/%s_%03i_MagneticField_%.2f_%c.dat", All.FilePrefix, ThisTask, All.RedShift, All.Sproj );
        write_img( buf );


        writelog( "group magnetic field ... done.\n" );
    }

/********************magnetic field*************************/

/********************mach number*************************/

    if ( All.MachFlag ) {
        writelog( "\ngroup mach number ...\n" );

        memset( img, 0, PicSize2 * sizeof( double ) );
        p = g.Head;
        for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {

            if ( P[p].Type != 0 )
                continue;

            ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
            jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
            //printf( "%li, %li\n", ii, jj );
            check_picture_index( ii );
            check_picture_index( jj );
            img[ ii*PicSize + jj ] += SphP[p].MachNumber * P[p].Mass;
        }

        for ( i=0; i<PicSize2; i++ ) {
            if ( m[i] == 0 )
                continue;
            img[i] /= m[i];
        }

        image.img = img;

        if ( All.ConvFlag )
            conv( dL );

        create_dir( "./group/MachNumber" );
        sprintf( buf, "./group/MachNumber/%s_%03i_MachNumber_%.2f_%c.dat", All.FilePrefix, ThisTask, All.RedShift, All.Sproj );
        write_img( buf );

        writelog( "group Mach Number ... done.\n" );
    }

/********************mach number*************************/

/********************Radio*************************/

    if ( All.RadioFlag ) {

        writelog( "\ngroup radio ...\n" );

        if ( All.HgeFlag == 0 ) {

            printf( "HgeFlag is required by computing radio!\n" );
            endrun( 20180730 );

        }

        memset( img, 0, PicSize2 * sizeof( double ) );
        p = g.Head;

        nu = 120 * 1e6;

        for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {

            if ( P[p].Type != 0 )
                continue;

            PP = particle_radio( nu, p );
            ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
            jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
            check_picture_index( ii );
            check_picture_index( jj );
            img[ ii*PicSize + jj ] += PP * P[p].Mass;
        }

        for ( i=0; i<PicSize2; i++ ) {
            if ( m[i] == 0 )
                continue;
            img[i] /= m[i];
        }

        if ( All.ConvFlag )
            conv( dL );

        create_dir( "./group/Radio" );
        sprintf( buf, "./group/Radio/%s_%03i_radio_%.2f_%c.dat", All.FilePrefix, ThisTask, All.RedShift, All.Sproj );
        write_img( buf );

        writelog( "\ngroup radio ... done.\n" );

    }

/********************Radio*************************/

/********************Hge Number Density*************************/

    if ( All.HgeNumDensFlag ) {

        writelog( "\ngroup Hge Number Density ...\n" );

        if ( All.HgeFlag == 0 ) {

            printf( "HgeFlag is required by computing radio!\n" );
            endrun( 20180730 );

        }

        memset( img, 0, PicSize2 * sizeof( double ) );
        p = g.Head;

        nu = 120 * 1e6;

        for ( i=0; i<g.Len; i++, p=FoFNext[p] ) {

            if ( P[p].Type != 0 )
                continue;

            n = particle_hge_num_dens( p );
            ii = PERIODIC( P[p].Pos[x] - g.cm[x] ) / dL + xo;
            jj = PERIODIC( P[p].Pos[y] - g.cm[y] ) / dL + yo;
            check_picture_index( ii );
            check_picture_index( jj );
            img[ ii*PicSize + jj ] += n * P[p].Mass;
        }

        for ( i=0; i<PicSize2; i++ ) {
            if ( m[i] == 0 )
                continue;
            img[i] /= m[i];
        }

        if ( All.ConvFlag )
            conv( dL );

        create_dir( "./group/HgeNumDens" );
        sprintf( buf, "./group/HgeNumDens/%s_%03i_HgeNumDens_%.2f_%c.dat", All.FilePrefix, ThisTask, All.RedShift, All.Sproj );
        write_img( buf );

        writelog( "\ngroup Hge Number Density ... done.\n" );

    }

/********************Hge Number Density*************************/
    myfree( img );
    myfree( m );

    writelog( "analysis group: %li ... done.\n", index );
    put_block_line;

    fof_free();

}

void analysis(){
    init_analysis();

    if ( (All.GasTemperature ||
                All.GasState || All.GroupFlag ) && All.ReadTemperature == 0  ) {
        compute_temperature();
    }

    if ( All.GasState ) {
        gas_state();
    }

    if ( All.GasDensity )
        gas_density_slice();

    if ( All.GasTemperature )
        gas_temperature_slice();

    if ( All.FoF ) {
        if ( All.FoFRead )
            fof_read();
        else
            fof();
    }

    if ( All.MFFlag )
        mass_function();

    if ( All.GroupFlag )
        group_analysis();

    //tree_build();
    //tree_free();
    //output_rho();
    //vel_value();
    //sort_gas_rho();
    free_analysis();
}
