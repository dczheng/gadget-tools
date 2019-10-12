#include "allvars.h"

void slice_init() {

    int pt;
    long offset, num, index, i;
    ParticleData p_tmp;
    SphParticleData sphp_tmp;

    writelog( "determine slice info ...\n" );

    if ( End[proj_i] - Start[proj_i] !=
            End[proj_j] - Start[proj_j] )
        endruns( "Projection Region Must Be Square!..." );

    if ( End[0] == 0 ){
        Start[0] = 0;
        End[0] = BoxSize;
    }

    if ( End[1] == 0 ){
        Start[1] = 0;
        End[1] = BoxSize;
    }

    if ( End[2] == 0 ){
        Start[2] = 0;
        End[2] = BoxSize;
    }

    writelog( "StartX: %g, EndX: %g\n"
            "StartY: %g, EndY: %g\n"
            "StartZ: %g, EndZ: %g\n",
            Start[0], End[0],
            Start[1], End[1],
            Start[2], End[2] );

    for ( pt=0; pt<6; pt++ ) {

        offset = OffsetPart6[pt];
        num = NumPart6[pt];
        //printf( "%li %li\n", offset, num );
        index = offset;

        if ( num == 0 ) {
            SliceStart[pt] = -1;
            SliceEnd[pt] = -1;
            continue;
        }

        writelog( "particle %i: offset=%li, num=%li\n",
                pt, offset, num );

        for ( i=offset; i<offset+num; i++ ) {
            if ( P[i].Pos[0] >= Start[0] &&
                 P[i].Pos[0] <= End[0] &&
                 P[i].Pos[1] >= Start[1] &&
                 P[i].Pos[1] <= End[1] &&
                 P[i].Pos[2] >= Start[2] &&
                 P[i].Pos[2] <= End[2] ) {

                p_tmp = P[index];
                P[index] = P[i];
                P[i] = p_tmp;

                if ( pt == 0 ) {
                    sphp_tmp = SphP[ index ];
                    SphP[index] = SphP[i];
                    SphP[i] = sphp_tmp;
                }

                index ++;
            }
        }

        SliceStart[pt] = offset;
        SliceEnd[pt] = index;
    }
    writelog( "Slice Start: " );

    for ( pt=0; pt<6; pt++ ) {
        writelog( "%ld ", SliceStart[pt] );
    }

    writelog( "\n" );
    writelog( "Slice End: " );

    for ( pt=0; pt<6; pt++ ) {
        writelog( "%ld ", SliceEnd[pt] );
    }
    writelog( "\n" );

    writelog( "determine slice info ... done.\n" );

}

void make_slice_img( int pt, double *data, long NPart, double *weight ) {

    double *img, *num, dx, dy, x, y, h, dh, lx, ly, v, w;
    int i, xi, yi, N, Nhalf, i1, i2, j1, j2, li, lj;
    long start, end;

    writelog( "make slice imgage  ...\n" );

    reset_img();
    img = image.img;
    num = image.num;

    dx = dy = (End[proj_i] - Start[proj_i])/ PicSize;
    //writelog( "dx: %f, dy: %f\n", dx, dy  );

    N = All.KernelN;
    Nhalf = N / 2;
    h = SofteningTable[pt];
    dh = h / Nhalf;

    if ( NPart ) {
        start = 0;
        end = NPart;
    }
    else {
        start = SliceStart[pt];
        end = SliceEnd[pt];
    }

    for ( i=start; i<end; i++ ) {

        if ( NPart ) {
            x = data[i*3];
            y = data[i*3+1];
            v = data[i*3+2];
            if ( weight ) {
                w = weight[i];
                v *= w;
            }
            else {
                w = 1;
            }
        }
        else {
            x = P[i].Pos[proj_i];
            y = P[i].Pos[proj_j];
            x -= Start[proj_i];
            y -= Start[proj_j];
            v = data[ i-SliceStart[pt] ];
            if ( weight ) {
                w = weight[i];
                v *= w;
            }
            else
                w = 1;
        }

        //printf( "%g\n", v );
        xi = x / dx;
        yi = y / dy;
        //printf( "%i, %i\n", xi, yi );
        check_picture_index( xi );
        check_picture_index( yi );

        if ( All.KernelInterpolation == 0 ){
            img[ xi * PicSize + yi ] += v;
            num[ xi * PicSize + yi ] += w;
            continue;
        }

        i1 = (int)(( x-h ) / dx);
        i2 = (int)(( x+h ) / dx);
        j1 = (int)(( y-h ) / dy);
        j2 = (int)(( y+h ) / dy);


        if ( i1 != xi || i2 != xi || j1 != yi || j2 != yi ) {
            /*
            writelog( "(%f, %f ), ( %i, %i), (%f, %f, %f, %f), (%i, %i, %i, %i)\n",
                x, y, xi, yi, x-h, x+h, y-h, y+h, i1, i2, j1, j2 );
                */
            for ( li=0; li<N; li++ )
                for ( lj=0; lj<N; lj++ ){
                        lx = x + ( li-Nhalf ) * dh;
                        ly = y + ( lj-Nhalf ) * dh;
                        i1 = lx / dx;
                        j1 = ly / dy;
                        if ( i1 < 0 || i1 >= PicSize ||
                                j1 < 0 || j1 >= PicSize ) continue;
                        img[ i1 * PicSize + j1 ] += v * KernelMat2D[pt][ li*N + lj ];
                        num[ i1 * PicSize + j1 ] += w * KernelMat2D[pt][ li*N + lj ];
                        //writelog( "%f %f\n", v, v * KernelMat2D[pt][li*N+lj] );
                }

        }
        else {
            img[ xi * PicSize + yi ] += v;
            num[ xi * PicSize + yi ] += w;
        }
        //printf( "%g\n", img[ xi * PicSize + yi ] );

    }

    for ( i=0; i<SQR(All.PicSize); i++ )
        if ( num[i] != 0 )
            img[i] /= num[i];

#ifdef UNITAREASLICE
        for ( i=0; i<SQR(All.PicSize); i++ )
            img[i] /= SQR(dx);
#endif

    img_xmin = Start[ proj_i ];
    img_xmax = End[ proj_i ];
    img_ymin = Start[ proj_j ];
    img_ymax = End[ proj_j ];

}

void field_slice( int pt, double *data, char *name, long N, double *weight ) {

    char buf[100];

    sprintf( buf, "`%s` slice\n", name );
    writelog( buf );

    create_dir( "%s%s", OutputDir, name );
    sprintf( buf, "%s%s/%s_%03i.dat", OutputDir, name, name, SnapIndex );

    if ( N )
        make_slice_img( 0, data, N, weight );
    else
        make_slice_img( pt, data, 0, weight );

    write_img( buf );

}

#ifdef BSLICE
void mag_slice() {
    int num, i;
    double *data, *weight;

    num = SliceEnd[0] - SliceStart[0];
    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( weight, sizeof( double ) * num );
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        data[i] = get_B( i ) * 1e6;
        weight[i] = SphP[i].Density;
    }
    field_slice( 0, data, "MagneticField", 0, weight );
    myfree( data );
    myfree( weight );
}
#endif

#ifdef MACHSLICE
void mach_slice() {
    int num, i;
    double *data, *weight;

    num = SliceEnd[0] - SliceStart[0];
    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( weight, sizeof( double ) * num );
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        data[i] = SphP[i].MachNumber;
        weight[i] = SphP[i].Density;
    }
    field_slice( 0, data, "MachNumber", 0, weight );
    myfree( data );
    myfree( weight );
}
#endif

#ifdef DENSITYSLICE
void density_slice() {
    int num, i;
    long index;
    double *data, *data3;

    num = SliceEnd[0] - SliceStart[0];
    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( data3, sizeof( double ) * num * 3 );

    index = 0;
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        data[i] = SphP[i].Density / Time3 / RhoBaryon;
        //data3[3*index] = P[i].Pos[proj_i] - Start[proj_i];
        //data3[3*index+1] = P[i].Pos[proj_j] - Start[proj_j];
        //data3[3*index+2] = SphP[i].Density / Time3 / RhoBaryon;
        index ++;
    }

    field_slice( 0, data, "Density", 0, NULL );
    //field_slice( 0, data3, "Density", index, NULL );

    index = 0;
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        if ( SphP[i].Temp >= 1e7 ) {
            data3[3*index] = P[i].Pos[proj_i] - Start[proj_i];
            data3[3*index+1] = P[i].Pos[proj_j] - Start[proj_j];
            data3[3*index+2] = SphP[i].Density / Time3 / RhoBaryon;
            index ++;
        }
    }
    field_slice( 0, data3, "Density_hot", index, NULL );

    index = 0;
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        if ( SphP[i].Temp < 1e7 && SphP[i].Temp >= 1e5 ) {
            data3[3*index] = P[i].Pos[proj_i] - Start[proj_i];
            data3[3*index+1] = P[i].Pos[proj_j] - Start[proj_j];
            data3[3*index+2] = SphP[i].Density / Time3 / RhoBaryon;
            index ++;
        }
    }
    field_slice( 0, data3, "Density_warm-hot", index, NULL );

    index = 0;
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        if ( ( SphP[i].Density / Time3 / RhoBaryon ) < 1e3 && SphP[i].Temp < 1e5 ) {
            data3[3*index] = P[i].Pos[proj_i] - Start[proj_i];
            data3[3*index+1] = P[i].Pos[proj_j] - Start[proj_j];
            data3[3*index+2] = SphP[i].Density / Time3 / RhoBaryon;
            index ++;
        }
    }
    field_slice( 0, data3, "Density_diffuse", index, NULL );

    index = 0;
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        if ( ( SphP[i].Density / Time3 / RhoBaryon ) >= 1e3 && SphP[i].Temp < 1e5 ) {
            data3[3*index] = P[i].Pos[proj_i] - Start[proj_i];
            data3[3*index+1] = P[i].Pos[proj_j] - Start[proj_j];
            data3[3*index+2] = SphP[i].Density / Time3 / RhoBaryon ;
            index ++;
        }
    }
    field_slice( 0, data3, "Density_condensed", index, NULL );

    myfree( data3 );
    myfree( data );
}
#endif

#ifdef TEMPSLICE
void temperature_slice() {
    int num, i;
    double *data, *weight;

    num = SliceEnd[0] - SliceStart[0];
    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( weight, sizeof( double ) * num );
    for ( i=SliceStart[0]; i<num; i++ ) {
        data[i] = SphP[i].Temp;
        weight[i] = SphP[i].Density;
    }
    field_slice( 0, data, "Temperature", 0, weight );
    myfree( data );
    myfree( weight );
}
#endif

#ifdef CRENSLICE
void cren_slice() {
    int num, i;
    double *data, *weight;

    num = SliceEnd[0] - SliceStart[0];
    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( weight, sizeof( double ) * num );
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        data[i] = SphP[i].CRE_n * SphP[i].Density / guc.m_e / CUBE( g2c.cm );
        weight[i] = SphP[i].Density;
    }
    field_slice( 0, data, "cre_n", 0, weight );
    myfree( data );
    myfree( weight );
}
#endif

#ifdef CREESLICE
void cree_slice() {
    int num, i;
    double *data, *weight;

    num = SliceEnd[0] - SliceStart[0];
    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( weight, sizeof( double ) * num );
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        data[i] = SphP[i].CRE_e / SphP[i].u;
        weight[i] = SphP[i].Density;
    }
    field_slice( 0, data, "cre_e", 0, weight );
    myfree( data );
    myfree( weight );
}
#endif

#ifdef RADSLICE
void radio_slice() {

    double dnu, *data, x, area, frac, *weight;
    int num, index, index1, i;
    char buf[20];

    dnu = log( All.NuMax/All.NuMin ) / ( All.NuNum-1 );
    num = SliceEnd[0] - SliceStart[0];

    x = log( All.RadSliceFreq/All.NuMin ) / dnu;
    index = (int)(x);

    if ( index >= All.NuNum || index < 0 ) {
        endrun( 20190711 );
    }

    x -= index;

    area = (End[proj_i] - Start[proj_i]) / PicSize;
    area = SQR( area );

    index1 = ( index == All.NuNum-1 ) ? All.NuNum-1 : index + 1;

    mymalloc2( data, sizeof( double ) * num );
    mymalloc2( weight, sizeof( double ) * num );

    frac = 1.0 / (4.0 * PI * SQR( LumDis * g2c.cm )) / ( area / SQR(ComDis) );
    for ( i=SliceStart[0]; i<SliceEnd[0]; i++ ) {
        data[i] = exp (
                log( get_particle_radio_index(i, index) ) * ( 1-x )
              + log( get_particle_radio_index(i, index1) ) * x
               ) * frac;
        weight[i] = SphP[i].Density;
    }


    sprintf( buf, "radio_%.2f", All.RadSliceFreq );
    field_slice( 0, data, buf, 0, weight );

    myfree( data );
    myfree( weight );

}
#endif
