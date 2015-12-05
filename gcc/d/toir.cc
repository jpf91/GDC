// toir.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/enum.h"
#include "dfrontend/module.h"
#include "dfrontend/init.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/expression.h"
#include "dfrontend/statement.h"
#include "dfrontend/visitor.h"

#include "tree.h"
#include "tree-iterator.h"
#include "options.h"
#include "stmt.h"
#include "fold-const.h"
#include "diagnostic.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "id.h"


// Implements the visitor interface to build the GCC trees of all Statement
// AST classes emitted from the D Front-end.
// All visit methods accept one parameter S, which holds the frontend AST
// of the statement to compile.  They also don't return any value, instead
// generated code are pushed to add_stmt(), which appends them to the
// statement list in the current_binding_level.

class IRVisitor : public Visitor
{
  FuncDeclaration *func_;

  // Stack of labels which are targets for "break" and "continue",
  // linked through TREE_CHAIN.
  tree break_label_;
  tree continue_label_;

public:
  IRVisitor(FuncDeclaration *fd)
  {
    this->func_ = fd;
    this->break_label_ = NULL_TREE;
    this->continue_label_ = NULL_TREE;
  }

  // Start a new scope for a KIND statement.
  // Each user-declared variable will have a binding contour that begins
  // where the variable is declared and ends at it's containing scope.
  void start_scope(level_kind kind)
  {
    push_binding_level(kind);
    push_stmt_list();
  }

  // Leave scope pushed by start_scope, returning a new bind_expr if
  // any variables where declared in the scope.
  tree end_scope()
  {
    tree block = pop_binding_level();
    tree body = pop_stmt_list();

    if (! BLOCK_VARS (block))
      return body;

    return build3(BIND_EXPR, void_type_node,
		  BLOCK_VARS (block), body, block);
  }

  // Like end_scope, but also push it into the outer statement-tree.
  void finish_scope()
  {
    tree scope = this->end_scope();
    add_stmt(scope);
  }

  // Return TRUE if IDENT is the current function return label.
  bool is_return_label(Identifier *ident)
  {
    if (this->func_->returnLabel)
      return this->func_->returnLabel->ident == ident;

    return false;
  }

  // Emit a LABEL expression.
  void do_label(tree label)
  {
    // Don't write out label unless it is marked as used by the frontend.
    // This makes auto-vectorization possible in conditional loops.
    // The only excemption to this is in the LabelStatement visitor,
    // in which all computed labels are marked regardless.
    if (TREE_USED (label))
      add_stmt(build1(LABEL_EXPR, void_type_node, label));
  }

  // Emit a goto expression to LABEL.
  void do_jump(Statement *stmt, tree label)
  {
    if (stmt)
      set_input_location(stmt->loc);

    add_stmt(fold_build1(GOTO_EXPR, void_type_node, label));
    TREE_USED (label) = 1;
  }

  // Set and return the current break label for the current block.
  tree push_break_label(Statement *s)
  {
    tree label = lookup_bc_label(s->getRelatedLabeled(), bc_break);
    DECL_CHAIN (label) = this->break_label_;
    this->break_label_ = label;
    return label;
  }

  // Finish with the current break label.
  void pop_break_label(tree label)
  {
    gcc_assert(this->break_label_ == label);
    this->break_label_ = DECL_CHAIN (this->break_label_);
    this->do_label(label);
  }

  // Set and return the continue label for the current block.
  tree push_continue_label(Statement *s)
  {
    tree label = lookup_bc_label(s->getRelatedLabeled(), bc_continue);
    DECL_CHAIN (label) = this->continue_label_;
    this->continue_label_ = label;
    return label;
  }

  // Finish with the current continue label.
  void pop_continue_label(tree label)
  {
    gcc_assert(this->continue_label_ == label);
    this->continue_label_ = DECL_CHAIN (this->continue_label_);
    this->do_label(label);
  }


  // Visitor interfaces.

  // This should be overridden by each statement class.
  void visit(Statement *s)
  {
    set_input_location(s->loc);
    gcc_unreachable();
  }

  // The frontend lowers scope(exit/failure/success) statements as
  // try/catch/finally. At this point, this statement is just an empty
  // placeholder.  Maybe the frontend shouldn't leak these.
  void visit(OnScopeStatement *)
  {
  }

  // if(...) { ... } else { ... }
  void visit(IfStatement *s)
  {
    set_input_location(s->loc);
    this->start_scope(level_cond);

    // Build the outer 'if' condition, which may produce temporaries
    // requiring scope destruction.
    tree ifcond = convert_for_condition(s->condition->toElemDtor(NULL),
					s->condition->type);
    tree ifbody = void_node;
    tree elsebody = void_node;

    // Build the 'then' branch.
    if (s->ifbody)
      {
	push_stmt_list();
	s->ifbody->accept(this);
	ifbody = pop_stmt_list();
      }

    // Now build the 'else' branch, which may have nested 'else if' parts.
    if (s->elsebody)
      {
	push_stmt_list();
	s->elsebody->accept(this);
	elsebody = pop_stmt_list();
      }

    // Wrap up our constructed if condition into a COND_EXPR.
    set_input_location(s->loc);
    tree cond = build3(COND_EXPR, void_type_node, ifcond, ifbody, elsebody);
    add_stmt(cond);

    // Finish the if-then scope.
    this->finish_scope();
  }

  // Should there be any pragma(...) statements requiring code generation,
  // here would be the place to do it.  For now, all pragmas are handled
  // by the frontend.
  void visit(PragmaStatement *)
  {
  }

  // The frontend lowers while(...) statements as for(...) loops.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(WhileStatement *s)
  {
    set_input_location(s->loc);
    gcc_unreachable();
  }

  // 
  void visit(DoStatement *s)
  {
    set_input_location(s->loc);
    tree lbreak = this->push_break_label(s);

    this->start_scope(level_loop);
    if (s->body)
      {
	tree lcontinue = this->push_continue_label(s);
	s->body->accept(this);
	this->pop_continue_label(lcontinue);
      }

    // Build the outer 'while' condition, which may produce temporaries
    // requiring scope destruction.
    set_input_location(s->condition->loc);
    tree exitcond = convert_for_condition(s->condition->toElemDtor(NULL),
					  s->condition->type);
    add_stmt(build1(EXIT_EXPR, void_type_node,
		    build1(TRUTH_NOT_EXPR, TREE_TYPE (exitcond), exitcond)));

    tree body = this->end_scope();
    set_input_location(s->loc);
    add_stmt(build1(LOOP_EXPR, void_type_node, body));

    this->pop_break_label(lbreak);
  }

  // for(...) { ... }
  void visit(ForStatement *s)
  {
    set_input_location(s->loc);
    tree lbreak = this->push_break_label(s);
    this->start_scope(level_loop);

    if (s->init)
      s->init->accept(this);

    if (s->condition)
      {
	set_input_location(s->condition->loc);
	tree exitcond = convert_for_condition(s->condition->toElemDtor(NULL),
					      s->condition->type);
	add_stmt(build1(EXIT_EXPR, void_type_node,
			build1(TRUTH_NOT_EXPR, TREE_TYPE (exitcond), exitcond)));
      }

    if (s->body)
      {
	tree lcontinue = this->push_continue_label(s);
	s->body->accept(this);
	this->pop_continue_label(lcontinue);
      }

    if (s->increment)
      {
	// Force side effects?
	set_input_location(s->increment->loc);
	add_stmt(s->increment->toElemDtor(NULL));
      }

    tree body = this->end_scope();
    set_input_location(s->loc);
    add_stmt(build1(LOOP_EXPR, void_type_node, body));

    this->pop_break_label(lbreak);
  }

  // The frontend lowers foreach(...) statements as for(...) loops.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(ForeachStatement *s)
  {
    set_input_location(s->loc);
    gcc_unreachable();
  }

  // The frontend lowers foreach(...[x..y]) statements as for(...) loops.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(ForeachRangeStatement *s)
  {
    set_input_location(s->loc);
    gcc_unreachable();
  }

  // Jump to the associated exit label for the current loop.
  // If IDENT for the Statement is not null, then the label is user defined.
  void visit(BreakStatement *s)
  {
    if (s->ident)
      {
	// The break label may actually be some levels up.
	// eg: on a try/finally wrapping a loop.
	LabelStatement *label = this->func_->searchLabel(s->ident)->statement;
	gcc_assert(label != NULL);
	Statement *stmt = label->statement->getRelatedLabeled();
	this->do_jump(s, lookup_bc_label(stmt, bc_break));
      }
    else
      this->do_jump(s, this->break_label_);
  }

  // Jump to the associated continue label for the current loop.
  // If IDENT for the Statement is not null, then the label is user defined.
  void visit(ContinueStatement *s)
  {
    if (s->ident)
      {
	LabelStatement *label = this->func_->searchLabel(s->ident)->statement;
	gcc_assert(label != NULL);
	this->do_jump(s, lookup_bc_label(label->statement, bc_continue));
      }
    else
      this->do_jump(s, this->continue_label_);
  }

  //
  void visit(GotoStatement *s)
  {
    gcc_assert(s->label->statement != NULL);
    gcc_assert(s->tf == s->label->statement->tf);

    // This makes the 'undefined label' error show up on the correct line.
    // The extra set_input_location in do_jump shouldn't cause a problem.
    set_input_location(s->loc);

    // If no label found, there was an error.
    tree label = lookup_label(s->label->statement, s->label->ident);
    this->do_jump(s, label);

    // Need to error if the goto is jumping into a try or catch block.
    check_goto(s, s->label->statement);
  }

  //
  void visit(LabelStatement *s)
  {
    LabelDsymbol *sym;

    if (this->is_return_label(s->ident))
      sym = this->func_->returnLabel;
    else
      sym = this->func_->searchLabel(s->ident);

    // If no label found, there was an error
    tree label = define_label(sym->statement, sym->ident);
    TREE_USED (label) = 1;

    this->do_label(label);

    if (this->is_return_label(s->ident) && this->func_->fensure != NULL)
      this->func_->fensure->accept(this);
    else if (s->statement)
      s->statement->accept(this);
  }

  //
  void visit(SwitchStatement *s)
  {
    set_input_location(s->loc);
    this->start_scope(level_switch);
    tree lbreak = this->push_break_label(s);

    tree condition = s->condition->toElemDtor(NULL);
    Type *condtype = s->condition->type->toBasetype();

    // A switch statement on a string gets turned into a library call,
    // which does a binary lookup on list of string cases.
    if (s->condition->type->isString())
      {
	Type *etype = condtype->nextOf()->toBasetype();
	LibCall libcall;

	switch (etype->ty)
	  {
	  case Tchar:
	    libcall = LIBCALL_SWITCH_STRING;
	    break;

	  case Twchar:
	    libcall = LIBCALL_SWITCH_USTRING;
	    break;

	  case Tdchar:
	    libcall = LIBCALL_SWITCH_DSTRING;
	    break;

	  default:
	    ::error("switch statement value must be an array of some character type, not %s",
		    etype->toChars());
	    gcc_unreachable();
	  }

	// Apparently the backend is supposed to sort and set the indexes
	// on the case array, have to change them to be useable.
	Symbol *sym = new Symbol();
	dt_t **pdt = &sym->Sdt;

	s->cases->sort();

	for (size_t i = 0; i < s->cases->dim; i++)
	  {
	    CaseStatement *cs = (*s->cases)[i];
	    cs->index = i;

	    if (cs->exp->op != TOKstring)
	      s->error("case '%s' is not a string", cs->exp->toChars());
	    else
	      pdt = cs->exp->toDt(pdt);
	  }

	sym->Sreadonly = true;
	d_finish_symbol(sym);

	tree args[2];
	args[0] = d_array_value(build_ctype(condtype->arrayOf()),
				size_int(s->cases->dim),
				build_address(sym->Stree));
	args[1] = condition;

	condition = build_libcall(libcall, 2, args);
      }
    else if (!condtype->isscalar())
      {
	::error("cannot handle switch condition of type %s", condtype->toChars());
	gcc_unreachable();
      }

    condition = fold(condition);

    // Build LABEL_DECLs now so they can be refered to by goto case.
    // Also checking the jump from the switch to the label is allowed.
    if (s->cases)
      {
	for (size_t i = 0; i < s->cases->dim; i++)
	  {
	    CaseStatement *cs = (*s->cases)[i];
	    tree caselabel = lookup_label(cs);

	    // Write cases as a series of if-then-else blocks.
	    // if (condition == case)
	    //   goto caselabel;
	    if (s->hasVars)
	      {
		tree ifcase = build2(EQ_EXPR, build_ctype(condtype), condition,
				     cs->exp->toElemDtor(NULL));
		tree ifbody = fold_build1(GOTO_EXPR, void_type_node, caselabel);
		tree cond = build3(COND_EXPR, void_type_node,
				   ifcase, ifbody, void_node);
		TREE_USED (caselabel) = 1;
		D_LABEL_VARIABLE_CASE (caselabel) = 1;
		add_stmt(cond);
	      }

	    check_goto(s, cs);
	  }

	if (s->sdefault)
	  {
	    tree defaultlabel = lookup_label(s->sdefault);

	    // The default label is the last 'else' block.
	    if (s->hasVars)
	      {
		this->do_jump(NULL, defaultlabel);
		D_LABEL_VARIABLE_CASE (defaultlabel) = 1;
	      }

	    check_goto(s, s->sdefault);
	  }
      }

    // Switch body goes in its own statement list.
    push_stmt_list();
    if (s->body)
      s->body->accept(this);

    tree casebody = pop_stmt_list();

    // Wrap up constructed body into a switch_expr, unless it was
    // converted to an if-then-else expression.
    if (s->hasVars)
      add_stmt(casebody);
    else
      {
	tree switchexpr = build3(SWITCH_EXPR, TREE_TYPE (condition),
				 condition, casebody, NULL_TREE);
	add_stmt(switchexpr);
      }

    // If the switch had any 'break' statements, emit the label now.
    this->pop_break_label(lbreak);
    this->finish_scope();
  }

  //
  void visit(CaseStatement *s)
  {
    // Emit the case label.
    tree label = define_label(s);

    if (D_LABEL_VARIABLE_CASE (label))
      this->do_label(label);
    else
      {
	tree casevalue;
	if (s->exp->type->isscalar())
	  casevalue = s->exp->toElem(NULL);
	else
	  casevalue = build_integer_cst(s->index, build_ctype(Type::tint32));

	tree caselabel = build_case_label(casevalue, NULL_TREE, label);
	add_stmt(caselabel);
      }

    // Now do the body.
    if (s->statement)
      s->statement->accept(this);
  }

  //
  void visit(DefaultStatement *s)
  {
    // Emit the default case label.
    tree label = define_label(s);

    if (D_LABEL_VARIABLE_CASE (label))
      this->do_label(label);
    else
      {
	tree caselabel = build_case_label(NULL_TREE, NULL_TREE, label);
	add_stmt(caselabel);
      }

    // Now do the body.
    if (s->statement)
      s->statement->accept(this);
  }

  // Implements 'goto default' by jumping to the label associated with
  // the DefaultStatement in a switch block.
  void visit(GotoDefaultStatement *s)
  {
    tree label = lookup_label(s->sw->sdefault);
    this->do_jump(s, label);
  }

  // Implements 'goto case' by jumping to the label associated with the
  // CaseStatement in a switch block.
  void visit(GotoCaseStatement *s)
  {
    tree label = lookup_label(s->cs);
    this->do_jump(s, label);
  }

  // Throw a SwitchError exception, called when a switch statement has
  // no DefaultStatement, yet none of the cases match.
  void visit(SwitchErrorStatement *s)
  {
    set_input_location(s->loc);
    add_stmt(d_assert_call(s->loc, LIBCALL_SWITCH_ERROR));
  }

  //
  void visit(ReturnStatement *s)
  {
    set_input_location(s->loc);

    if (s->exp == NULL || s->exp->type->toBasetype()->ty == Tvoid)
      {
	// Return has no value.
	add_stmt(return_expr(NULL_TREE));
	return;
      }

    TypeFunction *tf = (TypeFunction *)this->func_->type;
    Type *type = this->func_->tintro != NULL
      ? this->func_->tintro->nextOf() : tf->nextOf();

    if (this->func_->isMain() && type->toBasetype()->ty == Tvoid)
      type = Type::tint32;

    tree decl = DECL_RESULT (this->func_->toSymbol()->Stree);

    if (this->func_->nrvo_can && this->func_->nrvo_var)
      {
	// Just refer to the DECL_RESULT; this is a nop, but differs
	// from using NULL_TREE in that it indicates that we care about
	// the value of the DECL_RESULT.
	add_stmt(return_expr(decl));
      }
    else
      {
	// Convert for initialising the DECL_RESULT.
	tree value = convert_expr(s->exp->toElemDtor(NULL),
				  s->exp->type, type);

	// If we are returning a reference, take the address.
	if (tf->isref)
	  value = build_address(value);

	tree assign = build2(INIT_EXPR, TREE_TYPE (decl), decl, value);
	add_stmt(return_expr(assign));
      }
  }

  // 
  void visit(ExpStatement *s)
  {
    if (s->exp)
      {
	set_input_location(s->loc);
	// Expression may produce temporaries requiring scope destruction.
	tree exp = s->exp->toElemDtor(NULL);
	add_stmt(exp);
      }
  }

  //
  void visit(CompoundStatement *s)
  {
    if (s->statements == NULL)
      return;

    for (size_t i = 0; i < s->statements->dim; i++)
      {
	Statement *statement = (*s->statements)[i];

	if (statement != NULL)
	  statement->accept(this);
      }
  }

  // The frontend lowers foreach(Tuple!(...)) statements as an unrolled loop.
  // These are compiled down as a do ... while(0), where each unrolled loop is
  // nested inside and given their own continue label to jump to.
  void visit(UnrolledLoopStatement *s)
  {
    if (s->statements == NULL)
      return;

    tree lbreak = this->push_break_label(s);
    this->start_scope(level_loop);

    for (size_t i = 0; i < s->statements->dim; i++)
      {
	Statement *statement = (*s->statements)[i];

	if (statement != NULL)
	  {
	    tree lcontinue = this->push_continue_label(statement);
	    statement->accept(this);
	    this->pop_continue_label(lcontinue);
	  }
      }

    this->do_jump(NULL, this->break_label_);

    tree body = this->end_scope();
    set_input_location(s->loc);
    add_stmt(build1(LOOP_EXPR, void_type_node, body));

    this->pop_break_label(lbreak);
  }

  // Start a new scope and visit all nested statements, wrapping
  // them up into a BIND_EXPR at the end of the scope.
  void visit(ScopeStatement *s)
  {
    if (s->statement == NULL)
      return;

    this->start_scope(level_block);
    s->statement->accept(this);
    this->finish_scope();
  }

  //
  void visit(WithStatement *s)
  {
    set_input_location(s->loc);
    this->start_scope(level_with);

    if (s->wthis)
      {
	// Perform initialisation of the 'with' handle.
	ExpInitializer *ie = s->wthis->init->isExpInitializer();
	gcc_assert(ie != NULL);

	build_local_var(s->wthis);
	tree init = ie->exp->toElemDtor(NULL);
	add_stmt(init);
      }

    if (s->body)
      s->body->accept(this);

    this->finish_scope();
  }

  // Implements 'throw Object'.  Frontend already checks that the object
  // thrown is a class type, but does not check if it is derived from
  // Object.  Foreign objects are not currently supported in runtime.
  void visit(ThrowStatement *s)
  {
    ClassDeclaration *cd = s->exp->type->toBasetype()->isClassHandle();
    InterfaceDeclaration *id = cd->isInterfaceDeclaration();
    tree arg = s->exp->toElemDtor(NULL);

    if (!flag_exceptions)
      {
	static int warned = 0;
	if (!warned)
	  {
	    s->error("exception handling disabled, use -fexceptions to enable");
	    warned = 1;
	  }
      }

    if (cd->cpp || (id != NULL && id->cpp))
      s->error("cannot throw C++ classes");
    else if (cd->com || (id != NULL && id->com))
      s->error("cannot throw COM objects");
    else
      arg = build_nop(build_ctype(build_object_type()), arg);

    set_input_location(s->loc);
    add_stmt(build_libcall(LIBCALL_THROW, 1, &arg));
  }

  //
  void visit(TryCatchStatement *s)
  {
    set_input_location(s->loc);

    this->start_scope(level_try);
    if (s->body)
      s->body->accept(this);

    tree trybody = this->end_scope();

    // Try handlers go in their own statement list.
    push_stmt_list();

    if (s->catches)
      {
	for (size_t i = 0; i < s->catches->dim; i++)
	  {
	    Catch *vcatch = (*s->catches)[i];

	    set_input_location(vcatch->loc);
	    this->start_scope(level_catch);

	    tree catchtype = build_ctype(vcatch->type);

	    // Get D's internal exception Object, different from the generic
	    // exception pointer returned from gcc runtime.
	    tree ehptr = d_build_call_nary(builtin_decl_explicit(BUILT_IN_EH_POINTER),
					   1, integer_zero_node);
	    tree object = build_libcall(LIBCALL_BEGIN_CATCH, 1, &ehptr);
	    if (vcatch->var)
	      {
		object = build_nop(build_ctype(vcatch->type), object);

		tree var = vcatch->var->toSymbol()->Stree;
		tree init = build_vinit(var, object);

		build_local_var(vcatch->var);
		add_stmt(init);
	      }
	    else
	      {
		// Still need to emit a call to __gdc_begin_catch() to remove
		// the object from the uncaught exceptions list.
		add_stmt(object);
	      }

	    if (vcatch->handler)
	      vcatch->handler->accept(this);

	    tree catchbody = this->end_scope();
	    add_stmt(build2(CATCH_EXPR, void_type_node, catchtype, catchbody));
	  }
      }

    tree catches = pop_stmt_list();

    // Backend expects all catches in a TRY_CATCH_EXPR to be enclosed in a
    // statement list, however pop_stmt_list may optimise away the list
    // if there is only a single catch to push.
    if (TREE_CODE (catches) != STATEMENT_LIST)
      {
        tree stmt_list = alloc_stmt_list();
        append_to_statement_list_force(catches, &stmt_list);
        catches = stmt_list;
      }

    set_input_location(s->loc);
    add_stmt(build2(TRY_CATCH_EXPR, void_type_node, trybody, catches));
  }

  //
  void visit(TryFinallyStatement *s)
  {
    set_input_location(s->loc);
    this->start_scope(level_try);
    if (s->body)
      s->body->accept(this);

    tree trybody = this->end_scope();

    this->start_scope(level_finally);
    if (s->finalbody)
      s->finalbody->accept(this);

    tree finally = this->end_scope();

    set_input_location(s->loc);
    add_stmt(build2(TRY_FINALLY_EXPR, void_type_node, trybody, finally));
  }

  // The frontend lowers synchronized(...) statements as a call to
  // monitor/critical enter and exit wrapped around try/finally.
  // This visitor is not strictly required other than to enforce that
  // these kinds of statements never reach here.
  void visit(SynchronizedStatement *s)
  {
    set_input_location(s->loc);
    gcc_unreachable();
  }

  // D Inline Assembler is not implemented, as would require a writing
  // an assembly parser for each supported target.  Instead we leverage
  // GCC extended assembler using the ExtAsmStatement class.
  void visit(AsmStatement *s)
  {
    set_input_location(s->loc);
    sorry("D inline assembler statements are not supported in GDC.");
  }

  // Build a GCC extended assembler expression, whose components are
  // an INSN string, some OUTPUTS, some INPUTS, and some CLOBBERS.
  void visit(ExtAsmStatement *s)
  {
    StringExp *insn = (StringExp *)s->insn;
    tree outputs = NULL_TREE;
    tree inputs = NULL_TREE;
    tree clobbers = NULL_TREE;
    tree labels = NULL_TREE;

    set_input_location(s->loc);

    // Collect all arguments, which may be input or output operands.
    if (s->args)
      {
	for (size_t i = 0; i < s->args->dim; i++)
	  {
	    Identifier *name = (*s->names)[i];
	    StringExp *constr = (StringExp *)(*s->constraints)[i];
	    Expression *arg = (*s->args)[i];

	    tree id = name ? build_string(name->len, name->string) : NULL_TREE;
	    tree str = build_string(constr->len, (char *)constr->string);
	    tree val = arg->toElem(NULL);

	    if (i < s->outputargs)
	      {
		tree arg = build_tree_list(id, str);
		outputs = chainon(outputs, build_tree_list(arg, val));
	      }
	    else
	      {
		tree arg = build_tree_list(id, str);
		inputs = chainon(inputs, build_tree_list(arg, val));
	      }
	  }
      }

    // Collect all clobber arguments.
    if (s->clobbers)
      {
	for (size_t i = 0; i < s->clobbers->dim; i++)
	  {
	    StringExp *clobber = (StringExp *)(*s->clobbers)[i];
	    tree val = build_string(clobber->len, (char *)clobber->string);
	    clobbers = chainon(clobbers, build_tree_list(0, val));
	  }
      }

    // Collect all goto labels, these should have been already checked
    // by the front-end, so pass down the label symbol to the backend.
    if (s->labels)
      {
	for (size_t i = 0; i < s->labels->dim; i++)
	  {
	    Identifier *ident = (*s->labels)[i];
	    GotoStatement *gs = (*s->gotos)[i];

	    gcc_assert(gs->label->statement != NULL);
	    gcc_assert(gs->tf == gs->label->statement->tf);

	    tree name = build_string(ident->len, ident->string);
	    tree label = lookup_label(gs->label->statement, gs->label->ident);
	    TREE_USED (label) = 1;

	    labels = chainon(labels, build_tree_list(name, label));
	  }
      }

    // Should also do some extra validation on all input and output operands.
    tree string = build_string(insn->len, (char *)insn->string);
    string = resolve_asm_operand_names(string, outputs, inputs, labels);

    tree exp = build5(ASM_EXPR, void_type_node, string,
		      outputs, inputs, clobbers, labels);
    SET_EXPR_LOCATION (exp, input_location);

    // If the extended syntax was not used, mark the ASM_EXPR.
    if (s->args == NULL && s->clobbers == NULL)
      ASM_INPUT_P (exp) = 1;

    // Asm statements without outputs are treated as volatile.
    ASM_VOLATILE_P (exp) = (s->outputargs == 0);

    add_stmt(exp);
  }

  //
  void visit(ImportStatement *s)
  {
    if (s->imports == NULL)
      return;

    for (size_t i = 0; i < s->imports->dim; i++)
      {
	Dsymbol *dsym = (*s->imports)[i];

	if (dsym != NULL)
	  dsym->toObjFile(0);
      }
  }
};


// Main entry point for the IRVisitor interface to generate code for the
// statement AST class S.  IRS holds the state of the current function.

void
build_ir(FuncDeclaration *fd)
{
  IRVisitor v = IRVisitor(fd);
  fd->fbody->accept(&v);
}


