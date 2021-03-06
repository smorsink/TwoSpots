/***************************************************************************************/
/*                                   Bend.cpp

This code creates a table of bending angles. This file will be read in by Spot.cpp

The program needs to know:
(1) The value of NN (a value of 40 seems to work well) Defined in struct.h
(2) A range of M/R values -- mr_lo mr_hi
(3) How many values of M/R? 
(4) Name of the output file

*/
/***************************************************************************************/

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <exception>
#include <vector>
#include <string>
#include "OblDeflectionTOA.h"
#include "Chi.h"
#include "Atmo.h"    
#include "PolyOblModelNHQS.h"
#include "PolyOblModelCFLQS.h"
#include "SphericalOblModel.h"
#include "OblModelBase.h"
#include "Units.h"
#include "Exception.h"
#include "Struct.h"
#include "time.h"
#include "interp.h"
#include "nrutil.h"
#include <string.h>

// MAIN
int main ( int argc, char** argv ) try {  // argc, number of cmd line args; 
                                          // argv, actual character strings of cmd line args

  /*********************************************/
  /* VARIABLE DECLARATIONS AND INITIALIZATIONS */
  /*********************************************/
    
  std::ofstream out;      // output stream; printing information to the output file
  std::ifstream in;       // input stream

  double 
    mass,                       // Mass of the star, in M_sun
    radius,                 // Radius of the star at the spot, in km
    mass_over_r;              // Dimensionless mass divided by radius ratio
    
  unsigned int num_mr(1), numangles(NN);
  unsigned int calcflag(1), readflag(0);

  double mr_lo, mr_hi, mr_step;

  char out_file[256] = "bend-out.txt";
  char in_file[256] = "No file specified";
		
  // Create LightCurve data structure
  class LightCurve curve, *incurve;  // variables curve and normalized curve, of type LightCurve
  OblModelBase* model;

  /*********************************************************/
  /* READING IN PARAMETERS FROM THE COMMAND LINE ARGUMENTS */
  /*********************************************************/
    
    for ( int i(1); i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {  // the '-' flag lets the computer know that we're giving it information from the cmd line
            switch ( argv[i][1] ) {

	    case 'c': // Calculate Values? 1=yes; 0=no
	      sscanf(argv[i+1], "%u", &calcflag);
	      break;

	    case 'l':  // Lowest value of m/r
	                sscanf(argv[i+1], "%lf", &mr_lo);
	                break;

	    case 'h':  // Highest value of m/r
	                sscanf(argv[i+1], "%lf", &mr_hi);
	                break;

	    case 'i':  // Name of input file
	                sscanf(argv[i+1], "%s", in_file);
	                break;

	    case 'n': // Number of m/r values
	      sscanf(argv[i+1], "%u", &num_mr);
	      break;

	    case 'o':  // Name of output file
	                sscanf(argv[i+1], "%s", out_file);
	                break;

	    case 'r': // Read in Values? 1=yes; 0=no
	      sscanf(argv[i+1], "%u", &readflag);
	      break;
	                
            } // end switch	
        } // end if
    } // end for

    mass = 1.6;
    radius = 12.0;
    mass_over_r = 0.172273;
 
    mass = Units::cgs_to_nounits( mass*Units::MSUN, Units::MASS );
    radius = Units::cgs_to_nounits( radius*1.0e5, Units::LENGTH );   
    model = new SphericalOblModel( radius );

    // Allocate Memory for Bending Angle table for specific M/R
      // Allocate Memory -- Look up table for specific M/R value
    curve.defl.psi_b = dvector(0,3*NN+1);
    curve.defl.b_psi = dvector(0,3*NN+1);
    curve.defl.dcosa_dcosp_b = dvector(0,3*NN+1);
    curve.defl.toa_b = dvector(0,3*NN+1);


    if (calcflag){ // Calculate a table of bending angles for a range of M/R values

      out.open(out_file);
      out.precision(10);

      out << NN <<  std::endl;
      out << num_mr << std::endl;

      for (int j(0);j<=num_mr;j++){

	mass_over_r = mr_lo + j*(mr_hi-mr_lo)/(num_mr);
	std::cout << "M/R = " << mass_over_r << std::endl;
	OblDeflectionTOA* defltoa = new OblDeflectionTOA(model, mass, mass_over_r , radius); 

	/**********************************************************/
	/* Compute maximum deflection for purely outgoing photons */
	/**********************************************************/
	
	double  b_mid;  // the value of b, the impact parameter, at 90% of b_max
	double dpsi_db_val, sinalpha, cosalpha, eps, b_R, psi, alpha;
	curve.defl.b_max =  defltoa->bmax_outgoing(radius); // telling us the largest value of b
	curve.defl.b_R_max = curve.defl.b_max/radius;
	curve.defl.psi_max = defltoa->psi_max_outgoing_u(curve.defl.b_max/radius,&curve.problem); // telling us the largest value of psi

	/********************************************************************/
	/* COMPUTE b VS psi LOOKUP TABLE, GOOD FOR THE SPECIFIED M/R AND mu */
	/********************************************************************/
	
	b_mid = curve.defl.b_R_max * 0.9; 
	// since we want to split up the table between coarse and fine spacing. 
	// 0 - 90% is large spacing, 90% - 100% is small spacing. b_mid is this value at 90%.
	curve.defl.b_psi[0] = 0.0; // definitions of the actual look-up table for b when psi = 0
	curve.defl.psi_b[0] = 0.0; // definitions of the actual look-up table for psi when b = 0
	dpsi_db_val = defltoa->dpsi_db_outgoing_u( 0.0, &curve.problem );
	curve.defl.dcosa_dcosp_b[0] = fabs( (1.0 - 2.0 * mass_over_r) / (dpsi_db_val));
	curve.defl.toa_b[0] = defltoa->toa_outgoing_u( 0.0, &curve.problem );

	// computation for b < b_mid
	for ( unsigned int i(1); i < NN+1; i++ ) { /* compute table of b vs psi points */
	  curve.defl.b_psi[i] = b_mid * i / (NN * 1.0);
	  curve.defl.psi_b[i] = defltoa->psi_outgoing_u(curve.defl.b_psi[i], curve.defl.b_R_max, curve.defl.psi_max, &curve.problem); 
	  dpsi_db_val = defltoa->dpsi_db_outgoing_u( curve.defl.b_psi[i], &curve.problem );
	  curve.defl.toa_b[i] = defltoa->toa_outgoing_u(curve.defl.b_psi[i], &curve.problem );

	  sinalpha = curve.defl.b_psi[i] * sqrt(1.0 - 2.0*mass_over_r);
	  cosalpha = sqrt(1.0 - pow(sinalpha,2));

	  curve.defl.dcosa_dcosp_b[i] = fabs( sinalpha/cosalpha * sqrt(1.0 - 2.0*mass_over_r) / (sin(curve.defl.psi_b[i]) *dpsi_db_val));
	}

	// Compute slope of d(cosalpha)/d(psi) near cos(alpha)=0
	eps = 1e-2;
	//eps = 1e-3;
	double x1, x2, y1, y2, yslope, yy;
  
	x1 = Units::PI/2.0 - eps; 
	x2 = x1+eps*0.1;

	b_R = sin(x1) / sqrt( 1.0 - 2.0 * mass_over_r );
	psi = defltoa->psi_outgoing_u( b_R, curve.defl.b_R_max, curve.defl.psi_max, &curve.problem);
	dpsi_db_val = defltoa->dpsi_db_outgoing_u( b_R, &curve.problem );
	y1 = fabs( sin(x1)/cos(x1) * sqrt(1.0 - 2.0*mass_over_r) / (sin(fabs(psi)) * dpsi_db_val));

	/* std::cout << "eps = " << eps
	   << " x1 = " << x1
	   << " cos(alpha) = " << cos(x1)
	   << " b1 = " << b_R
	   << " dpsi = " << dpsi_db_val
	   << " dcosa = " << y1
	   << std::endl;*/

	b_R = sin(x2) / sqrt( 1.0 - 2.0 * mass_over_r );
	psi = defltoa->psi_outgoing_u( b_R, curve.defl.b_R_max, curve.defl.psi_max, &curve.problem);
	dpsi_db_val = defltoa->dpsi_db_outgoing_u( b_R,&curve.problem );
	y2 = fabs( sin(x2)/cos(x2) * sqrt(1.0 - 2.0*mass_over_r) / (sin(fabs(psi)) * dpsi_db_val));

      /* std::cout << "eps = " << eps
		<< " x2 = " << x2
		<< " cos(alpha) = " << cos(x2)
		<< " b2 = " << b_R
		<< " dpsi = " << dpsi_db_val
		<< " dcosa = " << y2
		<< std::endl;*/


	yslope = (y2-y1)/(x2-x1);

	// computation for b > b_mid
	for ( unsigned int i(NN+1); i < 3*NN; i++ ) { /* compute table of b vs psi points */
	  curve.defl.b_psi[i] = b_mid + (curve.defl.b_R_max - b_mid) / 2.0 * (i - NN) / (NN * 1.0); 
	  curve.defl.psi_b[i] = defltoa->psi_outgoing_u(curve.defl.b_psi[i], curve.defl.b_R_max, curve.defl.psi_max, &curve.problem); 
	  sinalpha = curve.defl.b_psi[i] * sqrt(1.0 - 2.0*mass_over_r);
	  curve.defl.toa_b[i] = defltoa->toa_outgoing_u(curve.defl.b_psi[i], &curve.problem );


	  if (cosalpha >= cos(x1)){
	    dpsi_db_val = defltoa->dpsi_db_outgoing_u( curve.defl.b_psi[i], &curve.problem );
	    cosalpha = sqrt(1.0 - pow(sinalpha,2));
	    curve.defl.dcosa_dcosp_b[i] = fabs( sinalpha/cosalpha * sqrt(1.0 - 2.0*mass_over_r) / (sin(curve.defl.psi_b[i]) * dpsi_db_val));
	  }
	  else{
	    alpha = asin(sinalpha);
	    yy = y2 + yslope*(alpha - x2);
	    curve.defl.dcosa_dcosp_b[i] = yy;
	  }
	}
    
	curve.defl.b_psi[3*NN] = curve.defl.b_R_max;   // maximums
	curve.defl.psi_b[3*NN] = curve.defl.psi_max;
	curve.defl.toa_b[3*NN] = defltoa->toa_outgoing_u(curve.defl.b_max/radius, &curve.problem );

	alpha = Units::PI/2.0;
	yy = y2 + yslope*(alpha - x2);
	curve.defl.dcosa_dcosp_b[3*NN]  = yy;
	// Finished computing lookup table
 

	//	out << "%M/R= " << mass_over_r << std::endl;
	//out << "%alpha #b/R #psi #dcosalpha/dcospsi #toa*C/R" << std::endl;

	for (unsigned int i(0); i <= 3*NN; i++ ) { 

	  sinalpha = curve.defl.b_psi[i] * sqrt( 1.0 - 2.0 * mass_over_r );
	  alpha = asin(sinalpha);
				
	  out 
	    << alpha << " " 
	    << curve.defl.b_psi[i] << " " 
	    << curve.defl.psi_b[i] << " " 
	    //	<< sin(curve.defl.psi_b[i]) << " "
	    << curve.defl.dcosa_dcosp_b[i] << " " 
	    << curve.defl.toa_b[i] << " " 
	    << mass_over_r << " "
	    << i
	    << std::endl;	       		
       	   
	}    

	delete defltoa;
      }
      out.close();
    }

    if (readflag){ // Read in from a file
      in.open(in_file);
      char line[265]; // line of the data file being read in
      double get_alpha, get_bR, get_psi, get_dcos, get_toa, get_mr;
      unsigned int get_i;

  

      // Read in value of NN
      in.getline(line,265);      
      sscanf( line, "%u", &numangles);
      if ( numangles != NN)
	std::cerr << "Something is really wrong here: numangles != NN "<< std::endl;

      // Read in value of num_mr
      in.getline(line,265);      
      sscanf( line, "%u", &num_mr);
      std::cout << "num_mr = " << num_mr << std::endl;
      if ( num_mr > MR)
	std::cerr << "Problem: num_mr > MR... check struct.h " << std::endl;
 
      curve.defl.mr = dvector(0,num_mr);
      curve.defl.psi = dmatrix(0,num_mr,0,3*numangles+1);
      curve.defl.b = dmatrix(0,num_mr,0,3*numangles+1);
      curve.defl.dcosa = dmatrix(0,num_mr,0,3*numangles+1);
      curve.defl.toa = dmatrix(0,num_mr,0,3*numangles+1);


      // Loop through the M/R values
      for (unsigned int j(0);j<=num_mr; j++){
	//in.getline(line,265);      
	//sscanf( line, "% M/R= %lf", &get_mr );
	//std::cout << "line = " << line << std::endl;
	//curve.defl.mr[j] = get_mr;
	//in.getline(line,265);  

	for (unsigned int i(0);i<=3*numangles; i++){

	  in.getline(line,265);  
	  sscanf( line, "%lf %lf %lf %lf %lf %lf %u", &get_alpha, &get_bR, &get_psi, &get_dcos, &get_toa, &get_mr, &get_i );

	  if ( get_mr != curve.defl.mr[j] || get_i != i)
	    std::cerr << "BIG PROBLEM!!!! " << std::endl;

	  if (i==0)
	    curve.defl.mr[j] = get_mr;
	  curve.defl.b[j][i] = get_bR;
	  curve.defl.psi[j][i] = get_psi;
	  curve.defl.dcosa[j][i] = get_dcos;
	  curve.defl.toa[j][i] = get_toa;
	  
	} // end-for-i loop

      } // end-for-j loop     

      in.close();
    

    // Interpolate to find bending angles for a particular m/r value

    mr_lo = curve.defl.mr[0];
    mr_hi = curve.defl.mr[num_mr];
    mr_step = (mr_hi-mr_lo)/num_mr;

    mass_over_r = 0.1722729212;

     int jlo, jhi;
    
    if (mass_over_r < mr_hi){
      jlo = (mass_over_r - mr_lo)/mr_step;
      jhi = jlo+1;
    }
    else{
      jlo = num_mr-1;
      jhi = num_mr;
    }

    std::cout << "mass_over_r = " << mass_over_r <<
      " jlo=" << jlo << " m/r[jlo]=" << curve.defl.mr[jlo] << std::endl;
    std::cout << "mass_over_r = " << mass_over_r <<
      " jhi=" << jhi << " m/r[jhi]=" << curve.defl.mr[jhi] << std::endl;
    
    incurve = &curve;

    double *b_int, *psi_int, *dcosa_int, *toa_int;
    psi_int = dvector(0,3*numangles+1);
    b_int = dvector(0,3*numangles+1);
    dcosa_int = dvector(0,3*numangles+1);
    toa_int = dvector(0,3*numangles+1);

    psi_int = interplin( curve.defl.mr, curve.defl.psi, numangles, mass_over_r, &jlo);
    b_int =  interplin( curve.defl.mr, curve.defl.b, numangles, mass_over_r, &jlo);
    dcosa_int =  interplin( curve.defl.mr, curve.defl.dcosa, numangles, mass_over_r, &jlo);
    toa_int =  interplin( curve.defl.mr, curve.defl.toa, numangles, mass_over_r, &jlo);
    }

    delete model;
    return 0;
 } 

catch(std::exception& e) {
       std::cerr << "\nERROR: Exception thrown. " << std::endl
	             << e.what() << std::endl;
       return -1;
}
