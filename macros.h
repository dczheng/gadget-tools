#define writelog( fmt, ... ) { \
    fprintf( LogFileFd, fmt, ##__VA_ARGS__ ); \
    if ( ThisTask == (NTask - 1) ) { \
        printf( fmt, ##__VA_ARGS__ ); \
    }\
}

#define vmax( a, b ) ( ( a > b ) ? a : b )
#define vmin( a, b, mode) ( mode == 0 ) ? ( ( a > b ) ? b : a ) : ( ( a > b && b > 0 ) ? b : a )
#define check_picture_index( i )  i = ( ( i<0 || i>=All.PicSize ) ? ( (i<0) ? 0 : All.PicSize-1 ) : i )
#define PERIODIC( x ) ( ( x > All.HalfBoxSize || x < -All.HalfBoxSize ) ? ( ( x > All.HalfBoxSize ) ? ( x - All.BoxSize ) : ( x + All.BoxSize )  ) : x )
#define NGB_PERIODIC( x ) ( (fabs(x) > All.HalfBoxSize) ? ( All.BoxSize-fabs(x) ) : fabs(x) )

#define do_sync( a ) { \
    put_block_line; \
    put_block_line; \
    writelog( "synchronization after `%s` ... \n", a ); \
    MPI_Barrier( MPI_COMM_WORLD ); \
    put_block_line; \
    put_block_line; \
}


#define DATA_SWAP( a, b, tmp ) {\
   tmp = a; \
   a = b; \
   b = t1; \
}

#define put_block_line    writelog( sep_str )

#define find_global_value( a, A, type, op ) { \
    MPI_Reduce( &a, &A, 1, type, op, 0, MPI_COMM_WORLD ); \
    MPI_Bcast( &A, 1, type, 0, MPI_COMM_WORLD ); \
    MPI_Barrier( MPI_COMM_WORLD ); \
}

#define check_var_num() {\
    if ( ms.nn > MALLOC_VAR_NUM ) {\
        writelog( "MALLOC_VAR_NUM IS TOO SMALL ...\n" );\
        endrun( 20180430 ); \
    }\
}

#define check_var_len( a ) {\
    if ( strlen( #a ) > MALLOC_VAR_LEN ) {\
        writelog( "MALLOC_VAR_LEN IS TOO SMALL ...\n" );\
        endrun( 20180430 ); \
    }\
}

#define malloc_report() { \
    sprintf( ms.str, "memory info:" );\
    if ( ms.mem > CUBE( 1024 ) ) {\
        sprintf( ms.str, "%s Total %g Gb,", ms.str, ms.mem / CUBE( 1024. )  );\
    }\
    else if ( ms.mem > SQR( 1024 ) ) {\
        sprintf( ms.str, "%s Total %g Mb,", ms.str, ms.mem / SQR( 1024. )  );\
    }\
    else if( ms.mem > 1024 ) {\
        sprintf( ms.str, "%s Total %g Kb,", ms.str, ms.mem / 1024. );\
    }\
    else {\
        sprintf( ms.str, "%s Total %li b,", ms.str, ms.mem );\
    }\
\
    if ( ms.max_mem > CUBE( 1024 ) ) {\
        sprintf( ms.str, "%s Max %g Gb,", ms.str, ms.max_mem / CUBE( 1024. ) );\
    }\
    else if ( ms.max_mem > SQR( 1024 ) ) {\
        sprintf( ms.str, "%s Max %g Mb,", ms.str, ms.max_mem / SQR( 1024. ) );\
    }\
    else if( ms.max_mem > 1024 ) {\
        sprintf( ms.str, "%s Max %g Kb,", ms.str, ms.max_mem / 1024. );\
    }\
    else {\
        sprintf( ms.str, "%s Max %li b,", ms.str, ms.max_mem );\
    }\
\
    sprintf( ms.str, "%s Nvars %li.\n", ms.str, ms.nn );\
    fprintf( MemUseFileFd, ms.str ); \
}

#define mymalloc( a, n, flag, b ) {\
    if ( !(a = malloc( n )) ) { \
        if ( n > CUBE( 1024 ) ) {\
            writelog( "Failed to allocate memory for `%s` ( %g Gb )\n", #a, n / CUBE( 1024. ) ); \
        }\
        else if ( n > SQR( 1024 ) ) {\
            writelog( "Failed to allocate memory for `%s` ( %g Mb )\n", #a, n / SQR( 1024. ) ); \
        }\
        else if( n > 1024 ) {\
            writelog( "Failed to allocate memory for `%s` ( %g Kb )\n", #a, n /  1024. ); \
        }\
        else {\
            writelog( "Failed to allocate memory for `%s` ( %li b )\n", #a, n ); \
        }\
        endrun( 20180430 ); \
    }\
\
    if ( n > CUBE( 1024 ) ) {\
        fprintf( MemUseFileFd, "allocate memory for `%s` ( %g Gb )\n", #a, n / CUBE( 1024. ) ); \
    }\
    else if ( n > SQR( 1024 ) ) {\
        fprintf( MemUseFileFd, "allocate memory for `%s` ( %g Mb )\n", #a, n / SQR(  1024. ) ); \
    }\
    else if( n > 1024 ) {\
        fprintf( MemUseFileFd, "allocate memory for `%s` ( %g Kb )\n", #a, n /  1024. ); \
    }\
    else {\
        fprintf( MemUseFileFd, "allocate memory for `%s` ( %li b )\n", #a, n ); \
    }\
\
    if ( flag == 1 ){\
        fprintf( MemUseFileFd, "initialize `%s` ...\n", #a ); \
        memset( a, b, n ); \
    }\
    check_var_len( a );\
    sprintf( ms.var[ms.nn], "%s", #a );\
    ms.var_bytes[ms.nn] = n;\
    ms.nn++; \
    ms.mem += n; \
    ms.max_mem = vmax( ms.max_mem, ms.mem ); \
    check_var_num();\
    malloc_report(); \
    fprintf( MemUseFileFd, sep_str ); \
}

#define mymalloc1( a, n )  mymalloc( a, n, 0, 0 )
#define mymalloc2( a, n )  mymalloc( a, n, 1, 0 )
#define mymalloc3( a, n, b )  mymalloc( a, n, 1, b )

#define myfree( a ) {\
    for ( ms.i=0; ms.i<ms.nn; ms.i++ ) {\
        if ( !( strcmp( ms.var[ms.i], #a ) ) ) {\
            ms.b = ms.var_bytes[ms.i];\
            break;\
        }\
    }\
\
    if ( ms.b > CUBE( 1024 ) ) {\
        fprintf( MemUseFileFd, "Free memory for `%s` ( %g Gb )\n", #a, ms.b / CUBE( 1024. ) ); \
    }\
    else if ( ms.b > SQR( 1024 ) ) {\
        fprintf( MemUseFileFd, "Free memory for `%s` ( %g Mb )\n", #a, ms.b / SQR(  1024. ) ); \
    }\
    else if( ms.b > 1024 ) {\
        fprintf( MemUseFileFd, "Free memory for `%s` ( %g Kb )\n", #a, ms.b /  1024. ); \
    }\
    else {\
        fprintf( MemUseFileFd, "Free memory for `%s` ( %li b )\n", #a, ms.b ); \
    }\
\
    for ( ; ms.i<ms.nn-1; ms.i++ ) {\
        sprintf( ms.var[ms.i], "%s",  ms.var[ms.i+1] ); \
        ms.var_bytes[ms.i] = ms.var_bytes[ms.i+1]; \
    }\
    ms.nn--; \
    ms.mem -= ms.b; \
    malloc_report(); \
    fprintf( MemUseFileFd, sep_str ); \
}

#define endrun0( fmt, ... ) {\
    fprintf( stderr, fmt, ##__VA_ARGS__ ); \
    fprintf( stderr, "END IN: ( %s %s %i )\n" , __FILE__, __FUNCTION__, __LINE__ ); \
    MPI_Abort( MPI_COMM_WORLD, 0 ); \
    exit( 0 ); \
}

#define endruns( s ) {\
    endrun0( "%s\n", s ); \
}

#define endrun( i ) {\
    endrun0( "%i\n", i ); \
}


#define timer_start() \
    double t0, t1; \
    t0 = second();

#define timer_end() \
    t1 = second(); \
    writelog( "Time: %g sec\n", t1 - t0 );

