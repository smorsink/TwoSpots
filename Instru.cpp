/***************************************************************************************/
/*                                    Instru.cpp

    This holds the instrument-specifc routines used in Spot.cpp. Contains attenuation tables
    pre-calculated for NICER and instrument response curve. 

*/
/***************************************************************************************/

#include "matpack.h"
#include <exception>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>
#include "Atmo.h"
#include "OblDeflectionTOA.h"
#include "OblModelBase.h"
#include "PolyOblModelNHQS.h"
#include "Exception.h"
#include "Units.h"
#include "Struct.h"
#include "time.h"
#include <stdio.h>
using namespace std;


class LightCurve Attenuate (class LightCurve* incurve, unsigned int attenuation, double nh){

	class LightCurve curve, newcurve;
	double temp;
	char cwd[1024], ismdir[1024];
	std::vector<double> atten_ener, atten_width, atten_factors;
	unsigned int atten_size(0), numbands, numbins;

	//Pour numbands and numbins into newcurve

	curve = (*incurve);
	numbands = curve.numbands;
	numbins = curve.numbins;
	newcurve.numbands = numbands;
	newcurve.numbins = numbins;

    //Read in ism attenuation factors
    getcwd(cwd, sizeof(cwd));
    sprintf(ismdir,"%s/ISM",cwd);
    chdir(ismdir);

	//cout << "Reading Attenuation File" << std::endl;
	ifstream file;
	switch (attenuation) {
		case 1: // j0030 WABS
			file.open("j0030_wabs_1.8e20.txt");
			break;
		case 2: // j0030 TBABS
			file.open("j0030_tbabs_1.8e20.txt");
			break;
		case 3: // j0437 WABS
			file.open("j0437.wabs_4e19.txt");
			break;
		case 4: // j0437 TBABS
			file.open("j0437.tbabs_4e19.txt");
			break;
		case 5: //
			file.open("tbnew_4e19.txt");
			break;
	}

	if(file.is_open()){
        while (file >> temp) {
            atten_ener.push_back(temp);
            file >> temp;
            atten_width.push_back(temp);
            file >> temp;
            atten_factors.push_back(temp);
            atten_size += 1;
            //cout << temp << endl;
        }
    }
    else{
        cout << "attenuation file is not found" << endl;
    }
    file.close();
	chdir(cwd);

	for (unsigned int p = 0; p < numbands; p++){
        for (unsigned int i = 0; i < numbins; i++){
        	if (p >= atten_size){
        		newcurve.f[p][i] = curve.f[p][i];
        	} 
        	else {
        		newcurve.f[p][i] = curve.f[p][i] * pow(atten_factors[p],nh);
        	}

        	/*
        	if (newcurve.f[p][i] > incurve.f[p][i]){
				cout << "attenuation factor greater than 1!" << endl;
        	}
        	*/
        }
	}
	return newcurve;
}

class LightCurve Inst_Res (class LightCurve* incurve, unsigned int inst_curve){

	class LightCurve newcurve, curve;
	unsigned int response_size(0), numbands, numbins;
	double temp;
	char cwd[1024], resdir[1024];
	std::vector<double> response_list;

	//Pour numbands and numbins into newcurve
	curve = (*incurve);
	numbands = curve.numbands;
	numbins = curve.numbins;
	newcurve.numbands = numbands;
	newcurve.numbins = numbins;

	getcwd(cwd, sizeof(cwd));
	sprintf(resdir,"%s/Area",cwd);
	chdir(resdir);

	ifstream file;
	file.open("NICER_fine_area.txt");

	if(file.is_open()) {
		while (file >> temp) {
			file >> temp;
			response_list.push_back(temp);
			response_size += 1;
		}
	}else{
        throw( Exception( "instrument response curve file is not found" ));
    }
    file.close();
	chdir(cwd);

	for (unsigned int p = 0; p < numbands; p++){
        for (unsigned int i = 0; i < numbins; i++){
        	if (p >= response_size){
		 		throw( Exception( "instrument response curve doesn't have this band" ));
        	} 
        	else {
        		newcurve.f[p][i] = curve.f[p][i] * response_list[p];
        	}
        }
	}
	return newcurve;
}

class LightCurve Background_list (class LightCurve* incurve, char *background_file){

	class LightCurve newcurve, curve;
	unsigned int background_size(0), numbands, numbins;
	double temp;
	char cwd[1024], bgddir[1024];
	std::vector<double> background_list;

	//Pour numbands and numbins into newcurve
	curve = (*incurve);
	numbands = curve.numbands;
	numbins = curve.numbins;
	newcurve.numbands = numbands;
	newcurve.numbins = numbins;

    //Read in background numbers
    getcwd(cwd, sizeof(cwd));
    sprintf(bgddir,"%s/Background",cwd);
    chdir(bgddir);

	ifstream file;
	//std::cout << "background_file = " << background_file << std::endl;
	file.open(background_file);

	if(file.is_open()){
        while (file >> temp) {
            background_list.push_back(temp);
            background_size += 1;
        }
	}
	else{
	  throw( Exception( "background list file is not found" ));
	}
	file.close();
	chdir(cwd);


	for (unsigned int p = 0; p < numbands; p++){
        for (unsigned int i = 0; i < numbins; i++){
        	if (p >= background_size){
        		newcurve.f[p][i] = curve.f[p][i];
        	} 
        	else {
        		newcurve.f[p][i] = curve.f[p][i] + background_list[p];
        	}
        }
	}
	return newcurve;
}


class LightCurve AGN_Background (class LightCurve* incurve, double agnbackground, double nh){

	class LightCurve newcurve, curve;
	unsigned int numbands, numbins;
	double temp, gamma, n(0.0), A;
	char cwd[1024], bgddir[1024];
	std::vector<double> nicer_area, ism;

	//Pour numbands, numbins, and nh into newcurve
	curve = (*incurve);
	numbands = curve.numbands;
	numbins = curve.numbins;
	//nh = curve.nh;
	newcurve.numbands = numbands;
	newcurve.numbins = numbins;
	//newcurve.nh = nh;

	//Read in NICER effective area
    getcwd(cwd, sizeof(cwd));
    sprintf(bgddir,"%s/Area",cwd);
    chdir(bgddir);

    ifstream file;
	file.open("NICER_fine_area.txt");

	if(file.is_open()){
        while (file >> temp) {
        	file >> temp;
            nicer_area.push_back(temp);
        }
	}
	else{
	  throw( Exception( "nicer_area_file is not found" ));
	}
	file.close();
	chdir(cwd);

    //Read in ISM factors
    getcwd(cwd, sizeof(cwd));
    sprintf(bgddir,"%s/ISM",cwd);
    chdir(bgddir);

    ifstream file1;
	file1.open("tbnew_4e19.txt");

	if(file1.is_open()){
        while (file1 >> temp) {
        	file1 >> temp;
        	file1 >> temp;
            ism.push_back(temp);
        }
	}
	else{
	  throw( Exception( "ISM file is not found" ));
	}
	file1.close();
	chdir(cwd);

	//Calculate Normalization

	gamma = -2.59;

	for (unsigned int i = 0; i < 301; i++){
		n += nicer_area[i] * pow((i/100+0.1),gamma) * 0.01 * pow(ism[i],nh);
	}
	A = agnbackground/n;


	// Apply Background
	for (unsigned int p = 0; p < numbands; p++){
        for (unsigned int i = 0; i < numbins; i++){
        	newcurve.f[p][i] = curve.f[p][i] + nicer_area[p] * pow((p/100+0.1),gamma) * A * 0.01 * pow(ism[p],nh) / numbins;        	
        }
	}

    return newcurve;
}


class LightCurve Sky_Background (class LightCurve* incurve, double skybackground){

	class LightCurve newcurve, curve;
	unsigned int numbands, numbins;
	double temp;
	char cwd[1024], bgddir[1024];
	std::vector<double> skyback;

	//Pour numbands and numbins into newcurve
	curve = (*incurve);
	numbands = curve.numbands;
	numbins = curve.numbins;
	//nh = curve.nh;
	newcurve.numbands = numbands;
	newcurve.numbins = numbins;
	//newcurve.nh = nh;

    //Read in background numbers
    getcwd(cwd, sizeof(cwd));
    sprintf(bgddir,"%s/Background",cwd);
    chdir(bgddir);

	ifstream file;
	//std::cout << "background_file = " << background_file << std::endl;
	file.open("skyback_1191.txt");

	if(file.is_open()){
        while (file >> temp) {
        	file >> temp;
            skyback.push_back(temp);
        }
	}
	else{
	  throw( Exception( "Diffuse sky background file is not found" ));
	}
	file.close();
	chdir(cwd);



	for (unsigned int p = 0; p < numbands; p++){
        for (unsigned int i = 0; i < numbins; i++){
        		newcurve.f[p][i] = curve.f[p][i] + skyback[p]*skybackground/0.3*16/numbins;
				//This list is normalized to 0.3 counts per second with 16 phse bins, calculated using May 2014 NICER effective area	
        }
	}
	return newcurve;
}

