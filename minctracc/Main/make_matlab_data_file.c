/* ----------------------------- MNI Header -----------------------------------
@NAME       : make_matlab_data_file.c
@INPUT      : d1,d2,m1,m2 - data and mask volumes.
@OUTPUT     : creates an output file readable by matlab.
@RETURNS    : nothing
@DESCRIPTION: 
              this routines calculates the objective function value for 
	      the current transformation and variants thereof.

	      each parameter is varied in turn, one at a time, from 
	      -simplex to +simplex around the parameter.
	      
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

@CREATED    : Mon Oct  4 13:06:17 EST 1993 Louis
@MODIFIED   : $Log: make_matlab_data_file.c,v $
@MODIFIED   : Revision 1.6  1996-08-12 14:15:40  louis
@MODIFIED   : Pre-release
@MODIFIED   :
 * Revision 1.5  1995/02/22  08:56:06  collins
 * Montreal Neurological Institute version.
 * compiled and working on SGI.  this is before any changes for SPARC/
 * Solaris.
 *
 * Revision 1.4  94/04/26  12:54:23  louis
 * updated with new versions of make_rots, extract2_parameters_from_matrix 
 * that include proper interpretation of skew.
 * 
 * Revision 1.3  94/04/06  11:48:39  louis
 * working linted version of linear + non-linear registration based on Lvv
 * operator working in 3D
 * 
 * Revision 1.2  94/02/21  16:35:40  louis
 * version before feb 22 changes
 * 
 * Revision 1.1  93/11/15  16:26:47  louis
 * Initial revision
 * 
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[]="$Header: /private-cvsroot/registration/mni_autoreg/minctracc/Main/make_matlab_data_file.c,v 1.6 1996-08-12 14:15:40 louis Exp $";
#endif


#include <limits.h>
#include <volume_io.h>

#include "constants.h"
#include "arg_data.h"
#include "objectives.h"
#include "segment_table.h"
#include "print_error.h"

extern Arg_Data main_args;

extern   Volume   Gdata1, Gdata2, Gmask1, Gmask2;
extern   int      Ginverse_mapping_flag, Gndim;
extern   double   simplex_size ;
extern   Segment_Table  *segment_table;

extern Real            **prob_hash_table; 
extern Real            *prob_fn1;         
extern Real            *prob_fn2;         

extern int Matlab_num_steps;

public float fit_function(float *params);

public void make_zscore_volume(Volume d1, Volume m1, 
			       Real *threshold); 

public void add_speckle_to_volume(Volume d1, 
				  float speckle,
				  double  *start, int *count, VectorR directions[]);

public void parameters_to_vector(double *trans, 
				  double *rots, 
				  double *scales,
				  double *shears,
				  float  *op_vector,
				  double *weights);

public BOOLEAN replace_volume_data_with_ubyte(Volume data);

public void make_matlab_data_file(Volume d1,
				  Volume d2,
				  Volume m1,
				  Volume m2, 
				  char *comments,
				  Arg_Data *globals)
{


  Status
    status;
  float 
    *p;
  int 
    i,j,stat, 
    ndim;
  FILE
    *ofd;
  Real
    start,step;
  double trans[3], rots[3], shears[3], scales[3];
  Data_types
    data_type;

  start = 0.0;
  if (globals->obj_function == zscore_objective) { /* replace volume d1 and d2 by zscore volume  */
    make_zscore_volume(d1,m1,&globals->threshold[0]);
    make_zscore_volume(d2,m2,&globals->threshold[1]);
  } 
  else  if (globals->obj_function == ssc_objective) {	/* add speckle to the data set */
    
    make_zscore_volume(d1,m1,&globals->threshold[0]); /* need to make data sets comparable */
    make_zscore_volume(d2,m2,&globals->threshold[1]); /* in mean and sd...                 */
    
    if (globals->smallest_vol == 1)
      add_speckle_to_volume(d1, 
			    globals->speckle,
			    globals->start, globals->count, globals->directions);
    else
      add_speckle_to_volume(d2, 
			    globals->speckle,
			    globals->start, globals->count, globals->directions);    
  } else if (globals->obj_function == vr_objective) {
    if (globals->smallest_vol == 1) {
      if (!build_segment_table(&segment_table, d1, globals->groups))
	print_error_and_line_num("%s",__FILE__, __LINE__,"Could not build segment table for source volume\n");
    }
    else {
      if (!build_segment_table(&segment_table, d2, globals->groups))
	print_error_and_line_num("%s",__FILE__, __LINE__,"Could not build segment table for target volume\n");
    }
    if (globals->flags.debug && globals->flags.verbose>1) {
      print ("groups = %d\n",segment_table->groups);
      for_less(i, segment_table->min, segment_table->max+1) {
	print ("%5d: table = %5d, function = %5d\n",i,segment_table->table[i],
	       (segment_table->segment)(i,segment_table) );
      }
    }
    
  } else if (globals->obj_function == mutual_information_objective)
				/* Collignon's mutual information */
    {

      if ( globals->groups != 256 ) {
	print ("WARNING: -groups was %d, but will be forced to 256 in this run\n",globals->groups);
	globals->groups = 256;
      }

      data_type = get_volume_data_type (d1);
      if (data_type != UNSIGNED_BYTE) {
	print ("WARNING: source volume not UNSIGNED_BYTE, will do conversion now.\n");
	if (!replace_volume_data_with_ubyte(d1)) {
	  print_error_and_line_num("Can't replace volume data with unsigned bytes\n",
			     __FILE__, __LINE__);
	}
      }

      data_type = get_volume_data_type (d2);
      if (data_type != UNSIGNED_BYTE) {
	print ("WARNING: target volume not UNSIGNED_BYTE, will do conversion now.\n");
	if (!replace_volume_data_with_ubyte(d2)) {
	  print_error_and_line_num("Can't replace volume data with unsigned bytes\n",
			     __FILE__, __LINE__);
	}
      }

      ALLOC(   prob_fn1,   globals->groups);
      ALLOC(   prob_fn2,   globals->groups);
      ALLOC2D( prob_hash_table, globals->groups, globals->groups);

    } 
    

  /* ---------------- prepare the weighting array for for the objective function  ---------*/

  stat = TRUE;
  switch (globals->trans_info.transform_type) {
  case TRANS_LSQ3: 
    for_less(i,3,12) globals->trans_info.weights[i] = 0.0;
    for_less(i,0,3) {
      globals->trans_info.scales[i] = 1.0;
      globals->trans_info.rotations[i] = 0.0;
      globals->trans_info.shears[i] = 0.0;
    }
    break;
  case TRANS_LSQ6: 
    for_less(i,6,12) globals->trans_info.weights[i] = 0.0;
    for_less(i,0,3) {
      globals->trans_info.scales[i] = 1.0;
      globals->trans_info.shears[i] = 0.0;
    }
    break;
				/* collapsed because PROCRUSTES and LSQ7 are 
				   the same */
  case TRANS_PROCRUSTES: 
  case TRANS_LSQ7: 
    for_less(i,7,12) globals->trans_info.weights[i] = 0.0;
    for_less(i,0,3) {
      globals->trans_info.shears[i] = 0.0;
    }
    break;
  case TRANS_LSQ9: 
    for_less(i,9,12) globals->trans_info.weights[i] = 0.0;
    for_less(i,0,3) {
      globals->trans_info.shears[i] = 0.0;
    }
    break;
  case TRANS_LSQ10: 
    for_less(i,10,12) globals->trans_info.weights[i] = 0.0;
    for_less(i,1,3) {
      globals->trans_info.shears[i] = 0.0;
    }
    break;
  case TRANS_LSQ: 
				/* nothing to be zeroed */
    break;
  case TRANS_LSQ12: 
				/* nothing to be zeroed */
    break;
  default:
    (void)fprintf(stderr, "Unknown type of transformation requested (%d)\n",
		   globals->trans_info.transform_type);
    (void)fprintf(stderr, "Error in line %d, file %s\n",__LINE__, __FILE__);
    stat = FALSE;
  }

  if ( !stat ) 
    print_error_and_line_num ("Can't calculate measure (stat is false).", 
			      __FILE__, __LINE__);


				/* find number of dimensions for obj function */
  ndim = 0;
  for_less(i,0,12)
    if (globals->trans_info.weights[i] != 0.0) ndim++;


				/* set GLOBALS to communicate with the
				   function to be fitted!              */
  Gndim = ndim;
  Gdata1 = d1;
  Gdata2 = d2;
  Gmask1 = m1;
  Gmask2 = m2;
  Ginverse_mapping_flag = FALSE;

print ("trans: %10.5f %10.5f %10.5f \n",
       globals->trans_info.translations[0],globals->trans_info.translations[1],globals->trans_info.translations[2]);
print ("rots : %10.5f %10.5f %10.5f \n",
       globals->trans_info.rotations[0],globals->trans_info.rotations[1],globals->trans_info.rotations[2]);
print ("scale: %10.5f %10.5f %10.5f \n",
       globals->trans_info.scales[0],globals->trans_info.scales[1],globals->trans_info.scales[2]);


  if (ndim>0) {

    ALLOC(p,ndim+1); /* parameter values */
    
    /*  translation +/- simplex_size
	rotation    +/- simplex_size*DEG_TO_RAD
	scale       +/- simplex_size/50
	*/
    
    
    
    status = open_file(  globals->filenames.matlab_file, WRITE_FILE, BINARY_FORMAT,  &ofd );
    if ( status != OK ) 
      print_error_and_line_num ("filename `%s' cannot be opened.", 
		   __FILE__, __LINE__, globals->filenames.matlab_file);
    
    
    (void)fprintf (ofd,"%% %s\n",comments);
    
    /* do translations */
    for_inclusive(j,1,12) {
      if (globals->trans_info.weights[j-1] != 0.0) {
	switch (j) {
	case  1: (void)fprintf (ofd,"tx = [\n"); start = globals->trans_info.translations[0]; break;
	case  2: (void)fprintf (ofd,"ty = [\n"); start = globals->trans_info.translations[1]; break;
	case  3: (void)fprintf (ofd,"tz = [\n"); start = globals->trans_info.translations[2]; break;
	case  4: (void)fprintf (ofd,"rx = [\n"); start = globals->trans_info.rotations[0]; break;
	case  5: (void)fprintf (ofd,"ry = [\n"); start = globals->trans_info.rotations[1]; break;
	case  6: (void)fprintf (ofd,"rz = [\n"); start = globals->trans_info.rotations[2]; break;
	case  7: (void)fprintf (ofd,"sx = [\n"); start = globals->trans_info.scales[0]; break;
	case  8: (void)fprintf (ofd,"sy = [\n"); start = globals->trans_info.scales[1]; break;
	case  9: (void)fprintf (ofd,"sz = [\n"); start = globals->trans_info.scales[2]; break;
	case 10: (void)fprintf (ofd,"shx = [\n");start = globals->trans_info.shears[0]; break;
	case 11: (void)fprintf (ofd,"shy = [\n");start = globals->trans_info.shears[1]; break;
	case 12: (void)fprintf (ofd,"shz = [\n");start = globals->trans_info.shears[2]; break; 
	}
	
	for_less(i,0,3) {
	  trans[i]  = globals->trans_info.translations[i];
	  scales[i] = globals->trans_info.scales[i];
	  shears[i] = globals->trans_info.shears[i];
	  rots[i]   = globals->trans_info.rotations[i];
	}

	step =  globals->trans_info.weights[j-1] * simplex_size/ Matlab_num_steps;
	
	for_inclusive(i,-Matlab_num_steps,Matlab_num_steps) {
	  
	  switch (j) {
	  case  1: trans[0] =start + i*step; break;
	  case  2: trans[1] =start + i*step; break;
	  case  3: trans[2] =start + i*step; break;
	  case  4: rots[0]  =start + i*step; break;
	  case  5: rots[1]  =start + i*step; break;
	  case  6: rots[2]  =start + i*step; break;
	  case  7: scales[0]=start + i*step; break;
	  case  8: scales[1]=start + i*step; break;
	  case  9: scales[2]=start + i*step; break;
	  case 10: shears[0]=start + i*step; break;
	  case 11: shears[1]=start + i*step; break;
	  case 12: shears[2]=start + i*step; break; 
	  }
	  
	  parameters_to_vector(trans,
			       rots,
			       scales,
			       shears,
			       p,
			       globals->trans_info.weights);
    
	  (void)fprintf (ofd, "%f %f %f\n",i*step, start+i*step, fit_function(p));
	}

	(void)fprintf (ofd,"];\n"); 
      }
    }
    
    status = close_file(ofd);
    if ( status != OK ) 
      print_error_and_line_num ("filename `%s' cannot be closed.", 
		   __FILE__, __LINE__, globals->filenames.matlab_file);
    
    
    FREE(p);
  }

  if (globals->obj_function == vr_objective) {
    if (!free_segment_table(segment_table)) {
      (void)fprintf(stderr, "Can't free segment table.\n");
      (void)fprintf(stderr, "Error in line %d, file %s\n",__LINE__, __FILE__);
    }
  } else
  if (globals->obj_function == mutual_information_objective)
				/* Collignon's mutual information */
    {
      FREE(   prob_fn1 );
      FREE(   prob_fn2 );
      FREE2D( prob_hash_table);
    }



}
