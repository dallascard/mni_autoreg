/* ----------------------------- MNI Header -----------------------------------
@NAME       : volume_functions.c
@DESCRIPTION: collection of routines used to manipulate volume data.
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

@CREATED    : Tue Jun 15 08:57:23 EST 1993 LC
@MODIFIED   :  $Log: volume_functions.c,v $
@MODIFIED   :  Revision 1.11  1996-08-12 14:16:13  louis
@MODIFIED   :  Pre-release
@MODIFIED   :
 * Revision 1.10  1995/02/22  08:56:06  collins
 * Montreal Neurological Institute version.
 * compiled and working on SGI.  this is before any changes for SPARC/
 * Solaris.
 *
 * Revision 1.9  94/04/26  12:54:44  louis
 * updated with new versions of make_rots, extract2_parameters_from_matrix 
 * that include proper interpretation of skew.
 * 
 * Revision 1.8  94/04/06  11:49:00  louis
 * working linted version of linear + non-linear registration based on Lvv
 * operator working in 3D
 * 
 * Revision 1.7  94/02/21  16:37:46  louis
 * version before feb 22 changes
 * 
 * Revision 1.6  93/11/15  16:27:13  louis
 * working version, with new library, with RCS revision stuff,
 * before deformations included
 * 
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[]="$Header: /private-cvsroot/registration/mni_autoreg/minctracc/Volume/volume_functions.c,v 1.11 1996-08-12 14:16:13 louis Exp $";
#endif

#include <limits.h>
#include <volume_io.h>
#include "point_vector.h"
#include "constants.h"
#include <print_error.h>

public int point_not_masked(Volume volume, 
			    Real wx, Real wy, Real wz);


#define MIN_ZRANGE -5.0
#define MAX_ZRANGE  5.0

public void make_zscore_volume(Volume d1, Volume m1, 
			       Real *threshold)
{
  unsigned long
    count;
  int 
    stat_count,
    sizes[MAX_DIMENSIONS],
    s,r,c;
  Real
    wx,wy,wz,
    valid_min_dvoxel, valid_max_dvoxel,
    min,max,
    sum, sum2, mean, var, std,
    data_vox,data_val,
    thick[MAX_DIMENSIONS];

  PointR 
    voxel;

  Volume 
    vol;

  progress_struct
    progress;

  /* get default information from data and mask */

  /* build temporary working volume */
 
  vol = copy_volume_definition(d1, NC_UNSPECIFIED, FALSE, 0.0, 0.0);
  set_volume_real_range(vol, MIN_ZRANGE, MAX_ZRANGE);
  get_volume_sizes(d1, sizes);
  get_volume_separations(d1, thick);
  get_volume_voxel_range(d1, &valid_min_dvoxel, &valid_max_dvoxel);

  /* initialize counters and sums */

  count  = 0;
  sum  = 0.0;
  sum2 = 0.0;
  min = 1e38;
  max = -1e38;
  stat_count = 0;

  initialize_progress_report(&progress, FALSE, sizes[0]*sizes[1]*sizes[2] + 1,
			     "Tally stats" );

				/* do first pass, to get mean and std */
  for_less( s, 0,  sizes[0]) {
    for_less( r, 0, sizes[1]) {
      for_less( c, 0, sizes[2]) {

	stat_count++;
	update_progress_report( &progress, stat_count);
	convert_3D_voxel_to_world(d1, (Real)s, (Real)r, (Real)c, &wx, &wy, &wz);

	if (m1 != NULL) {
	  convert_3D_world_to_voxel(m1, wx, wy, wz, &Point_x(voxel), &Point_y(voxel), &Point_z(voxel));
	}
	else {
	  wx = 0.0; wy = 0.0; wz = 0.0;
	}

	if (point_not_masked(m1, wx,wy,wz)) {
	  
	  GET_VOXEL_3D( data_vox,  d1 , s, r, c );

	  if (data_vox >= valid_min_dvoxel && data_vox <= valid_max_dvoxel) { 

	    data_val = CONVERT_VOXEL_TO_VALUE(d1, data_vox);
	    
	    if (data_val > *threshold) {
	      sum  += data_val;
	      sum2 += data_val*data_val;
	      
	      count++;
	      
	      if (data_val < min)
		min = data_val;
	      else
		if (data_val > max)
		  max = data_val;
	    }
	  }
	}
      }
    }
  }
  terminate_progress_report( &progress );

  stat_count = 0;
  initialize_progress_report(&progress, FALSE, sizes[0]*sizes[1]*sizes[2] + 1,
			     "Zscore convert" );

				/* calc mean and std */
  mean = sum / (float)count;
  var  = ((float)count*sum2 - sum*sum) / ((float)count*((float)count-1));
  std  = sqrt(var);

  min = 1e38;
  max = -1e38;

				/* replace the voxel values */
  for_less( s, 0,  sizes[0]) {
    for_less( r, 0, sizes[1]) {
      for_less( c, 0, sizes[2]) {
	
	stat_count++;
	update_progress_report( &progress, stat_count);

	GET_VOXEL_3D( data_vox,  d1, s, r, c );
	
	if (data_vox >= valid_min_dvoxel && data_vox <= valid_max_dvoxel) { 
	  
	  data_val = CONVERT_VOXEL_TO_VALUE(d1, data_vox);
	  
	  if (data_val > *threshold) {

				/* instead of   
				   data_val = CONVERT_VALUE_TO_VOXEL(d1, data_vox);
				   i will use
				   data_val = CONVERT_VALUE_TO_VOXEL(d1, vol);

				   since the values in vol are changed with respect to the
				   new z-score volume */

	    data_val = (data_val - mean) / std;
	    if (data_val< MIN_ZRANGE) data_val = MIN_ZRANGE;
	    if (data_val> MAX_ZRANGE) data_val = MAX_ZRANGE;

	    data_vox = CONVERT_VALUE_TO_VOXEL( vol, data_val);
	    

	    if (data_val < min) {
	      min = data_val;
	    }
	    else {
	      if (data_val > max)
		max = data_val;
	    }
	  }
	  else
	    data_vox = -DBL_MAX;   /* should be fill_value! */
	  
	  SET_VOXEL_3D( d1 , s, r, c, data_vox );
	}
	
      }
    }
  }

  terminate_progress_report( &progress );

  set_volume_real_range(d1, MIN_ZRANGE, MAX_ZRANGE);	/* reset the data volume's range */

  *threshold = (*threshold - mean) / std;

  delete_volume(vol);
  
}

public void add_speckle_to_volume(Volume d1, 
				  float speckle,
				  double  *start, int *count, VectorR directions[])
{
  VectorR
    vector_step;

  PointR 
    starting_position,
    slice,
    row,
    col;
  Real valid_min_voxel, valid_max_voxel;

  double
    tx,ty,tz,
    voxel_value;
  int
    xi,yi,zi,
    flip_flag,r,c,s;


  flip_flag = FALSE;

  get_volume_voxel_range(d1, &valid_min_voxel, &valid_max_voxel);
  fill_Point( starting_position, start[0], start[1], start[2]);
  
  for_inclusive(s,0,count[SLICE_IND]) {

    SCALE_VECTOR( vector_step, directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    for_inclusive(r,0,count[ROW_IND]) {

      SCALE_VECTOR( vector_step, directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, slice, vector_step );

      SCALE_POINT( col, row, 1.0); /* init first col position */
      for_inclusive(c,0,count[COL_IND]) {

	convert_3D_world_to_voxel(d1, Point_x(col), Point_y(col), Point_z(col), &tx, &ty, &tz);

	xi = ROUND( tx );
	yi = ROUND( ty );
	zi = ROUND( tz );

	GET_VOXEL_3D( voxel_value, d1 , xi, yi, zi ); 


	if (voxel_value >= valid_min_voxel && voxel_value <= valid_max_voxel) {
	  if (flip_flag)
	    voxel_value = voxel_value * (1 + 0.01*speckle);
	  else
	    voxel_value = voxel_value * (1 - 0.01*speckle);

	  SET_VOXEL_3D( d1 , xi, yi, zi, voxel_value );
	}

	flip_flag = !flip_flag;


	ADD_POINT_VECTOR( col, col, directions[COL_IND] );

      }
	


    }
  }


}


public void save_volume(Volume d, char *filename)
{
  Status status;

  status = output_volume(filename,NC_UNSPECIFIED, FALSE, 0.0, 0.0, d, (char *)NULL,
			 (minc_output_options *)NULL);

  if (status != OK)
    print_error_and_line_num("problems writing  volume `%s'.",__FILE__, __LINE__, filename);

}
