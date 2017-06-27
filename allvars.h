#include "hdf5.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "plplot.h"
#include "plConfig.h"
#include "unistd.h"
#include "math.h"

#define MAX_PARA_FILE_LINE_LEN 200
#define SEP_LEN 50
#define IO_NBLOCKS 10
#define LONGIDS

#ifdef LONGIDS
typedef unsigned long long MyIDType;
#else
typedef unsigned int MyIDType;
#endif

struct Particle_Struct {
    float *pos;
    float *vel;
    float *accel;
    float *mag;
    float *elec;
    MyIDType *id;
    long num;
    float *pot;
    float *m;
    float *u;
    float *rho;
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
    IO_POS,
    IO_VEL,
    IO_ACCEL,
    IO_MAG,
    IO_MASS,
    IO_U,
    IO_RHO,
    IO_POT,
    IO_ELEC,
    IO_ID
};



extern struct Particle_Struct Particle[6];
extern struct io_header header;

extern char Para_file[ FILENAME_MAX ];
extern char file_Prefix[ FILENAME_MAX ];
extern char  Out_file[ FILENAME_MAX ];
extern char  Out_Picture_Prefix[ FILENAME_MAX ];
extern char sep_str[ SEP_LEN ];
extern int Num_files;
extern int slice_num, slice_index_num, *slice_index, pic_xsize, pic_ysize;
extern float redshift;


extern hid_t hdf5_file, hdf5_group, hdf5_dataset, hdf5_dataspace, hdf5_dataspace_in_file, hdf5_dataspace_in_memory, hdf5_type, hdf5_hdf5_type_mem, hdf5_attribute, hdf5_type;
extern herr_t herr;
extern hsize_t dims[2], maxdims[2], npoints, precision;
extern int ndims;

void show_header( struct io_header header );
void read_header();
void read_all_data();
void free_all_memory();
void write_file( char *fn, struct io_header header, struct Particle_Struct *Particle);
void Plot_3D_Point( int pt );
void Plot_2D_Point( int pt );
void plot_scalar( int pt, enum iofields blk );
void get_dataset_name( enum iofields blk, char *buf );
