// Copyright (c) 1999,2000  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
// See the file LICENSE.LGPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL: svn+ssh://scm.gforge.inria.fr/svn/cgal/branches/CGAL-3.4-branch/Filtered_kernel/include/CGAL/Static_filter_error.h $
// $Id: Static_filter_error.h 40822 2007-11-07 16:51:18Z ameyer $
// 
//
// Author(s)     : Sylvain Pion

#ifndef CGAL_STATIC_FILTER_ERROR_H
#define CGAL_STATIC_FILTER_ERROR_H

// This file contains the description of the class Static_filter_error.
// The goal of this class is to be run by some overloaded predicates,
// to compute error bound done in these functions.
// 
// The original idea is from Olivier Devillers.

// It is still EXPERIMENTAL.

// TODO:
// - I need to add some missing operators and functions, min/max...
// - Remove the degree stuff, it's only meant for debug (?).
// - Add __attribute__((const)) for optimizing ?

#include <CGALmini/basic.h>
#include <CGALmini/FPU.h>

CGAL_BEGIN_NAMESPACE

struct Static_filter_error
{
  typedef Static_filter_error Sfe;

  Static_filter_error () {}

  Static_filter_error (const int &i, const double &e = 0, const int &d = 1)
      : _b(i), _e(e), _d(d) {}

  Static_filter_error (const double &b, const double &e = 0, const int &d = 1)
      : _b(b), _e(e), _d(d) {}

  static double ulp ()
  {
      FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
      double e = ulp(1);
      FPU_set_cw(backup);
      return e;
  }

  static double ulp (double d)
  {
      // You are supposed to call this function with rounding towards
      // +infinity, and on a positive number.
      d = CGAL_IA_FORCE_TO_DOUBLE(d); // stop constant propagation.
      CGAL_assertion(d>=0);
      double u;
      if (d == 1) // I need to special case to prevent infinite recursion.
          u = (d + CGAL_IA_MIN_DOUBLE) - d;
      else {
          // We need to use the d*ulp formula, in order for the formal proof
          // of homogeneisation to work.
          // u = (d + CGAL_IA_MIN_DOUBLE) - d;
          u = d * ulp();
      }

      // Then add extra bonus, because of Intel's extended precision feature.
      // (ulp can be 2^-53 + 2^-64)
      u += u / (1<<11);
      CGAL_assertion(u!=0);
      return u;
  }

  Sfe operator+ (const Sfe &f) const
  {
      CGAL_warning_msg( _d == f._d ,
	      "you are adding variables of different homogeneous degree");
      // We have to add an ulp, since the homogeneization could induce such
      // an error.
      FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
      double b = _b + f._b;
      double u = ulp(b) / 2;
      b += u;
      double e = u + _e + f._e;
      FPU_set_cw(backup);
      return Sfe(b, e, _d);
  }

  Sfe operator* (const Sfe &f) const
  {
      // We have to add an ulp, since the homogeneization could induce such
      // an error.
      FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
      double b = _b * f._b;
      double u = ulp(b) / 2;
      b += u;
      double e = u + _e * f._e + _e * f._b + _b * f._e;
      FPU_set_cw(backup);
      return Sfe(b, e, _d+f._d);
  }

  Sfe operator- (const Sfe &f) const { return *this + f; }
  Sfe operator- ()             const { return *this; }
  // Sfe operator/ (const Sfe &) const { CGAL_error(); }
  // Division not supported.

  Sfe& operator+=(const Sfe &f) { return *this = *this + f; }
  Sfe& operator-=(const Sfe &f) { return *this = *this - f; }
  Sfe& operator*=(const Sfe &f) { return *this = *this * f; }
  // Sfe& operator/=(const Sfe &f) { return *this = *this / f; }

  double error()  const { return _e; }
  double bound()  const { return _b; }
  int    degree() const { return _d; }

  bool operator< (const Sfe &f) const
  {
      Sfe e = *this + f;
      std::cerr << "Static error is : " << e.error() << std::endl;
      CGAL_error();
      return false;
  }
  bool operator> (const Sfe &f) const { return *this < f; }
  bool operator<=(const Sfe &f) const { return *this < f; }
  bool operator>=(const Sfe &f) const { return *this < f; }
  bool operator==(const Sfe &f) const { return *this < f; }
  bool operator!=(const Sfe &f) const { return *this < f; }

private:
  // _b is a bound on the absolute value of the _double_ value of the
  //    variable.
  // _e is a bound on the absolute error (difference between _b and the
  //    _real_ value of the variable.
  // _d is the degree of the variable, it allows some additionnal checks.
  double _b, _e;
  int _d;
};

inline
Static_filter_error
sqrt(const Static_filter_error &f)
{
  CGAL_warning_msg( (f.degree() & 1) == 0,
	            "Do you really want a non integral degree ???");
  // We have to add an ulp, since the homogeneization could induce such
  // an error.
  FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
  double b = std::sqrt(f.bound());
  double u = Static_filter_error::ulp(b) / 2;
  b += u;
  double e = std::sqrt(f.error()) + u;
  FPU_set_cw(backup);
  return Static_filter_error(b, e, f.degree()/2);
}


/*******************************************************************************/

struct Static_filter_error_float
{
  typedef Static_filter_error_float Sfe;
    
  Static_filter_error_float () {}

  Static_filter_error_float (const int &i, const float &e = 0, const int &d = 1)
      : _b(i), _e(e), _d(d) {}

  Static_filter_error_float (const float &b, const float &e = 0, const int &d = 1)
      : _b(b), _e(e), _d(d) {}

  static float ulp ()
  {
      FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
      float e = ulp(1);
      FPU_set_cw(backup);
      return e;
  }

  static float ulp (float d)
  {
      // You are supposed to call this function with rounding towards
      // +infinity, and on a positive number.
      d = CGAL_IA_FORCE_TO_FLOAT(d); // stop constant propagation.
      CGAL_assertion(d>=0);
      float u;
      if (d == 1) // I need to special case to prevent infinite recursion.
          u = (d + CGAL_IA_MIN_FLOAT) - d;
      else {
          // We need to use the d*ulp formula, in order for the formal proof
          // of homogeneisation to work.
          // u = (d + CGAL_IA_MIN_FLOAT) - d;
          u = d * ulp();
      }

      /*
	// [BL] I do not think this is still relevant now, and
        //   this is for double, so I do not know what to do for float.
	//
        // Then add extra bonus, because of Intel's extended precision feature.
        // (ulp can be 2^-53 + 2^-64)
      u += u / (1<<11);
      */
            
      CGAL_assertion(u!=0);
      return u;
  }

  Sfe operator+ (const Sfe &f) const
  {
      CGAL_warning_msg( _d == f._d ,
	      "you are adding variables of different homogeneous degree");
      // We have to add an ulp, since the homogeneization could induce such
      // an error.
      FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
      float b = _b + f._b;
      float u = ulp(b) / 2;
      b += u;
      float e = u + _e + f._e;
      FPU_set_cw(backup);
      return Sfe(b, e, _d);
  }

  Sfe operator* (const Sfe &f) const
  {
      // We have to add an ulp, since the homogeneization could induce such
      // an error.
      FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
      float b = _b * f._b;
      float u = ulp(b) / 2;
      b += u;
      float e = u + _e * f._e + _e * f._b + _b * f._e;
      FPU_set_cw(backup);
      return Sfe(b, e, _d+f._d);
  }

  Sfe operator- (const Sfe &f) const { return *this + f; }
  Sfe operator- ()             const { return *this; }
  // Sfe operator/ (const Sfe &) const { CGAL_error(); }
  // Division not supported.

  Sfe& operator+=(const Sfe &f) { return *this = *this + f; }
  Sfe& operator-=(const Sfe &f) { return *this = *this - f; }
  Sfe& operator*=(const Sfe &f) { return *this = *this * f; }
  // Sfe& operator/=(const Sfe &f) { return *this = *this / f; }

  float error()  const { return _e; }
  float bound()  const { return _b; }
  int    degree() const { return _d; }

  bool operator< (const Sfe &f) const
  {
      Sfe e = *this + f;
      std::cerr << "Static error is : " << e.error() << std::endl;
      CGAL_error();
      return false;
  }
  bool operator> (const Sfe &f) const { return *this < f; }
  bool operator<=(const Sfe &f) const { return *this < f; }
  bool operator>=(const Sfe &f) const { return *this < f; }
  bool operator==(const Sfe &f) const { return *this < f; }
  bool operator!=(const Sfe &f) const { return *this < f; }

private:
  // _b is a bound on the absolute value of the _float_ value of the
  //    variable.
  // _e is a bound on the absolute error (difference between _b and the
  //    _real_ value of the variable.
  // _d is the degree of the variable, it allows some additionnal checks.
  float _b, _e;
  int _d;
};

inline
Static_filter_error_float
sqrt(const Static_filter_error_float &f)
{
  CGAL_warning_msg( (f.degree() & 1) == 0,
	            "Do you really want a non integral degree ???");
  // We have to add an ulp, since the homogeneization could induce such
  // an error.
  FPU_CW_t backup = FPU_get_and_set_cw(CGAL_FE_UPWARD);
  float b = std::sqrt(f.bound());
  float u = Static_filter_error_float::ulp(b) / 2;
  b += u;
  float e = std::sqrt(f.error()) + u;
  FPU_set_cw(backup);
  return Static_filter_error_float(b, e, f.degree()/2);
}

CGAL_END_NAMESPACE

#endif // CGAL_STATIC_FILTER_ERROR_H
