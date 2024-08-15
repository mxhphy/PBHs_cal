#ifndef __PBHS_PS_METHOD_PS_GENERATE_MASS__   /* Include guard */
#define __PBHS_PS_METHOD_PS_GENERATE_MASS__

#include "../new_type.h"
#include "../phy_constant.h"
#include "../math_method.h"
#include "../general.h"

#include "covariance.h"
#include "zeta_probability.h"
#include "C_l_probability.h" 

int PS_C_l_to_M_ratio(arb_t res, const arb_t cl, slong prec); //黑洞质量m与C_l的关系，已知 C_l 求 m

int PS_M_ratio_to_C_l(arb_t res, const arb_t m, slong prec); //黑洞质量m与C_l的关系，已知 m 求 C_l

void Func_get_relative_M_max(arb_t press_schechter, arb_t peak_theory, slong prec); //获得相对质量取值的最大值


#endif // __PBHS_PS_METHOD_PS_GENERATE_MASS__  
 
