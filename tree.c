#include "allvars.h"

#ifdef TREE
long last, parent, father, npart;

void tree_allocate() {

    MaxNodes = npart * All.TreeAllocFactor;
    writelog( "TreeAllocateFactor = %g, MaxNodes = %ld\n",
            All.TreeAllocFactor, MaxNodes );
    writelog( "allocate memory for tree\n" );
    mymalloc1( Nodes_Base, ( MaxNodes+1 ) * sizeof( struct NODE ) );
    Nodes = Nodes_Base - NumPart;

    mymalloc1( NextNode, NumPart * sizeof( long ) );
}

void tree_build_single() {
    long i, j, subnode, bits, nfree, n, nn;
    struct NODE *nfreep;
    double max[3], min[3], len, lenhalf;

    writelog( "tree build ...\n" );
    for ( j=0; j<3; j++ ) {
        max[j] = -DBL_MAX;
        min[j] = DBL_MAX;
    }

    for ( i=0; i<NumPart; i++ ) {
        if ( ( 1 << P[i].Type ) & All.TreePartType )
            for ( j=0; j<3; j++ ) {
                max[j] = ( P[i].Pos[j] > max[j] ) ? P[i].Pos[j] : max[j];
                min[j] = ( P[i].Pos[j] < min[j] ) ? P[i].Pos[j] : min[j];
            }
    }

    len = DBL_MIN;

    for ( i=0; i<3; i++ ) {
        len = ( max[i] - min[i] > len ) ? ( max[i] - min[i] ) : len;
    }

    for ( i=0; i<3; i++ ) {
        writelog( "min[%li]=%g, max[%li]=%g\n",
                i, min[i], i, max[i] );
    }

    len *= 1.001;
    writelog( "len=%g\n", len );

    /* initialize first node */
    nfree = NumPart;
    nfreep = &Nodes[nfree];
    for ( j=0; j<3; j++ ) {
        nfreep->center[j] = ( max[j] + min[j] ) * 0.5;
    }
    for ( j=0; j<8; j++ ) {
        nfreep->sons[j] = -1;
    }
    nfreep->len = len;
    nfreep -> bitflags = 0;
    nfree++;
    nfreep++;

    for ( i=0; i<NumPart; i++ ) {
        if ( !( ( 1 << P[i].Type ) & All.TreePartType ) )
            continue;
        //printf( "%i %g\n", P[i].Type, P[i].Mass );
        //printf( "%li\n", i );
        //continue;
        n = NumPart;
        while(1) {
            if ( n >= NumPart ){
                for( j=0, subnode=0, bits=1; j<3; j++, bits<<=1 )
                    subnode += ( P[i].Pos[j] > Nodes[n].center[j] ) ? bits : 0;
                nn = Nodes[n].sons[subnode];
                if ( nn>=0 ) {
                    parent = n;
                    n = nn;
                }
                else {
                    Nodes[n].sons[subnode] = i;
                    //printf( "%li\n", i );
                    break;
                }
            }
            else {

                Nodes[parent].sons[subnode] = nfree;
                nfreep->len = 0.5 * Nodes[parent].len;
                nfreep -> bitflags = 0;
                lenhalf = 0.25 * Nodes[parent].len;

                for ( j=0, bits=1; j<3; j++, bits<<=1 )
                    nfreep->center[j] = Nodes[parent].center[j] + ( (subnode & bits) ? ( lenhalf ) : ( -lenhalf ) );
                for ( j=0; j<8; j++ )
                    nfreep->sons[j] = -1;
                for ( j=0, subnode=0, bits=1; j<3; j++, bits<<=1 )
                    subnode += (( P[n].Pos[j] > nfreep->center[j] ) ? bits : 0);

                /*dealing with particles at identical locations ( or extremely close ). */
                if ( nfreep -> len < 1.0e-3 * SofteningTable[P[n].Type] ) {

                    if ( ThisTask_Local == 0 )
                        printf( "[snapshot: %03i] extremely close particle:\n"
                            "[1][%i][%i]( %.10f, %.10f, %.10f )\n"
                            "[2][%i][%i]( %.10f, %.10f, %.10f )\n",
                            SnapIndex,
                            P[n].ID, P[n].Type, P[n].Pos[0], P[n].Pos[1], P[n].Pos[2],
                            P[i].ID, P[i].Type, P[i].Pos[0], P[i].Pos[1], P[i].Pos[2] );

                    subnode = (int)(((double)rand()) / RAND_MAX * 8);
                    if (subnode >= 8)
                        subnode = 7;
                }

                nfreep->sons[subnode] = n;

                if ( nfree-NumPart >= MaxNodes ){
                    printf( "( %.10f, %.10f, %.10f ), ( %.10f, %.10f, %.10f )\ntype: (%i, %i), ID: (%i, %i)\n",
                            P[n].Pos[0], P[n].Pos[1], P[n].Pos[2],
                            P[i].Pos[0], P[i].Pos[1], P[i].Pos[2],
                            P[n].Type, P[i].Type,
                            P[n].ID, P[i].ID );
                    endrun0( "Task: %i, Max number of tree nodes reached.\n", ThisTask );
                }

                n = nfree;
                nfree++;
                nfreep++;


            }
        }
    }


    writelog( "total tree nodes number: %li\n", nfree );
    writelog( "tree build ... done.\n" );
}

void tree_walk_recursive( long n, long sib, long father ) {
    int i, j;
    long nextsib, p, pp;
    if ( n >= NumPart ) {
        if ( last >= 0 ) {
            if ( last >= NumPart )
                Nodes[last].nextnode = n;
            else
                NextNode[last] = n;
        }
        last = n;
        /*
        for ( i=0; i<8; i++ )
            printf( "%li ", Nodes[n].sons[i] );
        printf( "\n" );
        */
        for ( i=0; i<8; i++ ) {
            if ( (p = Nodes[n].sons[i]) >= 0 ) {
                for ( j=i+1; j<8; j++ )
                    if ( (pp = Nodes[n].sons[j]) >= 0 )
                        break;
                nextsib = ( j<8 ) ? pp : sib;
                tree_walk_recursive( p, nextsib, n );
                Nodes[n].sibling = sib;
                Nodes[n].father = father;
            }
        }
    }
    else {
        if ( last >= 0 ) {
            if ( last >= NumPart ) {
                Nodes[last].nextnode = n;
            }
            else {
                NextNode[last] = n;
            }
        }
        last = n;
    }
}

void tree_walk_test(){
    FILE *fd;
    long n;
    n = NumPart;
    fd = myfopen( "w", "walk.txt" );
    while ( n>=0 ) {
        if ( n<NumPart ){
            fprintf( fd, "%li\n", n );
            n = NextNode[n];
        }
        else {
            n = Nodes[n].nextnode;
        }
    }
    fclose( fd );
}
#endif

void tree_build() {
#if defined(TREE)
    long i;
    /*
    npart = 30;
    All.TreeAllocFactor = 2;
    */

    mytimer_start();
    for ( i=0, npart=0; i<NumPart; i++ )
        if ( ( 1 << P[i].Type ) & All.TreePartType )
            npart ++;

    if ( npart == 0 )
        endruns( "particle number is zero !!!" );

    writelog( "partcle type: ` " );
    for ( i=0; i<6; i++ )
        if ( ( 1 << i ) & All.TreePartType )
            writelog( "%li ", i );
    writelog( "` used in tree build\n" )
    writelog( "total particle number is: %li\n", npart );

    tree_allocate();
    tree_build_single();
    /*
    for ( i=0; i<npart*All.TreeAllocFactor; i++ ) {
        printf( "%2i %2i: ", i, i+npart );
        for ( j=0; j<8; j++ )
            printf( "%3li ", Nodes_Base[i].sons[j] );
        printf( "\n" );
    }
    */
    last = -1;
    writelog( "tree walk ...\n" );
    tree_walk_recursive( NumPart, -1, -1 );
    if ( last >= NumPart ) {
        Nodes[last].nextnode = -1;
    }
    else {
        NextNode[last] = -1;
    }
    //tree_walk_test();
    mytimer_end();
    writelog( "tree walk ... done.\n" );
#endif
}
void tree_free() {
#if defined(TREE)
    writelog( "free memory for tree\n" );
    myfree( Nodes_Base );
    myfree( NextNode );
#endif
}

