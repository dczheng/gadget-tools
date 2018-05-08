#include "hdf5.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "math.h"
#include "time.h"
#include "dirent.h"
#include "gsl/gsl_integration.h"
#include "gsl/gsl_sf_gamma.h"
#include "mpi.h"
#include "signal.h"
#include "limits.h"
#include "sys/stat.h"
#include "protos.h"
#include "macros.h"
#include "omp.h"

#define ZDEBUG

#define MAX_PARA_FILE_LINE_LEN 200
#define SEP_LEN 80
#define IO_NBLOCKS 100
//#define LONGIDS
#define GENERATIONS 8

#define SFR
#define BLACK_HOLES
#define SEC_PER_MEGAYEAR 3.155e13

#define PROTONMASS               1.6726e-24
#define ELECTRON_MASS            9.10953e-28
#define ELECTRON_CHARGE          4.8032e-10
#define BOLTZMANN                1.38066e-16
#define LIGHT_SPEED              2.9979e10
#define THOMSON_CROSS_SECTION    6.65246e-25
#define GRAVITY                  6.672e-8
#define HUBBLE                   3.24077789e-18  /* in h/sec */
#define PI M_PI
#define PC                       3.0857e18
#define KPC                      3.0857e21
#define MPC                      3.0857e24
#define HYDROGEN_MASSFRAC        0.76
#define GAMMA                    ( 5.0 / 3.0 )
#define GAMMA_MINUS1             ( GAMMA - 1 )

#define GSL_INTE_WS_LEN 1000
#define GSL_INTE_ERR_ABS 0.0
#define GSL_INTE_ERR_REL 1e-3
#define GSL_INTE_KEY GSL_INTEG_GAUSS61
extern gsl_integration_workspace *inte_ws;

#define SQR(X) ( X*X )
#define CUBE(X) ( X*X*X )

#ifdef LONGIDS
typedef unsigned long MyIDType;
#else
typedef unsigned int MyIDType;
#endif

#ifdef OUTPUT_IN_DOUBLEPRECISION
typedef double MyOutputFloat;
typedef double MyFloat;
typedef double MyDouble;
typedef double MyDoublePos;
#else
typedef float MyOutputFloat;
typedef float MyFloat;
typedef float MyDouble;
typedef float MyDoublePos;
#endif

struct group_struct{
  int Len;
  unsigned int Offset;
  MyIDType MinID;
  MyIDType MinIDTask;
  int GrNr;
#ifndef FOF_EXTENDED_PROPERTIES
  int LenType[6];
  MyOutputFloat MassType[6];
#endif
  MyOutputFloat Mass;
  MyOutputFloat CM[3];
  MyOutputFloat Vel[3];
  MyDoublePos FirstPos[3];
#ifdef SFR
  double Sfr;
#endif
#ifdef BLACK_HOLES
  MyOutputFloat BH_Mass;
  MyOutputFloat BH_Mdot;
  MyOutputFloat MaxDens;
  int index_maxdens, task_maxdens;
#endif

#ifdef SUBFIND
  int Nsubs;
  int FirstSub;
  MyDoublePos Pos[3];
  MyOutputFloat M_TopHat200, R_TopHat200;
  MyOutputFloat M_Mean200, R_Mean200;
  MyOutputFloat M_Crit200, R_Crit200;
#ifdef SO_VEL_DISPERSIONS
  MyOutputFloat VelDisp_TopHat200, VelDisp_Mean200, VelDisp_Crit200;
#endif
#ifdef SO_BAR_INFO
  MyOutputFloat M_Mean500, R_Mean500;
  MyOutputFloat M_Crit500, R_Crit500;
  MyOutputFloat M_Crit2500, R_Crit2500;
#ifdef SO_VEL_DISPERSIONS
  MyOutputFloat VelDisp_Mean500, VelDisp_Crit500, VelDisp_Crit2500;
#endif
#endif
  int ContaminationLen;
  MyOutputFloat ContaminationMass;
#ifdef SO_BAR_INFO
  MyOutputFloat gas_mass[6], star_mass[6], temp[6], xlum[6], ygas[6];
#endif
#endif

#ifdef FOF_EXTENDED_PROPERTIES
  MyOutputFloat VelDisp, Rmax, Vmax;
  MyOutputFloat ToI[9];
  MyOutputFloat AngMom[9];
  MyOutputFloat Pos[3];
  unsigned short Origintask;
#endif

};

struct io_header{
  int npart[6];
  double mass[6];
  double time;
  double redshift;
  int flag_sfr;
  int flag_feedback;
  unsigned int npartTotal[6];
  int flag_cooling;
  int num_files;
  double BoxSize;
  double Omega0;
  double OmegaLambda;
  double HubbleParam;
  int flag_stellarage;
  int flag_metals;
  unsigned int npartTotalHighWord[6];
  int flag_entropy_instead_u;
  int flag_doubleprecision;
  int flag_ic_info;
  float lpt_scalingfactor;
  char fill[18];
  char names[15][2];
};

enum iofields {
    IO_ID,
    IO_POS,
    IO_MASS,
    IO_VEL,
    IO_ACCEL,
    IO_MAG,
    IO_NE,
    IO_U,
    IO_RHO,
    IO_POT,
    IO_MN,
    IO_CR_C0,
    IO_CR_Q0,
    IO_CR_E0,
    IO_CR_n0,
    IO_CRE_C0,
    IO_CRE_Q0,
    IO_CRE_E0,
    IO_CRE_n0,
    IO_DIVB,
    IO_DBDT,
    IO_TEMP
};


extern struct io_header header;
extern struct group_struct *group;

extern struct particle_data {
    MyFloat Pos[3];
    MyFloat Mass;
    MyFloat Vel[3];
    MyFloat Pot;
    MyFloat Acc[3];
    MyIDType ID;
    int Type;
} *P;

extern struct sph_particle_data {
    MyFloat u;
    MyFloat Density;
    MyFloat Hsml;
    MyFloat NumNgb;
    MyFloat Pressure;
    MyFloat HydroAccel[3];
    MyFloat MachNumber;
    MyFloat CR_C0;
    MyFloat CR_Q0;
    MyFloat CR_n0;
    MyFloat CR_E0;
    MyFloat CRE_C0;
    MyFloat CRE_Q0;
    MyFloat CRE_E0;
    MyFloat CRE_n0;
    MyFloat B[3];
    MyFloat divB;
    MyFloat dBdt;
    MyFloat elec;
    MyFloat Temp;
    double vL;
    double P;
} *SphP;

extern char sep_str[ SEP_LEN ];
extern int ThisTask, NumTask;
extern long TotNgroups, *id_to_index, NumPart, N_Gas;
extern FILE *LogFileFd;

extern struct global_parameters_struct {
    char FilePrefix[ FILENAME_MAX ], GroupDir[ FILENAME_MAX ],
         FofFileName[ FILENAME_MAX ],
         LogFile[ FILENAME_MAX ];
    int StartSnapIndex, MpcFlag, ProjectDirection, KernelN,
        PicSize, PicSize2, NumFiles, HgeFlag, CrFlag, BFlag,
        GasState, GasDensity, GasTemperature, KernelInterpolation,
        ReadTemperature, FofMinLen, proj_i, proj_j, proj_k,
        TreePartType, ConvN, ConvFlag, GroupIndex;
    double SofteningTable[6], Alpha ,
           UnitTime_in_s,
           UnitMass_in_g,
           UnitLength_in_cm,
           UnitDensity_in_cgs,
           UnitEnergy_in_cgs,
           UnitPressure_in_cgs,
           UnitVelocity_in_cm_per_s,
           Start[3], End[3],
           TreeAllocFactor, LinkLength,
           BoxSize, HalfBoxSize,
           *KernelMat2D[6], *KernelMat3D[6],
           Time, Hubble_a, RhoBaryon,
           RedShift, HubbleParam, RhoCrit, G,
           Hubble, Omega0, OmegaLambda, OmegaBaryon,
           *ConvKernel, ConvSigma;
    long SliceStart[6], SliceEnd[6];
}All;

#define IMG_PROPS_START 11
#define IMG_PROPS_OTHERS ( All.PicSize - IMG_PROPS_START )
#define img_nprops     ( images.nprops[0] )
#define img_xmin       ( image.props[1] )
#define img_xmax       ( image.props[2] )
#define img_ymin       ( image.props[3] )
#define img_ymax       ( image.props[4] )
#define img_proj       ( image.props[5] )
#define img_z          ( image.props[6] )
#define img_min        ( image.props[7] )
#define img_max        ( image.props[8] )
#define img_globmin    ( image.props[9] )
#define img_globmax    ( image.props[10] )
#define img_props(i)   ( image.props[ IMG_PROPS_START+i ] )
struct image_struct{
    double *data, *img,
           *props;
    /* props:
     *      0 nprops
     *      1 xmin
     *      2 xmax
     *      3 ymin
     *      4 ymax
     *      5 projctdirection
     *      6 redshift
     *      7 min_i
     *      8 max_i
     *      9 globmin_i
     *      10 globmax_i
     *      11 ~ end others ... */

};
extern struct image_struct image;

extern struct NODE {
    double center[3];
    double len;
    long suns[8], sibling, father;
    long nextnode;
} *Nodes, *Nodes_Base;
extern long MaxNodes;
extern long *NextNode;
extern long *Ngblist;

struct fof_properties{
    double mass, cm[3], vr200, vel[3];
    long Head, Tail, Len;
};

extern struct fof_properties *FoFProps;
extern long *FoFNext;
extern int Ngroups;

extern struct gadget_2_cgs_unit{
    double cm, g, s, erg;
}g2c;

#define MALLOC_VAR_NUM 1000
#define MALLOC_VAR_LEN 200

struct malloc_struct {
    char var[MALLOC_VAR_NUM][MALLOC_VAR_LEN], str[100];
    long mem, var_bytes[MALLOC_VAR_NUM],
       i, nn, b, max_mem;
};

extern struct malloc_struct ms;

#ifdef ZDEBUG
void signal_hander( int s );
void init_sig();
void empty_sig_buf();
#define ZDEBUG_NUM 10

struct sig_struct{
    char stop[500];
    char buf[ZDEBUG_NUM][500];
    int i, j;
    double d[ZDEBUG_NUM];
} sig;

#define RAISE_SIGSTOP() { \
    sprintf( sig.stop, "%s %s %i" , __FILE__, __FUNCTION__, __LINE__ ); raise( SIGSEGV );\
}

#define ZSPRINTF( k, fmt, ... ) { \
    if ( k >= ZDEBUG_NUM ) { \
        sprintf( sig.buf[0], "%d is too large.", k ); \
        RAISE_SIGSTOP(); \
    } \
    sprintf( sig.buf[k], fmt, ##__VA_ARGS__ ); \
}

#define ZCLEAR() { empty_sig_buf();  }

#else

#define ZSPRINTF( k, fmt, ... )
#define ZCLEAR()

#endif
