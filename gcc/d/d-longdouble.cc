// d-longdouble.cc -- D frontend for GCC.
// Copyright (C) 2011, 2012 Free Software Foundation, Inc.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include "d-system.h"

#include "lexer.h"
#include "mtype.h"
#include "longdouble.h"

#include "d-lang.h"
#include "d-codegen.h"


// Return backend machine_mode for frontend mode MODE.

static enum machine_mode
machineMode (longdouble::Mode mode)
{
  switch (mode)
    {
    case longdouble::Float:
      return TYPE_MODE (float_type_node);

    case longdouble::Double:
      return TYPE_MODE (double_type_node);

    case longdouble::LongDouble:
      return TYPE_MODE (long_double_type_node);

    default:
      gcc_unreachable();
    }
}

real_properties real_limits[longdouble::NumModes];

#define M_LOG10_2       0.30102999566398119521

// Initialise D floating point property values.

void
longdouble::init (void)
{
  gcc_assert (sizeof (longdouble) >= sizeof (real_value));

  for (int i = (int) Float; i < (int) NumModes; i++)
    {
      real_properties& p = real_limits[i];

      enum machine_mode mode = machineMode ((Mode) i);
      const struct real_format& rf = *REAL_MODE_FORMAT (mode);
      char buf[128];

      /* .max:
	 The largest representable value that's not infinity.  */
      get_max_float (&rf, buf, sizeof (buf));
      real_from_string (&p.maxval.rv(), buf);

      /* .min, .min_normal:
	 The smallest representable normalized value that's not 0.  */
      snprintf (buf, sizeof (buf), "0x1p%d", rf.emin - 1);
      real_from_string (&p.minval.rv(), buf);

      /* .epsilon:
	 The smallest increment to the value 1.  */
      if (rf.pnan < rf.p)
	snprintf (buf, sizeof (buf), "0x1p%d", rf.emin - rf.p);
      else
	snprintf (buf, sizeof (buf), "0x1p%d", 1 - rf.p);
      real_from_string (&p.epsilonval.rv(), buf);

      /* .dig:
	 The number of decimal digits of precision.  */
      p.dig = (rf.p - 1) * M_LOG10_2;

      /* .mant_dig:
	 The number of bits in mantissa.  */
      p.mant_dig = rf.p;

      /* .max_10_exp:
	 The maximum int value such that 10**value is representable.  */
      p.max_10_exp = rf.emax * M_LOG10_2;

      /* .min_10_exp:
	 The minimum int value such that 10**value is representable as a normalized value.  */
      p.min_10_exp = (rf.emin - 1) * M_LOG10_2;

      /* .max_exp:
	 The maximum int value such that 2** (value-1) is representable.  */
      p.max_exp = rf.emax;

      /* .min_exp:
	 The minimum int value such that 2** (value-1) is representable as a normalized value.  */
      p.min_exp = rf.emin;
    }
}

// Return the hidden real_value from the longdouble type.

const real_value &
longdouble::rv (void) const
{
  return *(const real_value *) this;
}

real_value &
longdouble::rv (void)
{
  return *(real_value *) this;
}

// Return conversion of signed integer value D to longdouble.
// Conversion is done at precision mode of TYPE.

longdouble
longdouble::from_shwi (Type *type, int64_t d)
{
  double_int cst = shwi_to_double_int (d);
  REAL_VALUE_FROM_INT (rv(), cst.low, cst.high, TYPE_MODE (type->toCtype()));
  return *this;
}

// Return conversion of unsigned integer value D to longdouble.
// Conversion is done at precision mode of TYPE.

longdouble
longdouble::from_uhwi (Type *type, uint64_t d)
{
  double_int cst = uhwi_to_double_int (d);
  REAL_VALUE_FROM_UNSIGNED_INT (rv(), cst.low, cst.high, TYPE_MODE (type->toCtype()));
  return *this;
}

// Return conversion of longdouble value to int64_t.
// Conversion is done at precision mode of TYPE.

int64_t
longdouble::to_shwi (Type *type) const
{
  double_int cst;

  if (REAL_VALUE_ISNAN (rv()))
    {
      cst.low = 0;
      cst.high = 0;
    }
  else
    {
      tree t = fold_build1 (FIX_TRUNC_EXPR, type->toCtype(),
			    build_float_cst (*this, Type::tfloat64));
      // Can't use tree_low_cst as it asserts !TREE_OVERFLOW
      cst = TREE_INT_CST (t);
    }

  return cst_to_hwi (cst);
}

// Same as longdouble::to_shwi, but returns a uint64_t.

uint64_t
longdouble::to_uhwi (Type *type) const
{
  return (uint64_t) to_shwi (type);
}

// Helper functions which set longdouble to value D.

void
longdouble::set (real_value& r)
{
  real_convert (&rv(), TYPE_MODE (long_double_type_node), &r);
}

longdouble::operator
real_value& (void)
{
  return rv();
}

// Conversion routines between longdouble and host float types.

void
longdouble::set (float d)
{
  char buf[32];
  snprintf(buf, sizeof (buf), "%f", d);
  real_from_string3 (&rv(), buf, TYPE_MODE (double_type_node));
}

void
longdouble::set (double d)
{
  char buf[32];
  snprintf(buf, sizeof (buf), "%lf", d);
  real_from_string3 (&rv(), buf, TYPE_MODE (long_double_type_node));
}

// These functions should never be called.

longdouble::operator
float (void)
{
  gcc_unreachable();
}

longdouble::operator
double (void)
{
  gcc_unreachable();
}

// For conversion between boolean, only need to check if is zero.

void
longdouble::set (bool d)
{
  rv() = (d == false) ? dconst0 : dconst1;
}

longdouble::operator
bool (void)
{
  return rv().cl != rvc_zero;
}

// Conversion routines between longdouble and integer types.

void longdouble::set (int8_t d)  { from_shwi (Type::tfloat32, d); }
void longdouble::set (int16_t d) { from_shwi (Type::tfloat32, d); }
void longdouble::set (int32_t d) { from_shwi (Type::tfloat64, d); }
void longdouble::set (int64_t d) { from_shwi (Type::tfloat80, d); }

longdouble::operator int8_t (void)  { return to_shwi (Type::tint8); }
longdouble::operator int16_t (void) { return to_shwi (Type::tint16); }
longdouble::operator int32_t (void) { return to_shwi (Type::tint32); }
longdouble::operator int64_t (void) { return to_shwi (Type::tint64); }

void longdouble::set (uint8_t d)  { from_uhwi (Type::tfloat32, d); }
void longdouble::set (uint16_t d) { from_uhwi (Type::tfloat32, d); }
void longdouble::set (uint32_t d) { from_uhwi (Type::tfloat64, d); }
void longdouble::set (uint64_t d) { from_uhwi (Type::tfloat80, d); }

longdouble::operator uint8_t (void)  { return to_uhwi (Type::tuns8); }
longdouble::operator uint16_t (void) { return to_uhwi (Type::tuns16); }
longdouble::operator uint32_t (void) { return to_uhwi (Type::tuns32); }
longdouble::operator uint64_t (void) { return to_uhwi (Type::tuns64); }

// Overload numeric operators for longdouble types.

longdouble
longdouble::operator + (const longdouble& r)
{
  real_value x;
  REAL_ARITHMETIC (x, PLUS_EXPR, rv(), r.rv());
  return ldouble (x);
}

longdouble
longdouble::operator - (const longdouble& r)
{
  real_value x;
  REAL_ARITHMETIC (x, MINUS_EXPR, rv(), r.rv());
  return ldouble (x);
}

longdouble
longdouble::operator * (const longdouble& r)
{
  real_value x;
  REAL_ARITHMETIC (x, MULT_EXPR, rv(), r.rv());
  return ldouble (x);
}

longdouble
longdouble::operator / (const longdouble& r)
{
  real_value x;
  REAL_ARITHMETIC (x, RDIV_EXPR, rv(), r.rv());
  return ldouble (x);
}

longdouble
longdouble::operator % (const longdouble& r)
{
  real_value q, x;

  if (r.rv().cl == rvc_zero || REAL_VALUE_ISINF (rv()))
    {
      real_value rvt;
      real_nan (&rvt, "", 1, TYPE_MODE (long_double_type_node));
      return ldouble (rvt);
    }

  if (rv().cl == rvc_zero)
    return *this;

  if (REAL_VALUE_ISINF (r.rv()))
    return *this;

  // Need to check for NaN?
  REAL_ARITHMETIC (q, RDIV_EXPR, rv(), r.rv());
  real_arithmetic (&q, FIX_TRUNC_EXPR, &q, NULL);
  REAL_ARITHMETIC (q, MULT_EXPR, q, r.rv());
  REAL_ARITHMETIC (x, MINUS_EXPR, rv(), q);

  return ldouble (x);
}

longdouble
longdouble::operator - (void)
{
  real_value x = real_value_negate (&rv());
  return ldouble (x);
}

// Overload equality operators for longdouble types.

bool
longdouble::operator < (const longdouble& r)
{
  return real_compare (LT_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator > (const longdouble& r)
{
  return real_compare (GT_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator <= (const longdouble& r)
{
  return real_compare (LE_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator >= (const longdouble& r)
{
  return real_compare (GE_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator == (const longdouble& r)
{
  return real_compare (EQ_EXPR, &rv(), &r.rv());
}

bool
longdouble::operator != (const longdouble& r)
{
  return real_compare (NE_EXPR, &rv(), &r.rv());
}

// Returns TRUE if longdouble value is identical to R.

bool
longdouble::isIdenticalTo (const longdouble& r) const
{
  return REAL_VALUES_IDENTICAL (rv(), r.rv());
}

// Format longdouble value into decimal string BUF of size BUF_SIZE.

int
longdouble::format (char *buf, unsigned buf_size) const
{
  // %% restricting the precision of significant digits to 18.
  real_to_decimal (buf, &rv(), buf_size, 18, 1);
  return strlen (buf);
}

// Format longdouble value into hex string BUF of size BUF_SIZE.

int
longdouble::formatHex (char *buf, unsigned buf_size) const
{
  real_to_hexadecimal (buf, &rv(), buf_size, 0, 1);
  return strlen (buf);
}

// Dump value of longdouble for debugging purposes.

void
longdouble::dump (void)
{
  char buf[128];
  format (buf, sizeof (buf));
  fprintf (stderr, "%s\n", buf);
}

