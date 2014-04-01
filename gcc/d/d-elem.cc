// d-elem.cc -- D frontend for GCC.
// Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

#include "d-lang.h"
#include "d-codegen.h"

#include "id.h"
#include "module.h"
#include "ctfe.h"

elem *
Expression::toElem (IRState *)
{
  error ("abstract Expression::toElem called");
  return error_mark_node;
}

elem *
CondExp::toElem (IRState *irs)
{
  tree cn = convert_for_condition (econd->toElem (irs), econd->type);
  tree t1 = convert_expr (e1->toElemDtor (irs), e1->type, type);
  tree t2 = convert_expr (e2->toElemDtor (irs), e2->type, type);
  return build3 (COND_EXPR, type->toCtype(), cn, t1, t2);
}

elem *
IdentityExp::toElem (IRState *irs)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  tree_code code = (op == TOKidentity) ? EQ_EXPR : NE_EXPR;

  if ((tb1->ty == Tsarray || tb1->ty == Tarray)
      && (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
      // Convert arrays to D array types.
      return build2 (code, type->toCtype(),
		     d_array_convert (e1), d_array_convert (e2));
    }
  else if (tb1->isfloating())
    {
      tree t1 = e1->toElem (irs);
      tree t2 = e2->toElem (irs);
      // Assume all padding is at the end of the type.
      tree size = build_integer_cst (TYPE_PRECISION (e1->type->toCtype()) / BITS_PER_UNIT);

      // Do bit compare of floats.
      tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					build_address (t1), build_address (t2), size);

      return build_boolop (code, tmemcmp, integer_zero_node);
    }
  else if (tb1->ty == Tstruct)
    {
      tree t1 = e1->toElem (irs);
      tree t2 = e2->toElem (irs);

      if (TYPE_MODE (TREE_TYPE (t1)) != BLKmode)
	{
	  // Bitwise comparison of small structs not returned in memory may
	  // not work due to data holes loosing its zero padding upon return.
	  // Instead do field-by-field comparison of the two structs.
	  StructDeclaration *sd = ((TypeStruct *) tb1)->sym;
	  gcc_assert (d_types_same (tb1, tb2));

	  // Make temporaries to prevent multiple evaluations.
	  t1 = maybe_make_temp (t1);
	  t2 = maybe_make_temp (t2);

	  return build_struct_memcmp (code, sd, t1, t2);
	}
      else
	{
	  // Do bit compare of structs.
	  tree size = build_integer_cst (e1->type->size());

	  tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					    build_address (t1), build_address (t2), size);

	  return build_boolop (code, tmemcmp, integer_zero_node);
	}
    }
  else
    {
      // For operands of other types, identity is defined as being the same as equality.
      tree tcmp = build_boolop (code, e1->toElem (irs), e2->toElem (irs));

      return d_convert (type->toCtype(), tcmp);
    }
}

elem *
EqualExp::toElem (IRState *irs)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  tree_code code = (op == TOKequal) ? EQ_EXPR : NE_EXPR;

  if (tb1->ty == Tstruct)
    {
      // Do bit compare of structs
      tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					build_address (e1->toElem (irs)),
					build_address (e2->toElem (irs)),
					build_integer_cst (e1->type->size()));

      return build2 (code, type->toCtype(), tmemcmp, integer_zero_node);
    }
  else if ((tb1->ty == Tsarray || tb1->ty == Tarray)
	   && (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
      Type *t1elem = tb1->nextOf()->toBasetype();
      Type *t2elem = tb1->nextOf()->toBasetype();

      if ((t1elem->isintegral() || t1elem->ty == Tvoid) && t1elem->ty == t2elem->ty)
	{
	  // Optimise comparisons of arrays of basic types.
	  // For arrays of integers/characters, and void[], replace _adEq2 call with:
	  //     e1 == e2  =>  e1.length == e2.length && memcmp (e1.ptr, e2.ptr, size) == 0;
	  //     e1 != e2  =>  e1.length != e2.length || memcmp (e1.ptr, e2.ptr, size) != 0;
	  // 'size' is e1.length * sizeof(e1[0]) for dynamic arrays, or sizeof(e1) for static arrays.
	  tree t1 = e1->toElem (irs);
	  tree t2 = e2->toElem (irs);
	  // Length, for comparison.
	  tree t1len, t2len;
	  // Pointer to data and data size, to pass to memcmp.
	  tree t1ptr, t2ptr;
	  tree t1size, t2size;

	  // Make temporaries to prevent multiple evaluations.
	  tree t1saved = maybe_make_temp (t1);
	  tree t2saved = maybe_make_temp (t2);

	  if (tb1->ty == Tarray)
	    {
	      t1len = d_array_length (t1saved);
	      t1ptr = d_array_ptr (t1saved);
	      t1size = build2 (MULT_EXPR, size_type_node, t1len, size_int (t1elem->size()));
	    }
	  else
	    {
	      t1len = size_int (((TypeSArray *) tb1)->dim->toInteger());
	      t1ptr = build_address (t1saved);
	      t1size = size_int (tb1->size());
	    }

	  if (tb2->ty == Tarray)
	    {
	      t2len = d_array_length (t2saved);
	      t2ptr = d_array_ptr (t2saved);
	      t2size = build2 (MULT_EXPR, size_type_node, t2len, size_int (t2elem->size()));
	    }
	  else
	    {
	      t2len = size_int (((TypeSArray *) tb2)->dim->toInteger());
	      t2ptr = build_address (t2saved);
	      t2size = size_int (tb2->size());
	    }

	  tree tmemcmp = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCMP), 3,
					    t1ptr, t2ptr, (tb2->ty == Tsarray) ? t2size : t1size);

	  tree result = build2 (code, type->toCtype(), tmemcmp, integer_zero_node);

	  if (tb1->ty == Tsarray && tb2->ty == Tsarray)
	    gcc_assert (tb1->size() == tb2->size());
	  else
	    {
	      tree_code tcode = (op == TOKequal) ? TRUTH_ANDIF_EXPR : TRUTH_ORIF_EXPR;
	      tree tlencmp = build2 (code, size_type_node, t1len, t2len);

	      result = build_boolop (tcode, tlencmp, result);
	    }

	  // Ensure left-to-right order of evaluation.
	  if (t2 != t2saved)
	    result = compound_expr (t2saved, result);

	  if (t1 != t1saved)
	    result = compound_expr (t1saved, result);

	  return result;
	}
      else
	{
	  // _adEq2 compares each element.
	  tree args[3];
	  tree result;

	  args[0] = d_array_convert (e1);
	  args[1] = d_array_convert (e2);
	  args[2] = build_typeinfo (t1elem->arrayOf());

	  result = d_convert (type->toCtype(), build_libcall (LIBCALL_ADEQ2, 3, args));

	  if (op == TOKnotequal)
	    return build1 (TRUTH_NOT_EXPR, type->toCtype(), result);

	  return result;
	}
    }
  else if (tb1->ty == Taarray && tb2->ty == Taarray)
    {
      TypeAArray *taa1 = (TypeAArray *) tb1;
      tree args[3];
      tree result;

      args[0] = build_typeinfo (taa1);
      args[1] = e1->toElem (irs);
      args[2] = e2->toElem (irs);
      result = d_convert (type->toCtype(), build_libcall (LIBCALL_AAEQUAL, 3, args));

      if (op == TOKnotequal)
	return build1 (TRUTH_NOT_EXPR, type->toCtype(), result);

      return result;
    }
  else
    {
      tree tcmp = build_boolop (code, e1->toElem (irs), e2->toElem (irs));

      return d_convert (type->toCtype(), tcmp);
    }
}

elem *
InExp::toElem (IRState *irs)
{
  Type *tb2 = e2->type->toBasetype();
  AddrOfExpr aoe;
  gcc_assert (tb2->ty == Taarray);

  Type *tkey = ((TypeAArray *) tb2)->index->toBasetype();
  tree args[3];

  args[0] = e2->toElem (irs);
  args[1] = build_typeinfo (tkey);
  args[2] = aoe.set (convert_expr (e1->toElem (irs), e1->type, tkey));

  return convert (type->toCtype(),
		  aoe.finish (build_libcall (LIBCALL_AAINX, 3, args)));
}

elem *
CmpExp::toElem (IRState *irs)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  tree result;
  tree_code code;

  switch (op)
    {
    case TOKue:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNEQ_EXPR : EQ_EXPR;
      break;

    case TOKlg:
      code = tb1->isfloating() && tb2->isfloating() ?
	LTGT_EXPR : NE_EXPR;
      break;

    case TOKule:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNLE_EXPR : LE_EXPR;
      break;

    case TOKul:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNLT_EXPR : LT_EXPR;
      break;

    case TOKuge:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNGE_EXPR : GE_EXPR;
      break;

    case TOKug:
      code = tb1->isfloating() && tb2->isfloating() ?
	UNGT_EXPR : GT_EXPR;
      break;

    case TOKle:
      code = LE_EXPR;
      break;

    case TOKlt:
      code = LT_EXPR;
      break;

    case TOKge:
      code = GE_EXPR;
      break;

    case TOKgt:
      code = GT_EXPR;
      break;

    case TOKleg:
      code = ORDERED_EXPR;
      break;

    case TOKunord:
      code = UNORDERED_EXPR;
      break;

    default:
      gcc_unreachable();
    }

  if ((tb1->ty == Tsarray || tb1->ty == Tarray)
      && (tb2->ty == Tsarray || tb2->ty == Tarray))
    {
      Type *telem = tb1->nextOf()->toBasetype();
      tree args[3];

      args[0] = d_array_convert (e1);
      args[1] = d_array_convert (e2);
      args[2] = build_typeinfo (telem->arrayOf());
      result = build_libcall (LIBCALL_ADCMP2, 3, args);

      // %% For float element types, warn that NaN is not taken into account?

      // %% Could do a check for side effects and drop the unused condition
      if (code == ORDERED_EXPR)
	return build_boolop (COMPOUND_EXPR, result,
			     d_truthvalue_conversion (integer_one_node));

      if (code == UNORDERED_EXPR)
	return build_boolop (COMPOUND_EXPR, result,
			     d_truthvalue_conversion (integer_zero_node));

      result = build_boolop (code, result, integer_zero_node);
      return d_convert (type->toCtype(), result);
    }
  else
    {
      if (!tb1->isfloating() || !tb2->isfloating())
	{
	  // %% is this properly optimized away?
	  if (code == ORDERED_EXPR)
	    return convert (boolean_type_node, integer_one_node);

	  if (code == UNORDERED_EXPR)
	    return convert (boolean_type_node, integer_zero_node);
	}

      result = build_boolop (code, e1->toElem (irs), e2->toElem (irs));
      return d_convert (type->toCtype(), result);
    }
}

elem *
AndAndExp::toElem (IRState *irs)
{
  if (e2->type->toBasetype()->ty != Tvoid)
    {
      tree t1 = convert_for_condition (e1->toElem (irs), e1->type);
      tree t2 = convert_for_condition (e2->toElem (irs), e2->type);

      return d_convert (type->toCtype(), build_boolop (TRUTH_ANDIF_EXPR, t1, t2));
    }
  else
    {
      return build3 (COND_EXPR, type->toCtype(),
		     convert_for_condition (e1->toElem (irs), e1->type),
		     e2->toElemDtor (irs), d_void_zero_node);
    }
}

elem *
OrOrExp::toElem (IRState *irs)
{
  if (e2->type->toBasetype()->ty != Tvoid)
    {
      tree t1 = convert_for_condition (e1->toElem (irs), e1->type);
      tree t2 = convert_for_condition (e2->toElem (irs), e2->type);

      return d_convert (type->toCtype(), build_boolop (TRUTH_ORIF_EXPR, t1, t2));
    }
  else
    {
      return build3 (COND_EXPR, type->toCtype(),
		     build1 (TRUTH_NOT_EXPR, boolean_type_node,
			     convert_for_condition (e1->toElem (irs), e1->type)),
		     e2->toElemDtor (irs), d_void_zero_node);
    }
}

elem *
XorExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (BIT_XOR_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
OrExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (BIT_IOR_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
AndExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (BIT_AND_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
UshrExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (UNSIGNED_RSHIFT_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
ShrExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (RSHIFT_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
ShlExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (LSHIFT_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
ModExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (e1->type->isfloating() ? FLOAT_MOD_EXPR : TRUNC_MOD_EXPR,
			  type->toCtype(), e1->toElem (irs), e2->toElem (irs));
}

elem *
DivExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (e1->type->isintegral() ? TRUNC_DIV_EXPR : RDIV_EXPR,
			  type->toCtype(), e1->toElem (irs), e2->toElem (irs));
}

elem *
MulExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  return build_binary_op (MULT_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
PowExp::toElem (IRState *irs)
{
  tree e1_t, e2_t;
  tree powtype, powfn = NULL_TREE;
  Type *tb1 = e1->type->toBasetype();

  // Dictates what version of pow() we call.
  powtype = type->toBasetype()->toCtype();
  // If type is int, implicitly convert to double.
  // This allows backend to fold the call into a constant return value.
  if (type->isintegral())
    powtype = double_type_node;

  // Lookup compatible builtin. %% TODO: handle complex types?
  if (TYPE_MAIN_VARIANT (powtype) == double_type_node)
    powfn = builtin_decl_explicit (BUILT_IN_POW);
  else if (TYPE_MAIN_VARIANT (powtype) == float_type_node)
    powfn = builtin_decl_explicit (BUILT_IN_POWF);
  else if (TYPE_MAIN_VARIANT (powtype) == long_double_type_node)
    powfn = builtin_decl_explicit (BUILT_IN_POWL);

  if (powfn == NULL_TREE)
    {
      if (tb1->ty == Tarray || tb1->ty == Tsarray)
	error ("Array operation %s not implemented", toChars());
      else
	error ("%s ^^ %s is not supported", e1->type->toChars(), e2->type->toChars());
      return error_mark_node;
    }

  e1_t = d_convert (powtype, e1->toElem (irs));
  e2_t = d_convert (powtype, e2->toElem (irs));

  return d_convert (type->toCtype(), d_build_call_nary (powfn, 2, e1_t, e2_t));
}

elem *
CatExp::toElem (IRState *irs)
{
  Type *etype;

  // One of the operands may be an element instead of an array.
  // Logic copied from CatExp::semantic
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();

  if (tb1->ty == Tarray || tb1->ty == Tsarray)
    etype = tb1->nextOf();
  else
    etype = tb2->nextOf();

  // Flatten multiple concatenations
  unsigned n_operands = 2;
  {
    Expression *e = e1;
    while (e->op == TOKcat)
      {
	e = ((CatExp *) e)->e1;
	n_operands += 1;
      }
  }

  unsigned n_args = (1 + (n_operands > 2 ? 1 : 0) + n_operands);

  tree *args = new tree[n_args];
  args[0] = build_typeinfo (type);

  if (n_operands > 2)
    args[1] = build_integer_cst (n_operands, Type::tuns32->toCtype());

  unsigned ai = n_args - 1;
  CatExp *ce = this;
  vec<tree, va_gc> *elem_vars = NULL;

  while (ce)
    {
      Expression *oe = ce->e2;
      while (1)
	{
	  tree array_exp;
	  if (d_types_compatible (oe->type->toBasetype(), etype->toBasetype()))
	    {
	      tree elem_var = NULL_TREE;
	      tree expr = maybe_temporary_var (oe->toElem (irs), &elem_var);
	      array_exp = d_array_value (oe->type->arrayOf()->toCtype(),
					 size_int (1), build_address (expr));

	      if (elem_var)
		vec_safe_push (elem_vars, elem_var);
	    }
	  else
	    array_exp = d_array_convert (oe);

	  args[ai--] = array_exp;

	  if (ce != NULL)
	    {
	      if (ce->e1->op != TOKcat)
		{
		  // Finish with atomtic lhs
		  oe = ce->e1;
		  ce = NULL;
		}
	      else
		{
		  // Continue with lhs CatExp
		  ce = (CatExp *) ce->e1;
		  break;
		}
	    }
	  else
	    goto all_done;
	}
    }
 all_done:

  tree result = build_libcall (n_operands > 2 ? LIBCALL_ARRAYCATNT : LIBCALL_ARRAYCATT,
			       n_args, args, type->toCtype());

  for (size_t i = 0; i < vec_safe_length (elem_vars); ++i)
    result = bind_expr ((*elem_vars)[i], result);

  return result;
}

elem *
MinExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  // %% faster: check if result is complex
  if ((e1->type->isreal() && e2->type->isimaginary())
      || (e1->type->isimaginary() && e2->type->isreal()))
    {
      // %%TODO: need to check size/modes
      tree t1 = e1->toElem (irs);
      tree t2 = e2->toElem (irs);

      t2 = build1 (NEGATE_EXPR, TREE_TYPE (t2), t2);

      if (e1->type->isreal())
	return build2 (COMPLEX_EXPR, type->toCtype(), t1, t2);
      else
	return build2 (COMPLEX_EXPR, type->toCtype(), t2, t1);
    }

  // The front end has already taken care of pointer-int and pointer-pointer
  return build_binary_op (MINUS_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
AddExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  // %% faster: check if result is complex
  if ((e1->type->isreal() && e2->type->isimaginary())
      || (e1->type->isimaginary() && e2->type->isreal()))
    {
      // %%TODO: need to check size/modes
      tree t1 = e1->toElem (irs);
      tree t2 = e2->toElem (irs);

      if (e1->type->isreal())
	return build2 (COMPLEX_EXPR, type->toCtype(), t1, t2);
      else
	return build2 (COMPLEX_EXPR, type->toCtype(), t2, t1);
    }

  // The front end has already taken care of (pointer + integer)
  return build_binary_op (PLUS_EXPR, type->toCtype(),
			  e1->toElem (irs), e2->toElem (irs));
}

elem *
BinExp::toElemBin (IRState *irs, int op)
{
  tree_code code = (tree_code) op;

  // Skip casts for lhs assignment.
  Expression *e1b = e1;
  while (e1b->op == TOKcast)
    {
      CastExp *ce = (CastExp *) e1b;
      gcc_assert (d_types_compatible (ce->type, ce->to));
      e1b = ce->e1;
    }

  // Prevent multiple evaluations of LHS, but watch out!
  // The LHS expression could be an assignment, to which
  // it's operation gets lost during gimplification.
  tree lexpr = NULL_TREE;
  tree lhs;

  if (e1b->op == TOKcomma)
    {
      CommaExp *ce = (CommaExp *) e1b;
      lexpr = ce->e1->toElem (irs);
      lhs = ce->e2->toElem (irs);
    }
  else
    lhs = e1b->toElem (irs);

  // Build assignment expression. Stabilize lhs for assignment.
  lhs = stabilize_reference (lhs);

  tree rhs = build_binary_op (code, e1->type->toCtype(),
			      convert_expr (lhs, e1b->type, e1->type), e2->toElem (irs));

  tree expr = modify_expr (lhs, convert_expr (rhs, e1->type, e1b->type));

  if (lexpr)
    expr = compound_expr (lexpr, expr);

  return expr;
}

elem *
XorAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, BIT_XOR_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
OrAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, BIT_IOR_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
AndAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, BIT_AND_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
UshrAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, UNSIGNED_RSHIFT_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
ShrAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, RSHIFT_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
ShlAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, LSHIFT_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
ModAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, e1->type->isfloating() ?
			FLOAT_MOD_EXPR : TRUNC_MOD_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
DivAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, e1->type->isintegral() ?
			TRUNC_DIV_EXPR : RDIV_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
MulAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, MULT_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
PowAssignExp::toElem (IRState *)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  gcc_unreachable();
}

// Determine if type is an array of structs that need a postblit.

static StructDeclaration *
needsPostblit (Type *t)
{
  t = t->baseElemOf();

  if (t->ty == Tstruct)
    {
      StructDeclaration *sd = ((TypeStruct *) t)->sym;
      if (sd->postblit)
	return sd;
    }

  return NULL;
}

elem *
CatAssignExp::toElem (IRState *irs)
{
  Type *tb1 = e1->type->toBasetype();
  Type *tb2 = e2->type->toBasetype();
  Type *etype = tb1->nextOf()->toBasetype();
  AddrOfExpr aoe;
  tree result;

  if (tb1->ty == Tarray && tb2->ty == Tdchar
      && (etype->ty == Tchar || etype->ty == Twchar))
    {
      // Append a dchar to a char[] or wchar[]
      tree args[2];
      LibCall libcall;

      args[0] = aoe.set (e1->toElem (irs));
      args[1] = e2->toElem (irs);
      libcall = etype->ty == Tchar ? LIBCALL_ARRAYAPPENDCD : LIBCALL_ARRAYAPPENDWD;

      result = build_libcall (libcall, 2, args, type->toCtype());
    }
  else
    {
      gcc_assert (tb1->ty == Tarray || tb2->ty == Tsarray);

      if ((tb2->ty == Tarray || tb2->ty == Tsarray)
	  && d_types_compatible (etype, tb2->nextOf()->toBasetype()))
	{
	  // Append an array
	  tree args[3];

	  args[0] = build_typeinfo (type);
	  args[1] = build_address (e1->toElem (irs));
	  args[2] = d_array_convert (e2);

	  result = build_libcall (LIBCALL_ARRAYAPPENDT, 3, args, type->toCtype());
	}
      else
	{
	  // Append an element
	  tree args[3];

	  args[0] = build_typeinfo (type);
	  args[1] = build_address (e1->toElem (irs));
	  args[2] = size_one_node;

	  result = build_libcall (LIBCALL_ARRAYAPPENDCTX, 3, args, type->toCtype());
	  result = make_temp (result);

	  // Assign e2 to last element
	  tree off_exp = d_array_length (result);
	  off_exp = build2 (MINUS_EXPR, TREE_TYPE (off_exp), off_exp, size_one_node);
	  off_exp = maybe_make_temp (off_exp);

	  tree ptr_exp = d_array_ptr (result);
	  ptr_exp = void_okay_p (ptr_exp);
	  ptr_exp = build_array_index (ptr_exp, off_exp);

	  // Evaluate expression before appending
	  tree e2e = e2->toElem (irs);
	  e2e = maybe_make_temp (e2e);
	  result = modify_expr (etype->toCtype(), build_deref (ptr_exp), e2e);
	  result = compound_expr (e2e, result);
	}
    }

  return aoe.finish (result);
}

elem *
MinAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, MINUS_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
AddAssignExp::toElem (IRState *irs)
{
  if (unhandled_arrayop_p (this))
    return error_mark_node;

  tree exp = toElemBin (irs, PLUS_EXPR);
  return convert_expr (exp, e1->type, type);
}

elem *
AssignExp::toElem (IRState *irs)
{
  // First, handle special assignment semantics

  // Look for array.length = n;
  if (e1->op == TOKarraylength)
    {
      // Assignment to an array's length property; resize the array.
      ArrayLengthExp *ale = (ArrayLengthExp *) e1;
      // Don't want ->toBasetype() for the element type.
      Type *etype = ale->e1->type->toBasetype()->nextOf();
      tree args[3];
      LibCall libcall;

      args[0] = build_typeinfo (ale->e1->type);
      args[1] = convert_expr (e2->toElem (irs), e2->type, Type::tsize_t);
      args[2] = build_address (ale->e1->toElem (irs));
      libcall = etype->isZeroInit() ? LIBCALL_ARRAYSETLENGTHT : LIBCALL_ARRAYSETLENGTHIT;

      tree result = build_libcall (libcall, 3, args);
      return d_array_length (result);
    }

  // Look for array[] = n;
  if (e1->op == TOKslice)
    {
      SliceExp *se = (SliceExp *) e1;
      Type *stype = se->e1->type->toBasetype();
      Type *tb2 = e2->type->toBasetype();
      Type *etype = stype->nextOf()->toBasetype();

      // Optimize static array assignment with array literal.
      // Front-end writes these as an assignment of a dynamic
      // array literal with a slice.
      if (se->lwr == NULL && stype->ty == Tsarray
	  && e2->op == TOKarrayliteral
	  && tb2->nextOf()->mutableOf()->implicitConvTo(stype->nextOf()))
	{
	  Expression *e1 = se->e1;
	  Type *t2save = e2->type;

	  // Treat [e2] as a static array literal.
	  e2->type = stype;
	  tree t1 = e1->toElem (irs);
	  tree t2 = convert_for_assignment (e2->toElem (irs), e2->type, e1->type);
	  tree result = modify_expr (e1->type->toCtype(), t1, t2);
	  e2->type = t2save;

	  return convert_expr (result, e1->type, type);
	}

      // Determine if we need to do postblit.
      int postblit = 0;

      if (needsPostblit (etype) != NULL
	  && ((e2->op != TOKslice && e2->isLvalue())
	      || (e2->op == TOKslice && ((UnaExp *) e2)->e1->isLvalue())
	      || (e2->op == TOKcast && ((UnaExp *) e2)->e1->isLvalue())))
	postblit = 1;

      if (d_types_compatible (etype, tb2))
	{
	  // Set a range of elements to one value.
	  tree t1 = maybe_make_temp (e1->toElem (irs));

	  if (op != TOKblit)
	    {
	      if (postblit)
		{
		  AddrOfExpr aoe;
		  tree args[4];
		  LibCall libcall;

		  args[0] = d_array_ptr (t1);
		  args[1] = aoe.set (e2->toElem (irs));
		  args[2] = d_array_length (t1);
		  args[3] = build_typeinfo (etype);
		  libcall = (op == TOKconstruct) ? LIBCALL_ARRAYSETCTOR : LIBCALL_ARRAYSETASSIGN;

		  tree call = build_libcall (libcall, 4, args);
		  return compound_expr (aoe.finish (call), t1);
		}
	    }

	  tree set_exp = irs->doArraySet (d_array_ptr (t1), e2->toElem (irs), d_array_length (t1));
	  return compound_expr (set_exp, t1);
	}

      if (op != TOKblit && postblit)
	{
	  // Generate _d_arrayassign() or _d_arrayctor()
	  tree args[3];
	  LibCall libcall;

	  args[0] = build_typeinfo (etype);
	  args[1] = d_array_convert (e1);
	  args[2] = d_array_convert (e2);
	  libcall = (op == TOKconstruct) ? LIBCALL_ARRAYCTOR : LIBCALL_ARRAYASSIGN;

	  return build_libcall (libcall, 3, args, type->toCtype());
	}

      if (array_bounds_check())
	{
	  tree args[3];

	  args[0] = build_integer_cst (etype->size(), Type::tsize_t->toCtype());
	  args[1] = d_array_convert (e2);
	  args[2] = d_array_convert (e1);

	  return build_libcall (LIBCALL_ARRAYCOPY, 3, args, type->toCtype());
	}
      else
	{
	  tree t1 = maybe_make_temp (d_array_convert (e1));
	  tree t2 = d_array_convert (e2);
	  tree size = fold_build2 (MULT_EXPR, size_type_node,
				   d_convert (size_type_node, d_array_length (t1)),
				   size_int (etype->size()));

	  tree result = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCPY), 3,
					   d_array_ptr (t1),
					   d_array_ptr (t2), size);
	  return compound_expr (result, t1);
	}
    }

  // Assignment that requires post construction.
  if (op == TOKconstruct)
    {
      tree lhs = e1->toElem (irs);
      tree rhs = convert_for_assignment (e2->toElem (irs), e2->type, e1->type);
      Type *tb1 = e1->type->toBasetype();

      if (e1->op == TOKvar)
	{
	  Declaration *decl = ((VarExp *) e1)->var;
	  // Look for reference initializations
	  if (decl->storage_class & (STCout | STCref))
	    {
	      // Want reference to lhs, not indirect ref.
	      lhs = TREE_OPERAND (lhs, 0);
	      rhs = build_address (rhs);
	    }
	}

      tree result = modify_expr (type->toCtype(), lhs, rhs);

      if (tb1->ty == Tstruct && e2->op == TOKint64)
	{
	  // Maybe set-up hidden pointer to outer scope context.
	  StructDeclaration *sd = ((TypeStruct *) tb1)->sym;

	  if (sd->isNested())
	    {
	      tree vthis_field = sd->vthis->toSymbol()->Stree;
	      tree vthis_value = build_vthis (sd, irs->func, this);

	      tree vthis_exp = modify_expr (component_ref (lhs, vthis_field), vthis_value);
	      result = compound_expr (result, vthis_exp);
	    }
	}

      return result;
    }

  // Simple assignment
  tree te1 = e1->toElem (irs);
  tree te2 = convert_for_assignment (e2->toElem (irs), e2->type, e1->type);
  tree saved_args = NULL_TREE;
  if (TREE_SIDE_EFFECTS (te1) && TREE_SIDE_EFFECTS (te2))
    {
      te1 = stabilize_reference (te1);
      saved_args = maybe_vcompound_expr (saved_args, te1);

      te2 = maybe_make_temp (te2);
      saved_args = maybe_vcompound_expr (saved_args, te2);
    }

  tree result = modify_expr (type->toCtype(), te1, te2);
  return maybe_compound_expr (saved_args, result);
}

elem *
PostExp::toElem (IRState *irs)
{
  tree result;

  if (op == TOKplusplus)
    {
      result = build2 (POSTINCREMENT_EXPR, type->toCtype(),
		       e1->toElem (irs), e2->toElem (irs));
    }
  else if (op == TOKminusminus)
    {
      result = build2 (POSTDECREMENT_EXPR, type->toCtype(),
		       e1->toElem (irs), e2->toElem (irs));
    }
  else
    gcc_unreachable();

  TREE_SIDE_EFFECTS (result) = 1;
  return result;
}

elem *
IndexExp::toElem (IRState *irs)
{
  Type *tb1 = e1->type->toBasetype();

  if (tb1->ty == Taarray)
    {
      Type *tkey = ((TypeAArray *) tb1)->index->toBasetype();
      AddrOfExpr aoe;
      tree args[4];
      LibCall libcall;
      tree index;

      if (modifiable)
	{
	  libcall = LIBCALL_AAGETX;
	  args[0] = build_address (e1->toElem (irs));
	}
      else
	{
	  libcall = LIBCALL_AAGETRVALUEX;
	  args[0] = e1->toElem (irs);
	}

      args[1] = build_typeinfo (tkey);
      args[2] = build_integer_cst (tb1->nextOf()->size(), Type::tsize_t->toCtype());
      args[3] = aoe.set (convert_expr (e2->toElem (irs), e2->type, tkey));

      index = aoe.finish (build_libcall (libcall, 4, args, type->pointerTo()->toCtype()));

      if (array_bounds_check() && !skipboundscheck)
	{
	  index = make_temp (index);
	  index = build3 (COND_EXPR, TREE_TYPE (index), d_truthvalue_conversion (index),
			  index, d_assert_call (loc, LIBCALL_ARRAY_BOUNDS));
	}

      return indirect_ref (type->toCtype(), index);
    }
  else
    {
      // Build an array index expression.  ArrayScope may build a BIND_EXPR
      // if temporaries were created for bounds checking.
      ArrayScope arrscope (lengthVar, loc);

      // The expression that holds the array data.
      tree t1 = e1->toElem (irs);
      // The expression that indexes the array data.
      tree t2 = e2->toElem (irs);
      // The base pointer to the elements.
      tree ptrexp;

      switch (tb1->ty)
	{
	case Tarray:
	case Tsarray:
	  t1 = arrscope.setArrayExp (t1, e1->type);

	  // If it's a static array and the index is constant,
	  // the front end has already checked the bounds.
	  if (array_bounds_check() && !(tb1->ty == Tsarray && e2->isConst()))
	    {
	      // Implement bounds check as a conditional expression:
	      // array [inbounds(index) ? index : { throw ArrayBoundsError}]
	      tree length;

	      // First, set up the index expression to only be evaluated once.
	      tree index = maybe_make_temp (t2);

	      if (tb1->ty == Tarray)
		{
		  t1 = maybe_make_temp (t1);
		  length = d_array_length (t1);
		}
	      else
		length = ((TypeSArray *) tb1)->dim->toElem (irs);

	      t2 = d_checked_index (loc, index, length, false);
	    }

	  if (tb1->ty == Tarray)
	    ptrexp = d_array_ptr (t1);
	  else
	    ptrexp = build_address (t1);

	  // This conversion is required for static arrays and is
	  // just-to-be-safe for dynamic arrays.
	  ptrexp = convert (tb1->nextOf()->pointerTo()->toCtype(), ptrexp);
	  break;

	case Tpointer:
	  // Ignores ArrayScope.
	  ptrexp = t1;
	  break;

	default:
	  gcc_unreachable();
	}

      ptrexp = void_okay_p (ptrexp);
      t2 = arrscope.finish (t2);

      return indirect_ref (TREE_TYPE (TREE_TYPE (ptrexp)),
			   build_array_index (ptrexp, t2));
    }
}

elem *
CommaExp::toElem (IRState *irs)
{
  tree t1 = e1->toElem (irs);
  tree t2 = e2->toElem (irs);
  tree tt = type ? type->toCtype() : void_type_node;

  return build2 (COMPOUND_EXPR, tt, t1, t2);
}

elem *
ArrayLengthExp::toElem (IRState *irs)
{
  if (e1->type->toBasetype()->ty == Tarray)
    return d_array_length (e1->toElem (irs));
  else
    {
      // Tsarray case seems to be handled by front-end
      error ("unexpected type for array length: %s", type->toChars());
      return error_mark_node;
    }
}

elem *
SliceExp::toElem (IRState *irs)
{
  Type *tb = type->toBasetype();
  gcc_assert (tb->ty == Tarray || tb->ty == Tsarray);

  // Use convert-to-dynamic-array code if possible
  if (e1->type->toBasetype()->ty == Tsarray && !upr && !lwr)
    return convert_expr (e1->toElem (irs), e1->type, type);

  Type *tb1 = e1->type->toBasetype();

  tree orig_array_expr, orig_pointer_expr;
  tree len_expr, ptr_expr;
  tree array_len_expr = NULL_TREE;
  tree lwr_tree = NULL_TREE;
  tree upr_tree = NULL_TREE;

  ArrayScope aryscp (lengthVar, loc);

  orig_array_expr = aryscp.setArrayExp (e1->toElem (irs), e1->type);
  orig_array_expr = maybe_make_temp (orig_array_expr);
  // specs don't say bounds if are checked for error or clipped to current size

  // Get the data pointer for static and dynamic arrays
  orig_pointer_expr = convert_expr (orig_array_expr, tb1,
				    tb1->nextOf()->pointerTo());

  ptr_expr = orig_pointer_expr;

  // orig_array_expr is already a SAVE_EXPR if necessary, so
  // we don't make array_len_expr a SAVE_EXPR which is, at most,
  // a COMPONENT_REF on top of orig_array_expr.
  if (tb1->ty == Tarray)
    array_len_expr = d_array_length (orig_array_expr);
  else if (tb1->ty == Tsarray)
    array_len_expr = ((TypeSArray *) tb1)->dim->toElem (irs);

  if (lwr)
    {
      lwr_tree = lwr->toElem (irs);

      if (!integer_zerop (lwr_tree))
	{
	  lwr_tree = maybe_make_temp (lwr_tree);
	  // Adjust .ptr offset
	  ptr_expr = build_array_index (void_okay_p (ptr_expr), lwr_tree);
	  ptr_expr = build_nop (TREE_TYPE (orig_pointer_expr), ptr_expr);
	}
      else
	lwr_tree = NULL_TREE;
    }

  if (upr)
    {
      upr_tree = upr->toElem (irs);
      upr_tree = maybe_make_temp (upr_tree);

      if (array_bounds_check())
	{
	  // %% && ! is zero
	  if (array_len_expr)
	    {
	      len_expr = d_checked_index (loc, upr_tree, array_len_expr, true);
	    }
	  else
	    {
	      // Still need to check bounds lwr <= upr for pointers.
	      gcc_assert (tb1->ty == Tpointer);
	      len_expr = upr_tree;
	    }
	  if (lwr_tree)
	    {
	      // Enforces lwr <= upr. No need to check lwr <= length as
	      // we've already ensured that upr <= length.
	      tree lwr_bounds_check = d_checked_index (loc, lwr_tree, upr_tree, true);
	      len_expr = compound_expr (lwr_bounds_check, len_expr);
	    }
	}
      else
	{
	  len_expr = upr_tree;
	}

      if (lwr_tree)
	{
	  // %% Need to ensure lwr always gets evaluated first, as it may be a function call.
	  // Does (-lwr + upr) rather than (upr - lwr)
	  len_expr = build2 (PLUS_EXPR, TREE_TYPE (len_expr),
			     build1 (NEGATE_EXPR, TREE_TYPE (lwr_tree), lwr_tree), len_expr);
	}
    }
  else
    {
      // If this is the case, than there is no lower bound specified and
      // there is no need to subtract.
      switch (tb1->ty)
	{
	case Tarray:
	  len_expr = d_array_length (orig_array_expr);
	  break;

	case Tsarray:
	  len_expr = ((TypeSArray *) tb1)->dim->toElem (irs);
	  break;

	default:
	  error ("Attempt to take length of something that was not an array");
	  return error_mark_node;
	}
    }

  tree exp;

  if (tb->ty == Tarray)
    exp = d_array_value (type->toCtype(), len_expr, ptr_expr);
  else
    {
      gcc_assert (lwr && tb->ty == Tsarray);
      exp = indirect_ref (type->toCtype(), ptr_expr);
    }

  return aryscp.finish (exp);
}

elem *
CastExp::toElem (IRState *irs)
{
  Type *ebtype = e1->type->toBasetype();
  Type *tbtype = to->toBasetype();
  tree t = e1->toElem (irs);

  // Just evaluate e1 if it has any side effects
  if (tbtype->ty == Tvoid)
    return build1 (NOP_EXPR, tbtype->toCtype(), t);

  return convert_expr (t, ebtype, tbtype);
}


elem *
DeleteExp::toElem (IRState *irs)
{
  LibCall libcall;
  tree t1 = e1->toElem (irs);
  Type *tb1 = e1->type->toBasetype();

  if (tb1->ty == Tclass)
    {
      if (e1->op == TOKvar)
        {
	  VarDeclaration *v = ((VarExp *) e1)->var->isVarDeclaration();
	  if (v && v->onstack)
	    {
	      libcall = tb1->isClassHandle()->isInterfaceDeclaration() ?
		LIBCALL_CALLINTERFACEFINALIZER : LIBCALL_CALLFINALIZER;
	      return build_libcall (libcall, 1, &t1);
	    }
	}
      libcall = tb1->isClassHandle()->isInterfaceDeclaration() ?
	LIBCALL_DELINTERFACE : LIBCALL_DELCLASS;

      t1 = build_address (t1);
      return build_libcall (libcall, 1, &t1);
    }
  else if (tb1->ty == Tarray)
    {
      // Might need to run destructor on array contents
      Type *telem = tb1->nextOf()->baseElemOf();
      tree ti = d_null_pointer;
      tree args[2];

      if (telem->ty == Tstruct)
	{
	  TypeStruct *ts = (TypeStruct *) telem;
	  if (ts->sym->dtor)
	    ti = tb1->nextOf()->getTypeInfo (NULL)->toElem (irs);
	}

      // call _delarray_t (&t1, ti);
      args[0] = build_address (t1);
      args[1] = ti;

      return build_libcall (LIBCALL_DELARRAYT, 2, args);
    }
  else if (tb1->ty == Tpointer)
    {
      t1 = build_address (t1);
      return build_libcall (LIBCALL_DELMEMORY, 1, &t1);
    }
  else
    {
      error ("don't know how to delete %s", e1->toChars());
      return error_mark_node;
    }
}

elem *
RemoveExp::toElem (IRState *irs)
{
  Expression *array = e1;
  Expression *index = e2;
  // Check that the array is actually an associative array
  if (array->type->toBasetype()->ty == Taarray)
    {
      Type *a_type = array->type->toBasetype();
      Type *tkey = ((TypeAArray *) a_type)->index->toBasetype();
      AddrOfExpr aoe;
      tree args[3];

      args[0] = array->toElem (irs);
      args[1] = build_typeinfo (tkey);
      args[2] = aoe.set (convert_expr (index->toElem (irs), index->type, tkey));

      return aoe.finish (build_libcall (LIBCALL_AADELX, 3, args));
    }
  else
    {
      error ("%s is not an associative array", array->toChars());
      return error_mark_node;
    }
}

elem *
BoolExp::toElem (IRState *irs)
{
  if (e1->op == TOKcall && e1->type->toBasetype()->ty == Tvoid)
    {
      /* This could happen as '&&' is allowed as a shorthand for 'if'
	 eg:  (condition) && callexpr();  */
      return e1->toElem (irs);
    }

  return convert (type->toCtype(), convert_for_condition (e1->toElem (irs), e1->type));
}

elem *
NotExp::toElem (IRState *irs)
{
  // %% doc: need to convert to boolean type or this will fail.
  tree t = build1 (TRUTH_NOT_EXPR, boolean_type_node,
		   convert_for_condition (e1->toElem (irs), e1->type));
  return d_convert (type->toCtype(), t);
}

elem *
ComExp::toElem (IRState *irs)
{
  return build1 (BIT_NOT_EXPR, type->toCtype(), e1->toElem (irs));
}

elem *
NegExp::toElem (IRState *irs)
{
  // %% GCC B.E. won't optimize (NEGATE_EXPR (INTEGER_CST ..))..
  // %% is type correct?
  TY ty1 = e1->type->toBasetype()->ty;

  if (ty1 == Tarray || ty1 == Tsarray)
    {
      error ("Array operation %s not implemented", toChars());
      return error_mark_node;
    }

  return build1 (NEGATE_EXPR, type->toCtype(), e1->toElem (irs));
}

elem *
PtrExp::toElem (IRState *irs)
{
  /* Produce better code by converting * (#rec + n) to
     COMPONENT_REFERENCE.  Otherwise, the variable will always be
     allocated in memory because its address is taken. */
  Type *rec_type = 0;
  size_t the_offset;
  tree rec_tree;

  if (e1->op == TOKadd)
    {
      BinExp *add_exp = (BinExp *) e1;
      if (add_exp->e1->op == TOKaddress
	  && add_exp->e2->isConst() && add_exp->e2->type->isintegral())
	{
	  Expression *rec_exp = ((AddrExp *) add_exp->e1)->e1;
	  rec_type = rec_exp->type->toBasetype();
	  rec_tree = rec_exp->toElem (irs);
	  the_offset = add_exp->e2->toUInteger();
	}
    }
  else if (e1->op == TOKsymoff)
    {
      SymOffExp *sym_exp = (SymOffExp *) e1;
      if (!decl_reference_p (sym_exp->var))
	{
	  rec_type = sym_exp->var->type->toBasetype();
	  rec_tree = get_decl_tree (sym_exp->var, irs->func);
	  the_offset = sym_exp->offset;
	}
    }

  if (rec_type && rec_type->ty == Tstruct)
    {
      StructDeclaration *sd = ((TypeStruct *) rec_type)->sym;
      for (size_t i = 0; i < sd->fields.dim; i++)
	{
	  VarDeclaration *field = sd->fields[i];
	  if (field->offset == the_offset
	      && d_types_same (field->type, this->type))
	    {
	      // Catch errors, backend will ICE otherwise.
	      if (error_operand_p (rec_tree))
		return rec_tree;

	      return component_ref (rec_tree, field->toSymbol()->Stree);
	    }
	  else if (field->offset > the_offset)
	    break;
	}
    }

  return indirect_ref (type->toCtype(), e1->toElem (irs));
}

elem *
AddrExp::toElem (IRState *irs)
{
  tree exp;

  if (e1->op == TOKstructliteral)
    {
      StructLiteralExp *sle = ((StructLiteralExp *) e1)->origin;
      exp = build_address (sle->toElem (irs));
    }
  else
    exp = build_address (e1->toElem (irs));

  return build_nop (type->toCtype(), exp);
}

elem *
CallExp::toElem (IRState *irs)
{
  Type *tb = e1->type->toBasetype();
  Expression *e1b = e1;
  tree object = NULL_TREE;

  // Calls to delegates can sometimes look like this:
  if (e1b->op == TOKcomma)
    {
      e1b = ((CommaExp *) e1b)->e2;
      gcc_assert (e1b->op == TOKvar);

      Declaration *var = ((VarExp *) e1b)->var;
      gcc_assert (var->isFuncDeclaration() && !var->needThis());
    }

  tree callee = e1b->toElem (irs);
  TypeFunction *tf = NULL;

  if (D_METHOD_CALL_EXPR (callee))
    {
      // This could be a delegate expression (TY == Tdelegate), but not
      // actually a delegate variable.
      if (e1b->op == TOKdotvar)
	{
	  // This gets the true function type, getting the function type from
	  // e1->type can sometimes be incorrect, eg: ref return functions.
	  tf = get_function_type (((DotVarExp *) e1b)->var->type);
	}
      else
	tf = get_function_type (tb);

      extract_from_method_call (callee, callee, object);
    }
  else if (tb->ty == Tdelegate)
    {
      // Delegate call, extract .object and .funcptr from var.
      callee = maybe_make_temp (callee);
      tf = get_function_type (tb);
      object = delegate_object (callee);
      callee = delegate_method (callee);
    }
  else if (e1b->op == TOKvar)
    {
      FuncDeclaration *fd = ((VarExp *) e1b)->var->isFuncDeclaration();
      gcc_assert (fd);
      tf = get_function_type (fd->type);

      if (fd->isNested())
	{
	  // Maybe re-evaluate symbol storage treating 'fd' as public.
	  if (call_by_alias_p (irs->func, fd))
	    setup_symbol_storage (fd, callee, true);

	  object = get_frame_for_symbol (irs->func, fd);
	}
      else if (fd->needThis())
	{
	  e1b->error ("need 'this' to access member %s", fd->toChars());
	  // Continue processing...
	  object = d_null_pointer;
	}
    }
  else
    {
      // Normal direct function call.
      tf = get_function_type (tb);
    }

  gcc_assert (tf != NULL);

  // Now we have the type, callee and maybe object reference,
  // build the call expression.
  tree exp = d_build_call (tf, callee, object, arguments);

  if (tf->isref)
    exp = build_deref (exp);

  // Some library calls are defined to return a generic type.
  // this->type is the real type we want to return.
  if (type->isTypeBasic())
    exp = d_convert (type->toCtype(), exp);

  return exp;
}

/*******************************************
 * Evaluate Expression, then call destructors on any temporaries in it.
 */

elem *
Expression::toElemDtor (IRState *irs)
{
  size_t starti = irs->varsInScope.length();
  tree exp = toElem (irs);
  size_t endi = irs->varsInScope.length();

  // Codegen can be improved by determining if no exceptions can be thrown
  // between the ctor and dtor, and eliminating the ctor and dtor.

  // Build an expression that calls destructors on all the variables going
  // going out of the scope starti .. endi.
  tree tdtors = NULL_TREE;
  for (size_t i = starti; i != endi; ++i)
    {
      VarDeclaration *vd = irs->varsInScope[i];
      if (vd)
	{
	  irs->varsInScope[i] = NULL;
	  tree td = vd->edtor->toElem (irs);
	  // Execute in reverse order.
	  tdtors = maybe_compound_expr (tdtors, td);
	}
    }

  if (tdtors != NULL_TREE)
    {
      if (op == TOKcall)
	{
	  // Wrap expression and dtors in a try/finally expression.
	  tree body = exp;

	  if (type->ty == Tvoid)
	    exp = build2 (TRY_FINALLY_EXPR, void_type_node, body, tdtors);
	  else
	    {
	      body = maybe_make_temp (body);
	      tree tfexp = build2 (TRY_FINALLY_EXPR, void_type_node, body, tdtors);
	      exp = compound_expr (tfexp, body);
	    }
	}
      else if (op == TOKcomma && ((CommaExp *) this)->e2->op == TOKvar)
	{
	  // Split comma expressions, so as don't require a save_expr.
	  tree lexp = TREE_OPERAND (exp, 0);
	  tree rvalue = TREE_OPERAND (exp, 1);
	  exp = compound_expr (compound_expr (lexp, tdtors), rvalue);
	}
      else
	{
	  exp = maybe_make_temp (exp);
	  exp = compound_expr (compound_expr (exp, tdtors), exp);
	}
    }

  return exp;
}


elem *
DotTypeExp::toElem (IRState *irs)
{
  // Just a pass through to e1.
  return e1->toElem (irs);
}

// The result will probably just be converted to a CONSTRUCTOR for a Tdelegate struct
elem *
DelegateExp::toElem (IRState *irs)
{
  Type *t = e1->type->toBasetype();

  if (func->fbody)
    {
      // Add the function as nested function if it belongs to this module
      // ie, it is a member of this module, or it is a template instance.
      Dsymbol *owner = func->toParent();
      while (!owner->isTemplateInstance() && owner->toParent())
	owner = owner->toParent();
      if (owner->isTemplateInstance() || owner == irs->mod)
	irs->func->deferred.push (func);
    }

  if (t->ty == Tclass || t->ty == Tstruct)
    {
      if (!func->isThis())
	{
	  error ("delegates are only for non-static functions");
	  return error_mark_node;
	}

      return get_object_method (e1->toElem (irs), e1, func, type);
    }
  else
    {
      tree this_tree;
      if (func->isNested())
	{
	  if (e1->op == TOKnull)
	    this_tree = e1->toElem (irs);
	  else
	    this_tree = get_frame_for_symbol (irs->func, func);
	}
      else
	{
	  gcc_assert (func->isThis());
	  this_tree = e1->toElem (irs);
	}

      return build_method_call (build_address (func->toSymbol()->Stree),
				this_tree, type);
    }
}

elem *
DotVarExp::toElem (IRState *irs)
{
  FuncDeclaration *func_decl;
  VarDeclaration *var_decl;
  Type *obj_basetype = e1->type->toBasetype();

  switch (obj_basetype->ty)
    {
    case Tpointer:
      if (obj_basetype->nextOf()->toBasetype()->ty != Tstruct)
	break;
      // drop through

    case Tstruct:
    case Tclass:
      func_decl = var->isFuncDeclaration();
      var_decl = var->isVarDeclaration();
      if (func_decl)
      {
	// if Tstruct, objInstanceMethod will use the address of e1
	if (func_decl->isThis())
	  return get_object_method (e1->toElem (irs), e1, func_decl, type);
	else
	  {
	    // Static method; ignore the object instance
	    return build_address (func_decl->toSymbol()->Stree);
    	  }
      }
      else if (var_decl)
	{
	  if (!var_decl->isField())
	    return get_decl_tree (var_decl, irs->func);
	  else
	    {
	      tree this_tree = e1->toElem (irs);
	      if (obj_basetype->ty != Tstruct)
		this_tree = build_deref (this_tree);
	      return component_ref (this_tree, var_decl->toSymbol()->Stree);
	    }
	}
      else
	error ("%s is not a field, but a %s", var->toChars(), var->kind());
      break;

    default:
      break;
    }

  error ("Don't know how to handle %s", toChars());
  return error_mark_node;
}

elem *
AssertExp::toElem (IRState *irs)
{
  if (global.params.useAssert)
    {
      Type *tb1 = e1->type->toBasetype();
      tree tmsg = NULL_TREE;
      LibCall libcall;

      // Build _d_assert call.
      if (irs->func->isUnitTestDeclaration())
	{
	  if (msg)
	    {
	      tmsg = msg->toElemDtor (irs);
	      libcall = LIBCALL_UNITTEST_MSG;
	    }
	  else
	    libcall = LIBCALL_UNITTEST;
	}
      else
	{
	  if (msg)
	    {
	      tmsg = msg->toElemDtor (irs);
	      libcall = LIBCALL_ASSERT_MSG;
	    }
	  else
	    libcall = LIBCALL_ASSERT;
	}

      tree assert_call = d_assert_call (loc, libcall, tmsg);

      // Build condition that we are asserting in this contract.
      if (tb1->ty == Tclass)
	{
	  ClassDeclaration *cd = tb1->isClassHandle();
	  tree arg = e1->toElem (irs);
	  tree invc = NULL_TREE;

	  if (cd->isCOMclass())
	    {
	      return build3 (COND_EXPR, void_type_node,
			     build_boolop (NE_EXPR, arg, d_null_pointer),
			     d_void_zero_node, assert_call);
	    }
	  else if (cd->isInterfaceDeclaration())
	    arg = convert_expr (arg, tb1, build_object_type());

	  if (global.params.useInvariants && !cd->isCPPclass())
	    invc = build_libcall (LIBCALL_INVARIANT, 1, &arg);

	  // This does a null pointer check before calling _d_invariant
	  return build3 (COND_EXPR, void_type_node,
			 build_boolop (NE_EXPR, arg, d_null_pointer),
			 invc ? invc : d_void_zero_node, assert_call);
	}
      else
	{
	  // build: ((bool) e1  ? (void)0 : _d_assert (...))
	  //    or: (e1 != null ? e1._invariant() : _d_assert (...))
	  tree result;
	  tree invc = NULL_TREE;
	  tree e1_t = e1->toElem (irs);

	  if (global.params.useInvariants
	      && tb1->ty == Tpointer && tb1->nextOf()->ty == Tstruct)
	    {
	      FuncDeclaration *inv = ((TypeStruct *) tb1->nextOf())->sym->inv;
	      if (inv != NULL)
		{
		  Expressions args;
		  e1_t = maybe_make_temp (e1_t);
		  invc = d_build_call (inv, e1_t, &args);
		}
	    }
	  result = build3 (COND_EXPR, void_type_node,
			   convert_for_condition (e1_t, e1->type),
			   invc ? invc : d_void_zero_node, assert_call);
	  return result;
	}
    }

  return d_void_zero_node;
}

elem *
DeclarationExp::toElem (IRState *irs)
{
  VarDeclaration *vd = declaration->isVarDeclaration();

  if (vd != NULL)
    {
      if (!vd->isStatic() && !(vd->storage_class & STCmanifest)
	  && !(vd->storage_class & (STCextern | STCtls | STCgshared)))
	{
	  // Put variable on list of things needing destruction
	  if (vd->edtor && !vd->noscope)
	    irs->varsInScope.safe_push (vd);
	}
    }

  irs->pushStatementList();
  declaration->toObjFile (0);
  tree t = irs->popStatementList();

  /* Construction of an array for typesafe-variadic function arguments
     can cause an empty STMT_LIST here.  This can causes problems
     during gimplification. */
  if (TREE_CODE (t) == STATEMENT_LIST && !STATEMENT_LIST_HEAD (t))
    return build_empty_stmt (input_location);

  return t;
}


elem *
FuncExp::toElem (IRState *irs)
{
  Type *ftype = type->toBasetype();

  // This check is for lambda's, remove 'vthis' as function isn't nested.
  if (fd->tok == TOKreserved && ftype->ty == Tpointer)
    {
      fd->tok = TOKfunction;
      fd->vthis = NULL;
    }

  // Emit after current function body has finished.
  if (irs->func)
    irs->func->deferred.push (fd);

  // If nested, this will be a trampoline...
  if (fd->isNested())
    {
      return build_method_call (build_address (fd->toSymbol()->Stree),
				get_frame_for_symbol (irs->func, fd), type);
    }

  return build_nop (type->toCtype(), build_address (fd->toSymbol()->Stree));
}

elem *
HaltExp::toElem (IRState *)
{
  // Needs improvement.  Avoid library calls if possible..
  return d_build_call_nary (builtin_decl_explicit (BUILT_IN_ABORT), 0);
}

elem *
SymbolExp::toElem (IRState *irs)
{
  tree exp;

  if (op == TOKvar)
    {
      if (var->needThis())
	{
	  error ("need 'this' to access member %s", var->ident->string);
	  return error_mark_node;
	}

      // __ctfe is always false at runtime
      if (var->ident == Id::ctfe)
	return integer_zero_node;

      exp = get_decl_tree (var, irs->func);
      TREE_USED (exp) = 1;

      // For variables that are references (currently only out/inout arguments;
      // objects don't count), evaluating the variable means we want what it refers to.
      if (decl_reference_p (var))
	exp = indirect_ref (var->type->toCtype(), exp);

      return exp;
    }
  else if (op == TOKsymoff)
    {
      size_t offset = ((SymOffExp *) this)->offset;

      exp = get_decl_tree (var, irs->func);
      TREE_USED (exp) = 1;

      if (decl_reference_p (var))
	gcc_assert (POINTER_TYPE_P (TREE_TYPE (exp)));
      else
	exp = build_address (exp);

      if (!offset)
	return d_convert (type->toCtype(), exp);

      tree b = build_integer_cst (offset, Type::tsize_t->toCtype());
      return build_nop (type->toCtype(), build_offset (exp, b));
    }

  gcc_assert (op == TOKvar || op == TOKsymoff);
  return error_mark_node;
}

elem *
NewExp::toElem (IRState *irs)
{
  Type *tb = type->toBasetype();
  LibCall libcall;
  tree result;

  if (allocator)
    gcc_assert (newargs);

  // New'ing a class.
  if (tb->ty == Tclass)
    {
      tb = newtype->toBasetype();
      gcc_assert (tb->ty == Tclass);
      TypeClass *class_type = (TypeClass *) tb;
      ClassDeclaration *class_decl = class_type->sym;

      tree new_call;
      tree setup_exp = NULL_TREE;
      // type->toCtype() is a REFERENCE_TYPE; we want the RECORD_TYPE
      tree rec_type = TREE_TYPE (class_type->toCtype());

      // Call allocator (custom allocator or _d_newclass).
      if (onstack)
	{
	  tree stack_var = build_local_temp (rec_type);
	  expand_decl (stack_var);
	  new_call = build_address (stack_var);
	  setup_exp = modify_expr (indirect_ref (rec_type, new_call),
				   class_decl->toInitializer()->Stree);
	}
      else if (allocator)
	{
	  new_call = d_build_call (allocator, NULL_TREE, newargs);
	  new_call = maybe_make_temp (new_call);
	  // copy memory...
	  setup_exp = modify_expr (indirect_ref (rec_type, new_call),
				   class_decl->toInitializer()->Stree);
	}
      else
	{
	  tree arg = build_address (class_decl->toSymbol()->Stree);
	  new_call = build_libcall (LIBCALL_NEWCLASS, 1, &arg);
	}
      new_call = build_nop (tb->toCtype(), new_call);

      // Set vthis for nested classes.
      if (class_decl->isNested())
	{
	  tree vthis_value = NULL_TREE;
	  tree vthis_field = class_decl->vthis->toSymbol()->Stree;
	  if (thisexp)
	    {
	      ClassDeclaration *thisexp_cd = thisexp->type->isClassHandle();
	      Dsymbol *outer = class_decl->toParent2();
	      int offset = 0;

	      vthis_value = thisexp->toElem (irs);
	      if (outer != thisexp_cd)
		{
		  ClassDeclaration *outer_cd = outer->isClassDeclaration();
		  gcc_assert (outer_cd->isBaseOf (thisexp_cd, &offset));
		  // could just add offset
		  vthis_value = convert_expr (vthis_value, thisexp->type, outer_cd->type);
		}
	    }
	  else
	    {
	      vthis_value = build_vthis (class_decl, irs->func, this);
	    }

	  if (vthis_value)
	    {
	      new_call = maybe_make_temp (new_call);
	      vthis_field = component_ref (indirect_ref (rec_type, new_call), vthis_field);
	      setup_exp = maybe_compound_expr (setup_exp, modify_expr (vthis_field, vthis_value));
	    }
	}
      new_call = maybe_compound_expr (setup_exp, new_call);

      // Call constructor.
      if (member)
	result = d_build_call (member, new_call, arguments);
      else
	result = new_call;
    }
  // New'ing a struct.
  else if (tb->ty == Tpointer && tb->nextOf()->toBasetype()->ty == Tstruct)
    {
      Type *htype = newtype->toBasetype();
      gcc_assert (htype->ty == Tstruct);
      gcc_assert (!onstack);

      TypeStruct *stype = (TypeStruct *) htype;
      StructDeclaration *sd = stype->sym;
      Expression *init = stype->defaultInit (loc);
      tree new_call;

      // Struct size is unknown.
      if (sd->size (loc) == 0)
	return d_convert (type->toCtype(), integer_zero_node);

      if (allocator)
	new_call = d_build_call (allocator, NULL_TREE, newargs);
      else
	{
	  libcall = stype->isZeroInit (loc) ? LIBCALL_NEWITEMT : LIBCALL_NEWITEMIT;
	  tree arg = type->getTypeInfo(NULL)->toElem (irs);
	  new_call = build_libcall (libcall, 1, &arg);
	}
      new_call = build_nop (tb->toCtype(), new_call);

      // Save the result allocation call.
      tree init_exp = convert_for_assignment (init->toElem (irs), init->type, stype);
      new_call = maybe_make_temp (new_call);

      tree setup_exp = modify_expr (build_deref (new_call), init_exp);
      new_call = compound_expr (setup_exp, new_call);

      // Set vthis for nested structs/classes.
      if (sd->isNested())
	{
	  tree vthis_value = build_vthis (sd, irs->func, this);
	  tree vthis_field;
	  new_call = maybe_make_temp (new_call);
	  vthis_field = component_ref (indirect_ref (stype->toCtype(), new_call),
				       sd->vthis->toSymbol()->Stree);
	  new_call = compound_expr (modify_expr (vthis_field, vthis_value), new_call);
	}

      // Call constructor.
      if (member)
	result = d_build_call (member, new_call, arguments);
      else
	result = new_call;
    }
  // New'ing a D array.
  else if (tb->ty == Tarray)
    {
      tb = newtype->toBasetype();
      gcc_assert (tb->ty == Tarray);
      TypeDArray *tarray = (TypeDArray *) tb;
      gcc_assert (!allocator);
      gcc_assert (arguments && arguments->dim >= 1);

      if (arguments->dim == 1)
	{
	  // Single dimension array allocations.
	  Expression *arg = (*arguments)[0];
	  tree args[2];

	  // Elem size is unknown.
	  if (tarray->next->size() == 0)
	    return d_array_value (type->toCtype(), size_int (0), d_null_pointer);

	  libcall = tarray->next->isZeroInit() ? LIBCALL_NEWARRAYT : LIBCALL_NEWARRAYIT;
	  args[0] = type->getTypeInfo(NULL)->toElem (irs);
	  args[1] = arg->toElem (irs);
	  result = build_libcall (libcall, 2, args, tb->toCtype());
	}
      else
	{
	  // Multidimensional array allocations.
	  vec<constructor_elt, va_gc> *elms = NULL;
	  Type *telem = newtype->toBasetype();
	  tree dims_var = create_temporary_var (d_array_type (Type::tsize_t, arguments->dim));
	  tree dims_init = build_constructor (TREE_TYPE (dims_var), NULL);
	  tree args[3];

	  for (size_t i = 0; i < arguments->dim; i++)
	    {
	      Expression *arg = (*arguments)[i];
	      tree index = build_integer_cst (i, size_type_node);
	      CONSTRUCTOR_APPEND_ELT (elms, index, arg->toElem (irs));

	      gcc_assert (telem->ty == Tarray);
	      telem = telem->toBasetype()->nextOf();
	      gcc_assert (telem);
	    }

	  CONSTRUCTOR_ELTS (dims_init) = elms;
	  DECL_INITIAL (dims_var) = dims_init;

	  libcall = telem->isZeroInit() ? LIBCALL_NEWARRAYMTX : LIBCALL_NEWARRAYMITX;
	  args[0] = type->getTypeInfo(NULL)->toElem (irs);
	  args[1] = build_integer_cst (arguments->dim, Type::tint32->toCtype());
	  args[2] = build_address (dims_var);
	  result = build_libcall (libcall, 3, args, tb->toCtype());
	  result = bind_expr (dims_var, result);
	}
    }
  // New'ing a pointer
  else if (tb->ty == Tpointer)
    {
      TypePointer *tpointer = (TypePointer *) tb;

      // Elem size is unknown.
      if (tpointer->next->size() == 0)
	return d_convert (type->toCtype(), integer_zero_node);

      libcall = tpointer->next->isZeroInit (loc) ? LIBCALL_NEWITEMT : LIBCALL_NEWITEMIT;

      tree arg = type->getTypeInfo(NULL)->toElem (irs);
      result = build_libcall (libcall, 1, &arg, tb->toCtype());
    }
  else
    gcc_unreachable();

  return convert_expr (result, tb, type);
}

elem *
ScopeExp::toElem (IRState *)
{
  error ("%s is not an expression", toChars());
  return error_mark_node;
}

elem *
TypeExp::toElem (IRState *)
{
  error ("type %s is not an expression", toChars());
  return error_mark_node;
}

elem *
RealExp::toElem (IRState *)
{
  return build_float_cst (value, type->toBasetype());
}

elem *
IntegerExp::toElem (IRState *)
{
  tree ctype = type->toBasetype()->toCtype();
  return build_integer_cst (value, ctype);
}

elem *
ComplexExp::toElem (IRState *)
{
  TypeBasic *compon_type;
  switch (type->toBasetype()->ty)
    {
    case Tcomplex32:
      compon_type = (TypeBasic *) Type::tfloat32;
      break;

    case Tcomplex64:
      compon_type = (TypeBasic *) Type::tfloat64;
      break;

    case Tcomplex80:
      compon_type = (TypeBasic *) Type::tfloat80;
      break;

    default:
      gcc_unreachable();
    }

  return build_complex (type->toCtype(),
			build_float_cst (creall (value), compon_type),
			build_float_cst (cimagl (value), compon_type));
}

elem *
StringExp::toElem (IRState *)
{
  Type *tb = type->toBasetype();
  // Assuming this->string is null terminated
  dinteger_t dim = len + (tb->ty == Tpointer);

  tree value = build_string (dim * sz, (char *) string);

  // Array type doesn't match string length if null terminated.
  TREE_TYPE (value) = d_array_type (tb->nextOf(), len);
  TREE_CONSTANT (value) = 1;
  TREE_READONLY (value) = 1;

  switch (tb->ty)
    {
    case Tarray:
      value = d_array_value (type->toCtype(), size_int (len), build_address (value));
      break;

    case Tpointer:
      value = build_address (value);
      break;

    case Tsarray:
      TREE_TYPE (value) = type->toCtype();
      break;

    default:
      error ("Invalid type for string constant: %s", type->toChars());
      return error_mark_node;
    }

  return value;
}

elem *
TupleExp::toElem (IRState *irs)
{
  tree exp = NULL_TREE;

  if (e0)
    exp = e0->toElem (irs);

  for (size_t i = 0; i < exps->dim; ++i)
    {
      Expression *e = (*exps)[i];
      exp = maybe_vcompound_expr (exp, e->toElem (irs));
    }

  if (exp == NULL_TREE)
    exp = d_void_zero_node;

  return exp;
}

elem *
ArrayLiteralExp::toElem (IRState *irs)
{
  Type *tb = type->toBasetype();
  gcc_assert (tb->ty == Tarray || tb->ty == Tsarray || tb->ty == Tpointer);

  // Convert void[n] to ubyte[n]
  if (tb->ty == Tsarray && tb->nextOf()->toBasetype()->ty == Tvoid)
    tb = TypeSArray::makeType(loc, Type::tuns8, ((TypeSArray *)tb)->dim->toUInteger());

  Type *etype = tb->nextOf();
  tree tsa = d_array_type (etype, elements->dim);
  tree result = NULL_TREE;
  bool constant_p = tb->isImmutable();

  // Handle empty array literals.
  if (elements->dim == 0)
    {
      if (tb->ty == Tarray)
	return d_array_value (type->toCtype(), size_int (0), d_null_pointer);
      else
	return build_constructor (tsa, NULL);
    }

  // Build an expression that assigns the expressions in ELEMENTS to a constructor.
  vec<constructor_elt, va_gc> *elms = NULL;
  vec_safe_reserve (elms, elements->dim);

  for (size_t i = 0; i < elements->dim; i++)
    {
      Expression *e = (*elements)[i];
      tree elem = e->toElem (irs);

      elem = maybe_make_temp (elem);
      CONSTRUCTOR_APPEND_ELT (elms, build_integer_cst (i, size_type_node),
			      convert_expr (elem, e->type, etype));

      if (constant_p && !e->isConst())
	constant_p = false;
    }

  tree ctor = build_constructor (tsa, elms);
  tree args[2];

  // Nothing else to do for static arrays.
  if (tb->ty == Tsarray)
    return d_convert (type->toCtype(), ctor);

  // Don't allocate immutable arrays on the heap.
  if (tb->ty == Tarray && constant_p)
    return d_array_value (type->toCtype(), size_int (elements->dim), build_address (ctor));

  args[0] = build_typeinfo (etype->arrayOf());
  args[1] = build_integer_cst (elements->dim, size_type_node);

  // Call _d_arrayliteralTX (ti, dim);
  tree mem = build_libcall (LIBCALL_ARRAYLITERALTX, 2, args, etype->pointerTo()->toCtype());
  mem = maybe_make_temp (mem);

  // memcpy (mem, &ctor, size)
  tree size = fold_build2 (MULT_EXPR, size_type_node,
			   size_int (elements->dim), size_int (tb->nextOf()->size()));

  result = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMCPY), 3,
			      mem, build_address (ctor), size);

  // Returns array pointed to by MEM.
  result = maybe_compound_expr (result, mem);

  if (tb->ty == Tarray)
    result = d_array_value (type->toCtype(), size_int (elements->dim), result);

  return result;
}

elem *
AssocArrayLiteralExp::toElem (IRState *irs)
{
  Type *tb = type->toBasetype();
  // %% want mutable type for typeinfo reference.
  tb = tb->mutableOf();

  TypeAArray *aa_type;

  if (tb->ty == Taarray)
    aa_type = (TypeAArray *) tb;
  else
    {
      // It's the AssociativeArray type.
      // Turn it back into a TypeAArray
      aa_type = new TypeAArray ((*values)[0]->type, (*keys)[0]->type);
      aa_type->semantic (loc, NULL);
    }

  Type *index = aa_type->index;
  Type *next = aa_type->next;
  gcc_assert (keys != NULL);
  gcc_assert (values != NULL);

  tree keys_var = create_temporary_var (d_array_type (index, keys->dim));
  tree vals_var = create_temporary_var (d_array_type (next, keys->dim));
  tree keys_ptr = build_nop (index->pointerTo()->toCtype(),
 			     build_address (keys_var));
  tree vals_ptr = build_nop (next->pointerTo()->toCtype(),
			     build_address (vals_var));
  tree keys_offset = size_zero_node;
  tree vals_offset = size_zero_node;
  tree keys_size = size_int (index->size());
  tree vals_size = size_int (next->size());
  tree result = NULL_TREE;

  for (size_t i = 0; i < keys->dim; i++)
    {
      Expression *e;
      tree elemp_e, assgn_e;

      e = (*keys)[i];
      elemp_e = build_offset (keys_ptr, keys_offset);
      assgn_e = vmodify_expr (build_deref (elemp_e), e->toElem (irs));
      keys_offset = size_binop (PLUS_EXPR, keys_offset, keys_size);
      result = maybe_compound_expr (result, assgn_e);

      e = (*values)[i];
      elemp_e = build_offset (vals_ptr, vals_offset);
      assgn_e = vmodify_expr (build_deref (elemp_e), e->toElem (irs));
      vals_offset = size_binop (PLUS_EXPR, vals_offset, vals_size);
      result = maybe_compound_expr (result, assgn_e);
    }

  tree args[3];

  args[0] = build_typeinfo (aa_type);
  args[1] = d_array_value (index->arrayOf()->toCtype(), size_int (keys->dim), keys_ptr);
  args[2] = d_array_value (next->arrayOf()->toCtype(), size_int (keys->dim), vals_ptr);
  result = maybe_compound_expr (result, build_libcall (LIBCALL_ASSOCARRAYLITERALTX, 3, args));

  tree aat_type = aa_type->toCtype();
  vec<constructor_elt, va_gc> *ce = NULL;
  CONSTRUCTOR_APPEND_ELT (ce, TYPE_FIELDS (aat_type), result);
  tree ctor = build_constructor (aat_type, ce);

  result = bind_expr (keys_var, bind_expr (vals_var, ctor));
  return build_nop (type->toCtype(), result);
}

elem *
StructLiteralExp::toElem (IRState *irs)
{
  vec<constructor_elt, va_gc> *ce = NULL;
  Type *tb = type->toBasetype();

  gcc_assert (tb->ty == Tstruct);

  if (sinit && sinit->Stree)
    return sinit->Stree;

  // CTFE may fill the hidden pointer by NullExp.
  size_t dim = elements ? elements->dim : 0;
  gcc_assert (dim <= sd->fields.dim);

  for (size_t i = 0; i < dim; i++)
    {
      if (!(*elements)[i])
	continue;

      Expression *exp = (*elements)[i];
      Type *exp_type = exp->type->toBasetype();
      tree exp_tree = NULL_TREE;

      VarDeclaration *fld = sd->fields[i];
      Type *fld_type = fld->type->toBasetype();

      if (fld_type->ty == Tsarray)
	{
	  if (d_types_compatible (exp_type, fld_type))
	    {
	      // %% This would call _d_newarrayT ... use memcpy?
	      exp_tree = convert_expr (exp->toElem (irs), exp->type, fld->type);
	    }
	  else
	    {
	      // %% Could use memset if is zero init...
	      exp_tree = build_local_temp (fld_type->toCtype());
	      Type *etype = fld_type;

	      while (etype->ty == Tsarray)
		etype = etype->nextOf();

	      gcc_assert (fld_type->size() % etype->size() == 0);
	      tree size = fold_build2 (TRUNC_DIV_EXPR, size_type_node,
				       size_int (fld_type->size()), size_int (etype->size()));

	      tree ptr_tree = build_nop (etype->pointerTo()->toCtype(),
					 build_address (exp_tree));
	      tree set_exp = irs->doArraySet (ptr_tree, exp->toElem (irs), size);
	      exp_tree = compound_expr (set_exp, exp_tree);
	    }
	}
      else
	exp_tree = convert_expr (exp->toElem (irs), exp->type, fld->type);

      CONSTRUCTOR_APPEND_ELT (ce, fld->toSymbol()->Stree, exp_tree);

      // Unions only have one field that gets assigned.
      if (sd->isUnionDeclaration())
	break;
    }

  if (sd->isNested() && dim != sd->fields.dim)
    {
      // Maybe setup hidden pointer to outer scope context.
      tree vthis_field = sd->vthis->toSymbol()->Stree;
      tree vthis_value = build_vthis (sd, irs->func, this);
      CONSTRUCTOR_APPEND_ELT (ce, vthis_field, vthis_value);
      gcc_assert (sinit == NULL);
    }

  tree ctor = build_constructor (type->toCtype(), ce);
  tree var = build_local_temp (TREE_TYPE (ctor));
  tree init = NULL_TREE;

  if (fillHoles)
    {
      // Initialize all alignment 'holes' to zero.
      init = d_build_call_nary (builtin_decl_explicit (BUILT_IN_MEMSET), 3,
				build_address (var), size_zero_node,
				size_int (sd->structsize));
    }

  init = maybe_compound_expr (init, modify_expr (var, ctor));

  return compound_expr (init, var);
}

elem *
NullExp::toElem (IRState *)
{
  TY base_ty = type->toBasetype()->ty;
  tree null_exp;
  vec<constructor_elt, va_gc> *ce = NULL;

  // 0 -> dynamic array.  This is a special case conversion.
  // Move to convert for convertTo if it shows up elsewhere.
  switch (base_ty)
    {
    case Tarray:
	  null_exp = d_array_value (type->toCtype(), size_int (0), d_null_pointer);
	  break;

    case Taarray:
	{
	  tree ttype = type->toCtype();
	  tree field = TYPE_FIELDS (ttype);
	  tree value = d_convert (TREE_TYPE (field), integer_zero_node);

	  CONSTRUCTOR_APPEND_ELT (ce, field, value);
	  null_exp = build_constructor (ttype, ce);
	  break;
	}

    case Tdelegate:
	  null_exp = build_delegate_cst (d_null_pointer, d_null_pointer, type);
	  break;

    default:
	  null_exp = d_convert (type->toCtype(), integer_zero_node);
	  break;
    }

  return null_exp;
}

elem *
ThisExp::toElem (IRState *irs)
{
  tree this_tree = NULL_TREE;
  FuncDeclaration *fd = irs->func;

  if (var)
    {
      gcc_assert(var->isVarDeclaration());
      this_tree = get_decl_tree (var, fd);
    }
  else
    {
      gcc_assert (fd && fd->vthis);
      this_tree = get_decl_tree (fd->vthis, fd);
    }

  if (type->ty == Tstruct)
    this_tree = build_deref (this_tree);

  return this_tree;
}

elem *
VectorExp::toElem (IRState *irs)
{
  tree vectype = type->toCtype();
  tree elemtype = TREE_TYPE (vectype);

  // First handle array literal expressions.
  if (e1->op == TOKarrayliteral)
    {
      Expressions *elements = ((ArrayLiteralExp *) e1)->elements;
      vec<constructor_elt, va_gc> *elms = NULL;
      bool constant_p = true;

      vec_safe_reserve (elms, elements->dim);
      for (size_t i = 0; i < elements->dim; i++)
	{
	  Expression *e = (*elements)[i];
	  tree value = d_convert (elemtype, e->toElem (irs));
	  if (!CONSTANT_CLASS_P (value))
	    constant_p = false;

	  CONSTRUCTOR_APPEND_ELT (elms, size_int (i), value);
	}

      // Build a VECTOR_CST from a constant vector constructor.
      if (constant_p)
	return build_vector_from_ctor (vectype, elms);

      return build_constructor (vectype, elms);
    }
  else
    {
      // Build constructor from single value.
      tree val = d_convert (elemtype, e1->toElem (irs));

      return build_vector_from_val (vectype, val);
    }
}

elem *
ClassReferenceExp::toElem (IRState *)
{
  // ClassReferenceExp builds the RECORD_TYPE,
  // we want to return a reference to it.
  tree exp = toSymbol()->Stree;

  return build_address (exp);
}

