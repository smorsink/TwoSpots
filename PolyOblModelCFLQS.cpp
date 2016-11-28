/***************************************************************************************/
/*                               PolyOblModelCFLQS.h

    This is now the old hadronic shape model.
    
    Based on code written by Coire Cadeau and modified by Sharon Morsink and 
    Abigail Stevens.
    
    (C) Coire Cadeau, 2007; Source (C) Coire Cadeau 2007, all rights reserved.
    Permission is granted for private use only, and not distribution, either verbatim or
    of derivative works, in whole or in part.
    This code is not thoroughly tested or guaranteed for any particular use.
*/
/***************************************************************************************/

#include "PolyOblModelCFLQS.h"

PolyOblModelCFLQS::PolyOblModelCFLQS (const double& Req_nounits, const double& zeta, const double& eps )
  : PolyOblModelBase( Req_nounits, zeta, eps) { }

double PolyOblModelCFLQS::a0() const {
  double eps(this->get_eps());
  double zeta(this->get_zeta());
  //return double(-0.26*eps + 0.50*zeta*eps - 0.04*eps*eps);
  return double(-0.18*eps + 0.23*zeta*eps - 0.05*eps*eps);
}

double PolyOblModelCFLQS::a2() const {
  double eps(this->get_eps());
  double zeta(this->get_zeta());
  //return double(-0.53*eps + 0.85*zeta*eps + 0.06*eps*eps);
  return double(-0.39*eps + 0.29*zeta*eps + 0.13*eps*eps); // Old value
}

double PolyOblModelCFLQS::a4() const {
  double eps(this->get_eps());
  double zeta(this->get_zeta());
  //return double(0.02*eps - 0.14*zeta*eps + 0.09*eps*eps );
  return double(0.04*eps - 0.15*zeta*eps + 0.07*eps*eps );
  //return (0.0);
}
