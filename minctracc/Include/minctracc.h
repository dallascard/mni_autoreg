/* ----------------------------- MNI Header -----------------------------------
@NAME       : minctracc.h
@DESCRIPTION: Header file for minctracc.c
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@COPYRIGHT  :
              Copyright 1993 Louis Collins, McConnell Brain Imaging Centre, 
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.

@CREATED    : Thu May 20 14:20:21 EST 1993 Louis Collins
@MODIFIED   : $Log: minctracc.h,v $
@MODIFIED   : Revision 1.8  1994-02-21 16:38:49  louis
@MODIFIED   : version before feb 22 changes
@MODIFIED   :
 * Revision 1.7  93/11/15  13:12:53  louis
 * working version, deform deform installation
 * 
---------------------------------------------------------------------------- */

#include <minc.h>
#include <ParseArgv.h>
#include <time_stamp.h>


/* ------------------------  Constants used in program  -------------------- */

#include "constants.h"

/* ------------------------  Types used in program  ------------------------ */

#include "deform_field.h"

#include "arg_data.h"

/*  ------------------------ Function prototypes  ------------------------ */

public int trilinear_interpolant(Volume volume, 
                                 PointR *coord, double *result);

public int tricubic_interpolant(Volume volume, 
                                PointR *coord, double *result);

public void do_Ncubic_interpolation(Volume volume, 
                                    long index[], int cur_dim, 
                                    double frac[], double *result);

public int nearest_neighbour_interpolant(Volume volume, 
                                         PointR *coord, double *result);
public int point_not_masked(Volume volume,
                            Real wx, Real wy, Real wz);

public int get_transformation(char *dst, char *key, char *nextArg);

public int get_mask_file(char *dst, char *key, char *nextArg);

public void procrustes(int npoints, int ndim, 
                       float **Apoints, float **Bpoints,
                       float *translation, float *centre_of_rotation,
                       float **rotation, float *scale);

public void transformations_to_homogeneous(int ndim, 
                  float *translation, float *centre_of_rotation,
                  float **rotation, float scale,
                  float **transformation);

public void translation_to_homogeneous(int ndim, float *translation,
                                       float **transformation);

public void rotation_to_homogeneous(int ndim, float **rotation,
                                       float **transformation);

public float fit_function(float *x);        /* apply cross correlation to the data sets    */

public float zscore_function(float *x);     /* calculate rms z-score difference.           */

public float check_function(float *x);      /* calculate the squared error between points2 */

public void invertmatrix(int n, float **mat, float **mat_invert);

public BOOLEAN init_params(Volume d1,
			   Volume d2,
			   Volume m1,
			   Volume m2, 
			   Arg_Data *globals);

public void init_lattice(Volume d1,
			 Volume d2,
			 Volume m1,
			 Volume m2, 
			 Arg_Data *globals);

public BOOLEAN optimize_linear_transformation(Volume d1,
					      Volume d2,
					      Volume m1,
					      Volume m2, 
					      Arg_Data *globals);

public BOOLEAN optimize_non_linear_transformation(Volume d1,
						  Volume d1_dx, 
						  Volume d1_dy, 
						  Volume d1_dz, 
						  Volume d1_dxyz,
						  Volume d2,
						  Volume d2_dx, 
						  Volume d2_dy, 
						  Volume d2_dz, 
						  Volume d2_dxyz,
						  Volume m1,
						  Volume m2, 
						  Arg_Data *globals);

#include "objectives.h"

public float measure_fit(Volume d1,
			 Volume d2,
			 Volume m1,
			 Volume m2, 
			 Arg_Data *globals);

public void make_matlab_data_file(Volume d1,
				  Volume d2,
				  Volume m1,
				  Volume m2, 
				  char *comments,
				  Arg_Data *globals);

Status read_all_data(Volume *dblur,
		     Volume *dx,
		     Volume *dy,
		     Volume *dz,
		     Volume *dxyz, 
		     char *name);

Status read_deform_data(Volume *dx,
			Volume *dy,
			Volume *dz,
			char *name,
			char *history);

Status save_deform_data(Volume dx,
			Volume dy,
			Volume dz,
			char *name,
			char *history);

public  Status  output_deformation_file(
    char                filename[],
    char                comments[],
    General_transform   *transform );

public void build_default_deformation_field(Arg_Data *globals);

/*  ------------------------ Macros used in program  ------------------------ */

#include "local_macros.h"

/*  ------------------------ Global data structure for program  ------------------------ */

Arg_Data main_args = {
  {NULL,NULL,NULL,NULL,NULL,NULL,NULL},	/* filenames           */
  {1,FALSE},			/* verbose, debug      */
  {				/* transformation info */
    TRUE,			/*   do default Principal Axis Transformation start */
    (char *)NULL,			/*   filename */
    (char *)NULL,			/*   file_contents */
    0,                          /* buffer_length   */
    (General_transform *)NULL,	/*   General transform */
    (General_transform *)NULL,	/*   General transform copy of input */
    TRANS_PROCRUSTES,		/*   default type      */
    {0.0, 0.0, 0.0},		/*   center            */
    {1.0, 1.0, 1.0},		/*   scale             */
    {0.0, 0.0, 0.0},		/*   shears            */
    {0.0, 0.0, 0.0},		/*   rotations         */
    {0.0, 0.0, 0.0},		/*   translations      */
    FALSE},			/*   invert_mapping_flag                  */
  trilinear_interpolant,	/* use trilinear interpolation by default */
  xcorr_objective,              /* use cross-correlation by default       */
  OPT_SIMPLEX,                  /* use simplex optimization strategy      */
  {4.0,4.0,4.0},		/* default step sizes for lattice         */
  {0.0,0.0,0.0},		/* default start for lattice, reset in init_lattice */
  {0,0,0},                      /* default number of element in lattice, also reset */

  {1.0,0.0,0.0},		/* default sampling lattice axes directions */
  {0.0,1.0,0.0},
  {0.0,0.0,1.0},

  1,                            /* use first volume as default smallest volume      */
  {FALSE, FALSE, FALSE, FALSE},
  {0.0,0.0},			/* lower limit of voxels considered                 */
  5.0,				/* percent noise speckle                            */
  16				/* number of groups to use for ratio of variance    */
};


