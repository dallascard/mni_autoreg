
#include <volume_io.h>
#include <limits.h>

static char *default_dim_names[N_DIMENSIONS] =
   { MIzspace, MIyspace, MIxspace };


char *prog_name;

public void get_zscore_values(Volume d1, Volume m1, 
			      Real threshold, Real *mean, Real *std); 


main(int argc, char *argv[])
{
  int 
    count,
    i,j,k, p,r,s, flag,
    sizes1[3],sizes2[3], sizes3[3];
  Real
    thresh, mean, std;
  Real
    min1,max1,min2,max2, mean1, std1, mean2, std2,
    corr, v1, v2, v3, s1, s2, s12,
    u,v,w,  x,y,z;
  Status status;

  Volume 
    data1, data2, mask;
  char *f1, *f2, *mf;

  if (argc<4) {
    print ("usage:  xcorr_vol.c vol1.mnc vol2.mnc threshold [mask.mnc]\n");
    exit(EXIT_FAILURE);
  }

  prog_name = argv[0];
  f1        = argv[1];
  f2        = argv[2];  
  thresh    = atof(argv[3]);
  if (argc==5)
    mf      = argv[4];

  status = input_volume(f1, 3, default_dim_names, NC_UNSPECIFIED, FALSE, 0.0,0.0,
			TRUE, &data1, (minc_input_options *)NULL); 
  if (status!=OK) {
    print ("Error reading %s.\n",f1);
    exit(EXIT_FAILURE);
  }
  
  status = input_volume(f2, 3, default_dim_names, NC_UNSPECIFIED, FALSE, 0.0,0.0,
			TRUE, &data2, (minc_input_options *)NULL); 
 
  if (status!=OK) {
    print ("Error reading %s.\n",f2);
    exit(EXIT_FAILURE);
  }

  if (argc==5) {
    status = input_volume(mf, 3, default_dim_names, NC_UNSPECIFIED, FALSE, 0.0,0.0,
			  TRUE, &mask, (minc_input_options *)NULL); 
    
    if (status!=OK) {
      print ("Error reading %s.\n",mf);
      exit(EXIT_FAILURE);
    }
    get_volume_sizes(mask,sizes3); 
  }
  else 
    mask = (Volume)NULL;
  
  get_volume_sizes(data1,sizes1);
  get_volume_sizes(data2,sizes2);
  
  if (sizes1[0] != sizes2[0] || sizes1[1] != sizes2[1] || sizes1[2] != sizes2[2]) {
    print ("Size mismatch between %s and %s.\n",f1,f2);
    print ("%d,%d,%d != %d,%d,%d\n", 
	   sizes1[0],sizes1[1],sizes1[2],
	   sizes2[0],sizes2[1],sizes2[2]);
    exit(EXIT_FAILURE);
  }
  
  get_zscore_values(data1, mask, thresh, &mean1, &std1); 
  get_zscore_values(data2, mask, thresh, &mean2, &std2); 

  if (std1==0.0 || std2==0.0) {
    print ("No standard deviation in one of %s and %s.\n",f1,f2);
    exit(EXIT_FAILURE);
  }

  count = 0;
  s1 = s2 = s12 = 0.0;
  for_less(i,0,sizes1[0]) {
    for_less(j,0,sizes1[1]) {
      for_less(k,0,sizes1[2]) {

	flag = TRUE;

	if (mask != NULL) {
	  convert_3D_voxel_to_world(data1, i,j,k, &x, &y, &z);
	  convert_3D_world_to_voxel(mask,  x,y,z, &u, &v, &w);
	  p = ROUND(u);
	  r = ROUND(v);
	  s = ROUND(w);
	  if (p>=0 &&  p<sizes3[0] &&
	      r>=0 &&  r<sizes3[1] &&
	      s>=0 &&  s<sizes3[2]) {
	    GET_VALUE_3D( v3 ,  mask, p, r, s);
	    if (v3 < 0.5)
	      flag = FALSE;
	  }
	  else 
	    flag = FALSE;
	}

	if (flag ) {

	  GET_VALUE_3D( v1 ,  data1, i, j, k);
	  GET_VALUE_3D( v2 ,  data2, i, j, k);


	  v1 = (v1 - mean1) / std1;
	  v2 = (v2 - mean2) / std2;
	  
	  v1 = v1 - v2;
	  s12 += ABS(v1);
	  
	  count++;


	}
      }
    }
  }

  if (count==0) {
    print ("No masked voxels\n");
  }
  else {
    corr = s12 / count;

    print ("%15.12f\n",corr);
  }

}



public void get_zscore_values(Volume d1, Volume m1, 
			      Real threshold, Real *mean, Real *std)
{
  unsigned long
    count;
  int 
    sizes[MAX_DIMENSIONS],
    s,r,c;
  Real
    wx,wy,wz,
    valid_min_dvoxel, valid_max_dvoxel,
    min,max,
    sum, sum2,  var,
    data_vox,data_val,
    thick[MAX_DIMENSIONS];

  /* get default information from data and mask */

  get_volume_sizes(d1, sizes);
  get_volume_separations(d1, thick);
  get_volume_voxel_range(d1, &valid_min_dvoxel, &valid_max_dvoxel);

  /* initialize counters and sums */

  count  = 0;
  sum  = 0.0;
  sum2 = 0.0;
  min = FLT_MAX;
  max = -FLT_MAX;

				/* get mean and std */
  for_less( s, 0,  sizes[0]) {
    for_less( r, 0, sizes[1]) {
      for_less( c, 0, sizes[2]) {

	convert_3D_voxel_to_world(d1, (Real)s, (Real)r, (Real)c, &wx, &wy, &wz);

	if (point_not_masked(m1, wx,wy,wz)) {
	  
	  GET_VOXEL_3D( data_vox,  d1 , s, r, c );

	  if (data_vox >= valid_min_dvoxel && data_vox <= valid_max_dvoxel) { 

	    data_val = CONVERT_VOXEL_TO_VALUE(d1, data_vox);
	    
	    if (data_val > threshold) {
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


				/* calc mean and std */
  *mean = sum / (float)count;
  var  = ((float)count*sum2 - sum*sum) / ((float)count*((float)count-1));
  *std  = sqrt(var);
 
}