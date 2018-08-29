#include "allvars.h"

void global_init() {
    init_sep_str();
    inte_ws = gsl_integration_workspace_alloc( GSL_INTE_WS_LEN );
}

void global_free() {
    gsl_integration_workspace_free( inte_ws );
}

int main( int argc, char *argv[] ){
    time_t time1, time2;
    long dtime;
    struct tm *tb;
    char buf[100];

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &ThisTask );
    MPI_Comm_size( MPI_COMM_WORLD, &NTask );

    if ( ThisTask == 0 ) {
        if ( argc < 2 ) {
            printf( "Parameters file is required on command line!\n " );
            endrun( 1 );
        }
        if ( access( argv[1], 0 ) == -1 ) {
            printf( "Parameters file `%s` is invalid!\n", argv[1] );
            endrun( 20180516 );
        }
    }

    time1 = time( NULL );
    tb = localtime( &time1 );

    global_init();

    if ( ThisTask == 0 ){
        printf( "%s", sep_str );
        if ( access( "./gadget-analysis.log/", 0 ) == -1 ) {
            printf( "create directory ./gadget-analysis.log/ by task 0\n" );
            if ( mkdir( "./gadget-analysis.log/", 0755 ) == -1 ) {
                printf( "failed create directory ./gadget-analysis.log/\n" );
                endrun( 20171203 );
            }
        }
    }

    MPI_Barrier( MPI_COMM_WORLD );
    sprintf( buf, "./gadget-analysis.log/gadget-analysis-%03d.log", ThisTask );
    LogFileFd = fopen( buf, "w" );


    sprintf( buf, "./gadget-analysis.log/memuse-%03d.txt", ThisTask );
    MemUseFileFd = fopen( buf, "w" );

    writelog( "open log file\n" );
    writelog( "Start At: %s", asctime(tb) );

#ifdef ZDEBUG
    writelog( "Assign `SIGSEGV` to signal_hander function.\n" );
    signal( SIGSEGV, signal_hander );
    init_sig();
#endif

    put_block_line;
    All.ToolsPath = getenv( "GADGET_TOOLS" );
    if ( strcmp( All.ToolsPath, "" ) == 0 ){
        writelog( "Please set `GADGET_TOOLS` evironment variable.\n" );
        put_block_line;
        endrun( 20180513 );
    }
    writelog( "GADGET_TOOLS: %s\n", All.ToolsPath );
    put_block_line;

    /******************read***********************/
    read_parameters( argv[1] );
    read_snapshot();
    /******************read***********************/

    set_units();
    compute_cosmo_quantities();

    /******************analysis***********************/
    analysis();
    /******************analysis***********************/
    MPI_Barrier( MPI_COMM_WORLD );

    free_particle_memory();

    MPI_Barrier( MPI_COMM_WORLD );

    time2 = time( NULL );
    tb = localtime( &time2 );

    writelog( "End At: %s", asctime(tb) );

    dtime = (long) ( difftime( time2, time1 ) );
    writelog( "Total Time %li sec.\n", dtime );
    put_block_line;

    fclose( LogFileFd );
    fclose( MemUseFileFd );

    global_free();

    MPI_Finalize();
    return 0;
}

