#include"allvars.h"

void B_Pdf() {

    double BMin, BMax, DensMin, DensMax,
           LogBMin, LogBMax, LogDensMin, LogDensMax,
           *img, DLogB, DLogDens, LogDens, LogB, sum,
           GlobLogBMin, GlobLogBMax, GlobLogDensMin, GlobLogDensMax,
           B;
    int i, j, k, PicSize;
    char buf[200];

    BMin = DensMin = DBL_MAX;
    BMax = DensMax = DBL_MIN;
    writelog( "plot B PDF ...\n" );

    //PicSize_tmp = All.PicSize;
    PicSize = All.PicSize;

    mymalloc2( img, sizeof( double ) * SQR( PicSize ) );

    for ( i=0; i<N_Gas; i++ ) {

        B = get_B( i ) * 1e6;

       // printf( "%g\n", B );
       //
       /*
       if ( B <= 1e-10 )
           continue;
           */

        if ( B == 0 )
            continue;

        BMin = vmin( B, BMin, 1 );
        BMax = vmax( B, BMax );
        DensMin = vmin( SphP[i].Density/All.RhoBaryon, DensMin, 1 );
        DensMax = vmax( SphP[i].Density/All.RhoBaryon, DensMax );
    }

    LogDensMin = log10( DensMin );
    LogDensMax = log10( DensMax );
    LogBMin = log10( BMin );
    LogBMax = log10( BMax );
    find_global_value( LogDensMin, GlobLogDensMin, MPI_DOUBLE, MPI_MIN );
    find_global_value( LogDensMax, GlobLogDensMax, MPI_DOUBLE, MPI_MAX );
    find_global_value( LogBMin, GlobLogBMin, MPI_DOUBLE, MPI_MIN );
    find_global_value( LogBMax, GlobLogBMax, MPI_DOUBLE, MPI_MAX );

    //LogBMax = 7;

    DLogB = ( LogBMax - LogBMin ) / (PicSize-1);
    DLogDens = ( LogDensMax - LogDensMin ) / (PicSize-1);
    writelog( "DensMin: %g, DensMax: %g, BMin: %g, BMax: %g\n"
            "LogDensMin: %g, LogDensMax: %g, LogBMin: %g, LogBMax: %g\n"
            "GlobLogDensMin: %g, GlobLogDensMax: %g,\nGlobLogBMIn: %g, GlobLogBMax: %g\n",
            DensMin, DensMax, BMin, BMax,
            LogDensMin, LogDensMax, LogBMin, LogBMax,
            GlobLogDensMin, GlobLogDensMax, GlobLogBMin, GlobLogBMax );

    for ( k=0; k<N_Gas; k++ ) {

        LogB = get_B( k ) * 1e6;

        /*
       if ( B <= 1e-10 )
           continue;
           */
        if ( LogB == 0 )
            continue;

        LogB = log10( LogB );

        //if ( LogB > LogBMax )
        //    continue;

        LogDens = SphP[k].Density / All.RhoBaryon;
        if ( LogDens <= 0 )
            endrun( 20181107 );
        LogDens = log10( LogDens );

        i = ( LogB - LogBMin ) / DLogB;
        check_picture_index( i );

        j = ( LogDens - LogDensMin ) / DLogDens;
        check_picture_index( j );

        img[ i*PicSize + j ]++;
     //  printf( "%g ", img[ i*PicSize +j ] ) ;
    }

    for ( i=0, sum=0; i<SQR(PicSize); i++ )
        sum += img[i];

    writelog( "sum: %g\n", sum );

    for ( i=0; i<SQR(PicSize); i++ )
        img[i] /= sum;

    create_dir( "./BPdf" );
    sprintf( buf, "./BPdf/BPdf_%.2f.dat", All.RedShift );

    image.img = img;
    img_xmin = LogDensMin;
    img_xmax = LogDensMax;
    img_ymin = LogBMin;
    img_ymax = LogBMax;
    write_img2( buf, "BPdf" );
    myfree( img );

    //All.PicSize = PicSize_tmp;
    put_block_line;

}