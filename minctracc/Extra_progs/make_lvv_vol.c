/* ----------------------------- MNI Header -----------------------------------
@NAME       : make_lvv_vol
@INPUT      : argc, argv - command line arguments
@OUTPUT     : (none)
@RETURNS    : status
@DESCRIPTION: Program to calculate an Lvv volume from a blurred input
              volume
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Thur Oct 5 08:45:43 MET 1995
@MODIFIED   : $Log: make_lvv_vol.c,v $
@MODIFIED   : Revision 1.1  1999-10-25 19:52:06  louis
@MODIFIED   : final checkin before switch to CVS
@MODIFIED   :

@COPYRIGHT  :
              Copyright 1995 Louis Collins, McConnell Brain Imaging Centre, 
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[]="$Header: /private-cvsroot/registration/mni_autoreg/minctracc/Extra_progs/make_lvv_vol.c,v 1.1 1999-10-25 19:52:06 louis Exp $";
#endif

#include <stdio.h>
#include <internal_volume_io.h>
#include <Proglib.h>
#include <config.h>

/* Constants */
#ifndef TRUE
#  define TRUE 1
#  define FALSE 0
#endif
#ifndef public
#  define public
#  define private static
#endif

#define VERY_SMALL_EPS 0.0001	/* this is data dependent! */

typedef struct {
  Real 
    u,v,w,
    uu,vv,ww,
    uv,uw,vw;
} deriv_3D_struct;

static char *default_dim_names[N_DIMENSIONS] = { MIxspace, MIyspace, MIzspace };


public Real return_Lvv(Real r[3][3][3],
		       Real eps);

public void init_the_volume_to_zero(Volume volume);

void get_volume_XYZV_indices(Volume data, int xyzv[]);

char *prog_name;
int stat_quad_total;
int stat_quad_zero;
int stat_quad_two;
int stat_quad_plus;
int stat_quad_minus;
int stat_quad_semi;


int main(int argc, char *argv[])
{

  progress_struct
    progress;

  Status 
    stat;

  Real 
    tmp, max_val, min_val, intensity_threshold,
    K, S, k1, k2, Lvv,
    val[3][3][3];
  
  Volume 
    data, lvv;

  float ***float_vol,  *f_ptr;

  int
    count,
    index[MAX_DIMENSIONS],
    data_xyzv[MAX_DIMENSIONS],
    sizes[MAX_DIMENSIONS],
    m,n,o,i,j,k;

  char 
    *history,
    output_filename[1024];

  prog_name = argv[0];
  history = time_stamp(argc, argv);

  if (argc!=3) {
    print("usage: %s input.mnc output_basename\n", prog_name);
    exit(EXIT_FAILURE);
  }

  stat = input_volume(argv[1],3,default_dim_names, NC_UNSPECIFIED, FALSE, 
		      0.0,0.0,TRUE, &data, (minc_input_options *)NULL);

  if (stat != OK) {
    print ("Error: cannot read %s.\n",argv[1]);
    exit(EXIT_FAILURE);
  }

  lvv = copy_volume_definition(data, NC_UNSPECIFIED, FALSE, 0.0, 0.0);


  get_volume_sizes(data,sizes);
  get_volume_XYZV_indices(data,data_xyzv);


  max_val = -DBL_MAX;
  min_val =  DBL_MAX;

  ALLOC3D(float_vol, sizes[0], sizes[1], sizes[2]);

  for_less(i,0,sizes[0])
    for_less(j,0,sizes[1])
      for_less(k,0,sizes[2]) {
	float_vol[i][j][k] = 0.0;
	tmp = get_volume_real_value(data, i,j,k,0,0);
	if (tmp>max_val) max_val = tmp;
	if (tmp<min_val) min_val = tmp;
      }

  intensity_threshold = 0.01 * max_val;

  initialize_progress_report(&progress, FALSE, sizes[0]*sizes[1]*sizes[2]+1,
			     "Building Lvv:");
  count = 0;
  max_val = -1000000.0;
  min_val =  1000000.0;

  for_less(index[ data_xyzv[X] ],1,sizes[data_xyzv[X]]-1)
    for_less(index[ data_xyzv[Y] ],1,sizes[data_xyzv[Y]]-1)
      for_less(index[ data_xyzv[Z] ],1,sizes[data_xyzv[Z]]-1) {
	
	tmp = get_volume_real_value(data, 
				    index[data_xyzv[X]], 
				    index[data_xyzv[Y]], 
				    index[data_xyzv[Z]], 0,0);

	Lvv = 0.0;
	if (tmp > intensity_threshold) {

	  for_inclusive(m,-1,1)
	    for_inclusive(n,-1,1)
	      for_inclusive(o,-1,1)
		val[m+1][n+1][o+1] =  
		  get_volume_real_value(data, 
					index[data_xyzv[X]]+m, 
					index[data_xyzv[Y]]+n, 
					index[data_xyzv[Z]]+o, 0,0);
	  
	  Lvv = return_Lvv(val, (Real)VERY_SMALL_EPS);
	}

	if (max_val < Lvv) max_val = Lvv;
	if (min_val > Lvv) min_val = Lvv;

	float_vol[index[data_xyzv[X]]][index[data_xyzv[Y]]][index[data_xyzv[Z]]]=Lvv;


	count++;
	update_progress_report( &progress, count );
	
      }
  terminate_progress_report(&progress);
  
  min_val *= 0.9;
  max_val *= 0.9;

  set_volume_real_range(lvv, min_val, max_val);

  for_less(i,0,sizes[0])
    for_less(j,0,sizes[1])
      for_less(k,0,sizes[2]) {
	
	Lvv = float_vol[i][j][k];
	if (Lvv < min_val) Lvv = min_val;
	if (Lvv > max_val) Lvv = max_val;
    
	set_volume_real_value(lvv, i,j,k, 0, 0, Lvv);
      }

  FREE3D(float_vol);

  print ("Saving data (%f,%f)...\n",max_val, min_val);

  sprintf(output_filename,"%s_Lvv.mnc",argv[2]);
  stat = output_modified_volume(output_filename, NC_UNSPECIFIED, FALSE, 
				0.0, 0.0,  lvv, argv[1], history, NULL);
  if (stat != OK) {
    print ("Error: cannot write %s.\n",output_filename);
    exit(EXIT_FAILURE);
  }

  print ("done.\n");

  exit(EXIT_SUCCESS);
}



public void init_the_volume_to_zero(Volume volume)
{
    int             v0, v1, v2, v3, v4;
    Real            zero;
  
    zero = CONVERT_VALUE_TO_VOXEL(volume, 0.0);

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
      
      set_volume_voxel_value( volume, v0, v1, v2, v3, v4, zero );
    
    END_ALL_VOXELS

}

void get_volume_XYZV_indices(Volume data, int xyzv[])
{
  
  int 
    axis, i, vol_dims;
  char 
    **data_dim_names;
  
  vol_dims       = get_volume_n_dimensions(data);
  data_dim_names = get_volume_dimension_names(data);
  
  for_less(i,0,N_DIMENSIONS+1) xyzv[i] = -1;
  for_less(i,0,vol_dims) {
    if (convert_dim_name_to_spatial_axis(data_dim_names[i], &axis )) {
      xyzv[axis] = i; 
    } 
    else {     /* not a spatial axis */
      xyzv[Z+1] = i;
    }
  }
  delete_dimension_names(data_dim_names);
  
}


public void estimate_3D_derivatives(Real r[3][3][3], 
				    deriv_3D_struct *d) 

{

  Real	*p11, *p12, *p13;
  Real	*p21, *p22, *p23;
  Real	*p31, *p32, *p33;

  Real	slice_u1, slice_u2, slice_u3;
  Real	slice_v1, slice_v2, slice_v3;
  Real	slice_w1, slice_w2, slice_w3;
  
  Real	edge_u1_v1, /* edge_u1_v2,*/ edge_u1_v3;
/*  Real	edge_u2_v1, edge_u2_v2, edge_u2_v3; */
  Real	edge_u3_v1,  /* edge_u3_v2,*/ edge_u3_v3;
  Real	edge_u1_w1, edge_u1_w2, edge_u1_w3;
  Real	edge_u2_w1, edge_u2_w2, edge_u2_w3;
  Real	edge_u3_w1, edge_u3_w2, edge_u3_w3;
  Real	edge_v1_w1, edge_v1_w2, edge_v1_w3;
  Real	edge_v2_w1, edge_v2_w2, edge_v2_w3;
  Real	edge_v3_w1, edge_v3_w2, edge_v3_w3;
  
  /* --- 3x3x3 [u][v][w] --- */
  
  p11 = r[0][0]; 
  p12 = r[0][1]; 
  p13 = r[0][2]; 
  p21 = r[1][0]; 
  p22 = r[1][1]; 
  p23 = r[1][2]; 
  p31 = r[2][0]; 
  p32 = r[2][1]; 
  p33 = r[2][2]; 
  
				/* lines varying along w */
  edge_u1_v1 = ( *p11     + *(p11+1) + *(p11+2));

/*  edge_u1_v2 = ( *p12     + *(p12+1) + *(p12+2)); */
  edge_u1_v3 = ( *p13     + *(p13+1) + *(p13+2));
/*  edge_u2_v1 = ( *p21     + *(p21+1) + *(p21+2)); */
/*  edge_u2_v2 = ( *p22     + *(p22+1) + *(p22+2)); */
/*  edge_u2_v3 = ( *p23     + *(p23+1) + *(p23+2)); */
  edge_u3_v1 = ( *p31     + *(p31+1) + *(p31+2));
/*  edge_u3_v2 = ( *p32     + *(p32+1) + *(p32+2)); */
  edge_u3_v3 = ( *p33     + *(p33+1) + *(p33+2));
  
				/* lines varying along v */
  edge_u1_w1 = (  *p11    +  *p12    +  *p13   );
  edge_u1_w2 = ( *(p11+1) + *(p12+1) + *(p13+1));
  edge_u1_w3 = ( *(p11+2) + *(p12+2) + *(p13+2));
  edge_u2_w1 = (  *p21    +  *p22    +  *p23   );
  edge_u2_w2 = ( *(p21+1) + *(p22+1) + *(p23+1));
  edge_u2_w3 = ( *(p21+2) + *(p22+2) + *(p23+2));
  edge_u3_w1 = (  *p31    +  *p32    +  *p33   );
  edge_u3_w2 = ( *(p31+1) + *(p32+1) + *(p33+1)); 
  edge_u3_w3 = ( *(p31+2) + *(p32+2) + *(p33+2));
  
				/* lines varying along u */
  edge_v1_w1 = (  *p11    +  *p21    +  *p31   );
  edge_v1_w2 = ( *(p11+1) + *(p21+1) + *(p31+1));
  edge_v1_w3 = ( *(p11+2) + *(p21+2) + *(p31+2));
  edge_v2_w1 = (  *p12    +  *p22    +  *p32   );
  edge_v2_w2 = ( *(p12+1) + *(p22+1) + *(p32+1));
  edge_v2_w3 = ( *(p12+2) + *(p22+2) + *(p32+2));
  edge_v3_w1 = (  *p13    +  *p23    +  *p33   );
  edge_v3_w2 = ( *(p13+1) + *(p23+1) + *(p33+1));
  edge_v3_w3 = ( *(p13+2) + *(p23+2) + *(p33+2));
  
  slice_u1 =  (edge_u1_w1 + edge_u1_w2 + edge_u1_w3);
  slice_u2 =  (edge_u2_w1 + edge_u2_w2 + edge_u2_w3);
  slice_u3 =  (edge_u3_w1 + edge_u3_w2 + edge_u3_w3);
  slice_v1 =  (edge_v1_w1 + edge_v1_w2 + edge_v1_w3);
  slice_v2 =  (edge_v2_w1 + edge_v2_w2 + edge_v2_w3);
  slice_v3 =  (edge_v3_w1 + edge_v3_w2 + edge_v3_w3);
  slice_w1 =  (edge_u1_w1 + edge_u2_w1 + edge_u3_w1);
  slice_w2 =  (edge_u1_w2 + edge_u2_w2 + edge_u3_w2);
  slice_w3 =  (edge_u1_w3 + edge_u2_w3 + edge_u3_w3);
  
  d->u  = (slice_u3 - slice_u1) / 18.0;                          
  d->v  = (slice_v3 - slice_v1) / 18.0;                          
  d->w  = (slice_w3 - slice_w1) / 18.0;                          
  d->uu = (slice_u3 + slice_u1 - 2*slice_u2) / 9.0;                   
  d->vv = (slice_v3 + slice_v1 - 2*slice_v2) / 9.0;                   
  d->ww = (slice_w3 + slice_w1 - 2*slice_w2) / 9.0;                   
  d->uv = (edge_u3_v3 + edge_u1_v1 - edge_u3_v1 - edge_u1_v3) / 12.0; 
  d->uw = (edge_u3_w3 + edge_u1_w1 - edge_u3_w1 - edge_u1_w3) / 12.0;  
  d->vw = (edge_v3_w3 + edge_v1_w1 - edge_v3_w1 - edge_v1_w3) / 12.0;  
  
} /* estimate_3D_derivatives */



public Real return_Lvv(Real r[3][3][3],
		       Real eps)
     
{
  deriv_3D_struct 
    d;			

  Real
    S,				/* mean curvature          */
    Lvv,
    sq_mag_grad,		/* square of magnitude of gradient   */
    x,y,z,			/* first order derivatives */
    xx,yy,zz,xy,xz,yz;		/* second order derivative */


  d.u  = (r[2][1][1] - r[0][1][1] ) / 2.0;
  d.v  = (r[1][2][1] - r[1][0][1] ) / 2.0;
  d.w  = (r[1][1][2] - r[1][1][0] ) / 2.0;
  d.uu = (r[2][1][1] + r[0][1][1] -2*r[1][1][1]);
  d.vv = (r[1][2][1] + r[1][0][1] -2*r[1][1][1]);
  d.ww = (r[1][1][2] + r[1][1][0] -2*r[1][1][1]);
  d.uv = (r[2][2][1] + r[0][0][1] - r[0][2][1] - r[2][0][1]) / 4.0;
  d.uw = (r[2][1][2] + r[0][1][0] - r[0][1][2] - r[2][1][0]) / 4.0;
  d.vw = (r[1][2][2] + r[1][0][0] - r[1][0][2] - r[1][2][0]) / 4.0;

  x  = d.u;  y  = d.v;  z  = d.w;
  xx = d.uu; yy = d.vv; zz = d.ww;
  xy = d.uv; yz = d.vw; xz = d.uw;

  Lvv = 0.0;
  sq_mag_grad = x*x + y*y + z*z;

  if ( ABS(sq_mag_grad) > eps )  {
				/* Mean curvature */
    S = (
	 x*x*(yy + zz) - 2*y*z*yz +
	 y*y*(xx + zz) - 2*x*z*xz +
	 z*z*(xx + yy) - 2*x*y*xy
	 )
          / (2 * sqrt(sq_mag_grad*sq_mag_grad*sq_mag_grad));

    Lvv =  sq_mag_grad * S;
  }

  return(Lvv);
}
					
