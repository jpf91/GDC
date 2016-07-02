// d-lang.cc -- D frontend for GCC.
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

// d-lang.cc: Implementation of back-end callbacks and data structures

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/mars.h"
#include "dfrontend/mtype.h"
#include "dfrontend/aggregate.h"
#include "dfrontend/cond.h"
#include "dfrontend/hdrgen.h"
#include "dfrontend/doc.h"
#include "dfrontend/json.h"
#include "dfrontend/lexer.h"
#include "dfrontend/module.h"
#include "dfrontend/scope.h"
#include "dfrontend/statement.h"
#include "dfrontend/root.h"
#include "dfrontend/target.h"

#include "d-system.h"
#include "options.h"
#include "cppdefault.h"
#include "debug.h"

#include "d-tree.h"
#include "d-codegen.h"
#include "d-objfile.h"
#include "d-dmd-gcc.h"
#include "id.h"

static const char *iprefix_dir = NULL;
static const char *imultilib_dir = NULL;

static char lang_name[6] = "GNU D";

/* Lang Hooks */
#undef LANG_HOOKS_NAME
#undef LANG_HOOKS_INIT
#undef LANG_HOOKS_INIT_TS
#undef LANG_HOOKS_INIT_OPTIONS
#undef LANG_HOOKS_INIT_OPTIONS_STRUCT
#undef LANG_HOOKS_OPTION_LANG_MASK
#undef LANG_HOOKS_HANDLE_OPTION
#undef LANG_HOOKS_POST_OPTIONS
#undef LANG_HOOKS_PARSE_FILE
#undef LANG_HOOKS_COMMON_ATTRIBUTE_TABLE
#undef LANG_HOOKS_ATTRIBUTE_TABLE
#undef LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE
#undef LANG_HOOKS_GET_ALIAS_SET
#undef LANG_HOOKS_TYPES_COMPATIBLE_P
#undef LANG_HOOKS_BUILTIN_FUNCTION
#undef LANG_HOOKS_BUILTIN_FUNCTION_EXT_SCOPE
#undef LANG_HOOKS_REGISTER_BUILTIN_TYPE
#undef LANG_HOOKS_FINISH_INCOMPLETE_DECL
#undef LANG_HOOKS_GIMPLIFY_EXPR
#undef LANG_HOOKS_CLASSIFY_RECORD
#undef LANG_HOOKS_EH_PERSONALITY
#undef LANG_HOOKS_EH_RUNTIME_TYPE

#define LANG_HOOKS_NAME				lang_name
#define LANG_HOOKS_INIT				d_init
#define LANG_HOOKS_INIT_TS			d_init_ts
#define LANG_HOOKS_INIT_OPTIONS			d_init_options
#define LANG_HOOKS_INIT_OPTIONS_STRUCT		d_init_options_struct
#define LANG_HOOKS_OPTION_LANG_MASK		d_option_lang_mask
#define LANG_HOOKS_HANDLE_OPTION		d_handle_option
#define LANG_HOOKS_POST_OPTIONS			d_post_options
#define LANG_HOOKS_PARSE_FILE			d_parse_file
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE       d_langhook_common_attribute_table
#define LANG_HOOKS_ATTRIBUTE_TABLE              d_langhook_attribute_table
#define LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE	d_langhook_format_attribute_table
#define LANG_HOOKS_GET_ALIAS_SET		d_get_alias_set
#define LANG_HOOKS_TYPES_COMPATIBLE_P		d_types_compatible_p
#define LANG_HOOKS_BUILTIN_FUNCTION		d_builtin_function
#define LANG_HOOKS_BUILTIN_FUNCTION_EXT_SCOPE	d_builtin_function
#define LANG_HOOKS_REGISTER_BUILTIN_TYPE	d_register_builtin_type
#define LANG_HOOKS_FINISH_INCOMPLETE_DECL	d_finish_incomplete_decl
#define LANG_HOOKS_GIMPLIFY_EXPR		d_gimplify_expr
#define LANG_HOOKS_CLASSIFY_RECORD		d_classify_record
#define LANG_HOOKS_EH_PERSONALITY		d_eh_personality
#define LANG_HOOKS_EH_RUNTIME_TYPE		d_build_eh_type_type

/* Lang Hooks for decls */
#undef LANG_HOOKS_PUSHDECL
#undef LANG_HOOKS_GETDECLS
#undef LANG_HOOKS_GLOBAL_BINDINGS_P
#undef LANG_HOOKS_WRITE_GLOBALS

#define LANG_HOOKS_PUSHDECL			d_pushdecl
#define LANG_HOOKS_GETDECLS			d_getdecls
#define LANG_HOOKS_GLOBAL_BINDINGS_P		d_global_bindings_p
#define LANG_HOOKS_WRITE_GLOBALS		d_write_global_declarations

/* Lang Hooks for types */
#undef LANG_HOOKS_TYPE_FOR_MODE
#undef LANG_HOOKS_TYPE_FOR_SIZE
#undef LANG_HOOKS_TYPE_PROMOTES_TO

#define LANG_HOOKS_TYPE_FOR_MODE		d_type_for_mode
#define LANG_HOOKS_TYPE_FOR_SIZE		d_type_for_size
#define LANG_HOOKS_TYPE_PROMOTES_TO		d_type_promotes_to


static const char *fonly_arg;

/* List of modules being compiled.  */
Modules builtin_modules;
Modules output_modules;

static Module *output_module = NULL;

static Module *entrypoint = NULL;
static Module *rootmodule = NULL;

/* Zero disables all standard directories for headers.  */
static bool std_inc = true;

/* The current and global binding level in effect.  */
struct binding_level *current_binding_level;
struct binding_level *global_binding_level;

/* Common initialization before calling option handlers.  */
static void
d_init_options(unsigned int, cl_decoded_option *decoded_options)
{
  // Set default values
  global.init();

  global.compiler.vendor = lang_name;

  global.params.argv0 = xstrdup(decoded_options[0].arg);
  global.params.link = true;
  global.params.useAssert = true;
  global.params.useInvariants = true;
  global.params.useIn = true;
  global.params.useOut = true;
  global.params.useArrayBounds = BOUNDSCHECKdefault;
  global.params.useSwitchError = true;
  global.params.useInline = false;
  global.params.warnings = 0;
  global.params.obj = true;
  global.params.useDeprecated = 1;
  global.params.betterC = false;
  global.params.allInst = false;

  global.params.linkswitches = new Strings();
  global.params.libfiles = new Strings();
  global.params.objfiles = new Strings();
  global.params.ddocfiles = new Strings();

  global.params.imppath = new Strings();
  global.params.fileImppath = new Strings();

  // extra D-specific options
  flag_emit_templates = 1;
}

/* Initialize options structure OPTS.  */
static void
d_init_options_struct(gcc_options *opts)
{
  // GCC options
  opts->x_flag_exceptions = 1;

  // Avoid range issues for complex multiply and divide.
  opts->x_flag_complex_method = 2;

  // Unlike C, there is no global 'errno' variable.
  opts->x_flag_errno_math = 0;
  opts->frontend_set_flag_errno_math = true;

  // Keep in synch with existing -fbounds-check flag.
  opts->x_flag_bounds_check = global.params.useArrayBounds;

  // D says that signed overflow is precisely defined.
  opts->x_flag_wrapv = 1;
}

/* Return language mask for option parsing.  */
static unsigned int
d_option_lang_mask()
{
  return CL_D;
}

static void
d_add_builtin_version(const char* ident)
{
  if (strcmp (ident, "linux") == 0)
    global.params.isLinux = true;
  else if (strcmp (ident, "OSX") == 0)
    global.params.isOSX = true;
  else if (strcmp (ident, "Windows") == 0)
    global.params.isWindows = true;
  else if (strcmp (ident, "FreeBSD") == 0)
    global.params.isFreeBSD = true;
  else if (strcmp (ident, "OpenBSD") == 0)
    global.params.isOpenBSD = true;
  else if (strcmp (ident, "Solaris") == 0)
    global.params.isSolaris = true;
  else if (strcmp (ident, "X86_64") == 0)
    global.params.is64bit = true;

  VersionCondition::addPredefinedGlobalIdent (ident);
}

static bool
d_init()
{
  Lexer::initLexer();
  Type::init();
  Id::initialize();
  Module::init();
  Expression::init();
  initPrecedence();
  initTraitsStringTable();

  // Backend init.
  global_binding_level = ggc_alloc_cleared_binding_level();
  current_binding_level = global_binding_level;

  // This allows the code in d-builtins.c to not have to worry about
  // converting (C signed char *) to (D char *) for string arguments of
  // built-in functions.
  // Parameters are (signed_char = false, short_double = false).
  build_common_tree_nodes (false, false);

  d_init_builtins();

  if (flag_exceptions)
    using_eh_for_cleanups();

  // This is the C main, not the D main.
  main_identifier_node = get_identifier ("main");

  longdouble::init();
  Target::init();
  Port::init();

#ifndef TARGET_CPU_D_BUILTINS
# define TARGET_CPU_D_BUILTINS()
#endif

#ifndef TARGET_OS_D_BUILTINS
# define TARGET_OS_D_BUILTINS()
#endif

# define builtin_define(TXT) d_add_builtin_version(TXT)

  TARGET_CPU_D_BUILTINS();
  TARGET_OS_D_BUILTINS();

  VersionCondition::addPredefinedGlobalIdent ("GNU");
  VersionCondition::addPredefinedGlobalIdent ("D_Version2");

  if (BYTES_BIG_ENDIAN)
    VersionCondition::addPredefinedGlobalIdent ("BigEndian");
  else
    VersionCondition::addPredefinedGlobalIdent ("LittleEndian");

  if (targetm_common.except_unwind_info (&global_options) == UI_SJLJ)
    VersionCondition::addPredefinedGlobalIdent ("GNU_SjLj_Exceptions");
  else if(targetm_common.except_unwind_info (&global_options) == UI_SEH)
    VersionCondition::addPredefinedGlobalIdent ("GNU_SEH_Exceptions");
  else if(targetm_common.except_unwind_info (&global_options) == UI_DWARF2)
    VersionCondition::addPredefinedGlobalIdent ("GNU_DWARF2_Exceptions");

  if(!targetm.have_tls)
    VersionCondition::addPredefinedGlobalIdent ("GNU_EMUTLS");

#ifdef STACK_GROWS_DOWNWARD
  VersionCondition::addPredefinedGlobalIdent ("GNU_StackGrowsDown");
#endif

  /* Should define this anyway to set us apart from the competition. */
  VersionCondition::addPredefinedGlobalIdent ("GNU_InlineAsm");

  /* LP64 only means 64bit pointers in D. */
  if (global.params.isLP64)
    VersionCondition::addPredefinedGlobalIdent ("D_LP64");

  /* Setting global.params.cov forces module info generation which is
     not needed for thee GCC coverage implementation.  Instead, just
     test flag_test_coverage while leaving global.params.cov unset. */
  //if (global.params.cov)
  if (flag_test_coverage)
    VersionCondition::addPredefinedGlobalIdent ("D_Coverage");
  if (flag_pic)
    VersionCondition::addPredefinedGlobalIdent ("D_PIC");
  if (global.params.doDocComments)
    VersionCondition::addPredefinedGlobalIdent ("D_Ddoc");
  if (global.params.useUnitTests)
    VersionCondition::addPredefinedGlobalIdent ("unittest");
  if (global.params.useAssert)
    VersionCondition::addPredefinedGlobalIdent("assert");
  if (global.params.useArrayBounds == BOUNDSCHECKoff)
    VersionCondition::addPredefinedGlobalIdent("D_NoBoundsChecks");

  VersionCondition::addPredefinedGlobalIdent ("all");

  /* Insert all library-configured identifiers and import paths.  */
  add_import_paths(iprefix_dir, imultilib_dir, std_inc);

  return 1;
}

void
d_init_ts()
{
  MARK_TS_TYPED (IASM_EXPR);
  MARK_TS_TYPED (FLOAT_MOD_EXPR);
  MARK_TS_TYPED (UNSIGNED_RSHIFT_EXPR);
}

//
static bool
d_handle_option (size_t scode, const char *arg, int value,
		 int kind ATTRIBUTE_UNUSED,
		 location_t loc ATTRIBUTE_UNUSED,
		 const cl_option_handlers *handlers ATTRIBUTE_UNUSED)
{
  opt_code code = (opt_code) scode;
  bool result = true;

  switch (code)
    {
    case OPT_fassert:
      global.params.useAssert = value;
      break;

    case OPT_fbounds_check:
      global.params.useArrayBounds = value
	? BOUNDSCHECKon : BOUNDSCHECKoff;
      break;

    case OPT_fbounds_check_:
      global.params.useArrayBounds = (value == 2) ? BOUNDSCHECKon
	: (value == 1) ? BOUNDSCHECKsafeonly : BOUNDSCHECKoff;
      break;

    case OPT_fdebug:
      global.params.debuglevel = value ? 1 : 0;
      break;

    case OPT_fdebug_:
      if (ISDIGIT (arg[0]))
	{
	  int level = integral_argument(arg);
	  if (level != -1)
	    {
	      DebugCondition::setGlobalLevel(level);
	      break;
	    }
	}

      if (Lexer::isValidIdentifier(CONST_CAST (char *, arg)))
	{
	  DebugCondition::addGlobalIdent(arg);
	  break;
	}

      error ("bad argument for -fdebug '%s'", arg);
      break;

    case OPT_fdeps:
      global.params.moduleDeps = new OutBuffer;
      break;

    case OPT_fdeps_:
      global.params.moduleDepsFile = arg;
      if (!global.params.moduleDepsFile[0])
	error ("bad argument for -fdeps");
      global.params.moduleDeps = new OutBuffer;
      break;

    case OPT_fdoc:
      global.params.doDocComments = value;
      break;

    case OPT_fdoc_dir_:
      global.params.doDocComments = true;
      global.params.docdir = arg;
      break;

    case OPT_fdoc_file_:
      global.params.doDocComments = true;
      global.params.docname = arg;
      break;

    case OPT_fdoc_inc_:
      global.params.ddocfiles->push (arg);
      break;

    case OPT_femit_templates:
      flag_emit_templates = value ? 1 : 0;
      global.params.allInst = value;
      break;

    case OPT_fignore_unknown_pragmas:
      global.params.ignoreUnsupportedPragmas = value;
      break;

    case OPT_fin:
      global.params.useIn = value;
      break;

    case OPT_fintfc:
      global.params.doHdrGeneration = value;
      break;

    case OPT_fintfc_dir_:
      global.params.doHdrGeneration = true;
      global.params.hdrdir = arg;
      break;

    case OPT_fintfc_file_:
      global.params.doHdrGeneration = true;
      global.params.hdrname = arg;
      break;

    case OPT_finvariants:
      global.params.useInvariants = value;
      break;

    case OPT_fmake_deps:
      global.params.makeDeps = new OutBuffer;
      break;

    case OPT_fmake_deps_:
      global.params.makeDeps = new OutBuffer;
      global.params.makeDepsStyle = 1;
      global.params.makeDepsFile = arg;
      if (!global.params.makeDepsFile[0])
	error ("bad argument for -fmake-deps");
      break;

    case OPT_fmake_mdeps:
      global.params.makeDeps = new OutBuffer;
      break;

    case OPT_fmake_mdeps_:
      global.params.makeDeps = new OutBuffer;
      global.params.makeDepsStyle = 2;
      global.params.makeDepsFile = arg;
      if (!global.params.makeDepsFile[0])
	error ("bad argument for -fmake-deps");
      break;

    case OPT_fmoduleinfo:
      global.params.betterC = !value;
      break;

    case OPT_fonly_:
      fonly_arg = arg;
      break;

    case OPT_fout:
      global.params.useOut = value;
      break;

    case OPT_fproperty:
      global.params.enforcePropertySyntax = value;
      break;

    case OPT_frelease:
      global.params.release = value;
      global.params.useInvariants = !value;
      global.params.useIn = !value;
      global.params.useOut = !value;
      global.params.useAssert = !value;
      global.params.useSwitchError = !value;
      break;

    case OPT_ftransition_complex:
      global.params.vcomplex = value;
      break;

    case OPT_ftransition_field:
      global.params.vfield = value;
      break;

    case OPT_ftransition_nogc:
      global.params.vgc = value;
      break;

    case OPT_ftransition_tls:
      global.params.vtls = value;
      break;

    case OPT_ftransition_dip25:
      global.params.useDIP25 = value;
      break;

    case OPT_funittest:
      global.params.useUnitTests = value;
      break;

    case OPT_fversion_:
      if (ISDIGIT (arg[0]))
	{
	  int level = integral_argument(arg);
	  if (level != -1)
	    {
	      VersionCondition::setGlobalLevel (level);
	      break;
	    }
	}

      if (Lexer::isValidIdentifier (CONST_CAST (char *, arg)))
	{
	  VersionCondition::addGlobalIdent (arg);
	  break;
	}

      error ("bad argument for -fversion '%s'", arg);
      break;

    case OPT_fXf_:
      global.params.doJsonGeneration = true;
      global.params.jsonfilename = arg;
      break;

    case OPT_imultilib:
      imultilib_dir = arg;
      break;

    case OPT_iprefix:
      iprefix_dir = arg;
      break;

    case OPT_I:
      global.params.imppath->push(arg); // %% not sure if we can keep the arg or not
      break;

    case OPT_J:
      global.params.fileImppath->push(arg);
      break;

    case OPT_nostdinc:
      std_inc = false;
      break;

    case OPT_v:
      global.params.verbose = value;
      break;

    case OPT_Wall:
      if (value)
	global.params.warnings = 2;
      break;

    case OPT_Wdeprecated:
      global.params.useDeprecated = value ? 2 : 1;
      break;

    case OPT_Werror:
      if (value)
	global.params.warnings = 1;
      break;

    case OPT_fmax_error_messages_:
      {
	int limit = integral_argument(arg);
	if (limit == -1)
	  error ("bad argument for -fmax-error-messages '%s'", arg);
	else
	  global.errorLimit = limit;
	break;
      }

    default:
      break;
    }

  return result;
}

bool
d_post_options (const char ** fn)
{
  // Canonicalize the input filename.
  if (in_fnames == NULL)
    {
      in_fnames = XNEWVEC (const char *, 1);
      in_fnames[0] = "";
    }
  else if (strcmp (in_fnames[0], "-") == 0)
    in_fnames[0] = "";

  // The front end considers the first input file to be the main one.
  if (num_in_fnames)
    *fn = in_fnames[0];

  // If we are given more than one input file, we must use unit-at-a-time mode.
  if (num_in_fnames > 1)
    flag_unit_at_a_time = 1;

  // Release mode doesn't turn off bounds checking for safe functions.
  if (global.params.useArrayBounds == BOUNDSCHECKdefault)
    {
      global.params.useArrayBounds = global.params.release
	? BOUNDSCHECKsafeonly : BOUNDSCHECKon;
      flag_bounds_check = !global.params.release;
    }

  // Error about use of deprecated features.
  if (global.params.useDeprecated == 2 && global.params.warnings == 1)
    global.params.useDeprecated = 0;

  // Make -fmax-errors visible to gdc's diagnostic machinery.
  if (global_options_set.x_flag_max_errors)
    global.errorLimit = flag_max_errors;

  if (flag_excess_precision_cmdline == EXCESS_PRECISION_DEFAULT)
    flag_excess_precision_cmdline = EXCESS_PRECISION_STANDARD;

  if (global.params.useUnitTests)
    global.params.useAssert = true;

  global.params.symdebug = write_symbols != NO_DEBUG;
  global.params.useInline = flag_inline_functions;
  global.params.obj = !flag_syntax_only;
  // Has no effect yet.
  global.params.pic = flag_pic != 0;

  return false;
}

// Return TRUE if an operand OP of a given TYPE being copied has no data.
// The middle-end does a similar check with zero sized types.
static bool
empty_modify_p(tree type, tree op)
{
  tree_code code = TREE_CODE (op);
  switch (code)
    {
    case COMPOUND_EXPR:
      return empty_modify_p(type, TREE_OPERAND (op, 1));

    case CONSTRUCTOR:
      // Non-empty construcors are valid.
      if (CONSTRUCTOR_NELTS (op) != 0 || TREE_CLOBBER_P (op))
	return false;
      break;

    case CALL_EXPR:
      // Leave nrvo alone because it isn't a copy.
      if (CALL_EXPR_RETURN_SLOT_OPT (op))
	return false;
      break;

    default:
      // If the operand doesn't have a simple form.
      if (!is_gimple_lvalue(op) && !INDIRECT_REF_P (op))
	return false;
      break;
    }

  return empty_aggregate_p(type);
}

// Gimplification of D specific expression trees.

int
d_gimplify_expr(tree *expr_p, gimple_seq *pre_p ATTRIBUTE_UNUSED,
		gimple_seq *post_p ATTRIBUTE_UNUSED)
{
  tree_code code = TREE_CODE (*expr_p);
  enum gimplify_status ret = GS_UNHANDLED;
  tree op0, op1;
  tree type;

  switch (code)
    {
    case INIT_EXPR:
    case MODIFY_EXPR:
      op0 = TREE_OPERAND (*expr_p, 0);
      op1 = TREE_OPERAND (*expr_p, 1);

      if (!error_operand_p(op0) && !error_operand_p(op1)
	  && (AGGREGATE_TYPE_P (TREE_TYPE (op0))
	      || AGGREGATE_TYPE_P (TREE_TYPE (op1)))
	  && !useless_type_conversion_p(TREE_TYPE (op1), TREE_TYPE (op0)))
	{
	  // If the back end isn't clever enough to know that the lhs and rhs
	  // types are the same, add an explicit conversion.
	  TREE_OPERAND (*expr_p, 1) = build1(VIEW_CONVERT_EXPR,
					     TREE_TYPE (op0), op1);
	  ret = GS_OK;
	}
      else if (empty_modify_p(TREE_TYPE (op0), op1))
	{
	  // Remove any copies of empty aggregates.  Also drop volatile
	  // loads on the RHS to avoid infinite recursion from
	  // gimplify_expr trying to load the value.
	  gimplify_expr(&TREE_OPERAND (*expr_p, 0), pre_p, post_p,
			is_gimple_lvalue, fb_lvalue);
	  if (TREE_SIDE_EFFECTS (op1))
	    {
	      if (TREE_THIS_VOLATILE (op1)
		  && (REFERENCE_CLASS_P (op1) || DECL_P (op1)))
		op1 = build_fold_addr_expr(op1);

	      gimplify_and_add(op1, pre_p);
	    }
	  *expr_p = TREE_OPERAND (*expr_p, 0);
	  ret = GS_OK;
	}
      break;

    case ADDR_EXPR:
      op0 = TREE_OPERAND (*expr_p, 0);
      // Constructors are not lvalues, so make them one.
      if (TREE_CODE (op0) == CONSTRUCTOR)
	{
	  TREE_OPERAND (*expr_p, 0) = build_target_expr(op0);
	  ret = GS_OK;
	}
      break;

    case UNSIGNED_RSHIFT_EXPR:
      // Convert op0 to an unsigned type.
      op0 = TREE_OPERAND (*expr_p, 0);
      op1 = TREE_OPERAND (*expr_p, 1);

      type = d_unsigned_type(TREE_TYPE (op0));

      *expr_p = convert(TREE_TYPE (*expr_p),
			build2(RSHIFT_EXPR, type, convert(type, op0), op1));
      ret = GS_OK;
      break;

    case FLOAT_MOD_EXPR:
    case IASM_EXPR:
      gcc_unreachable();

    default:
      break;
    }

  return ret;
}


Module *
d_gcc_get_output_module()
{
  return output_module;
}

static void
d_nametype (Type *t)
{
  tree type = build_ctype(t);
  tree ident = get_identifier (t->toChars());
  tree decl = build_decl (BUILTINS_LOCATION, TYPE_DECL, ident, type);
  TYPE_NAME (type) = decl;
  rest_of_decl_compilation (decl, 1, 0);
}

// Generate C main() in response to seeing D main().
// This used to be in libdruntime, but contained a reference to _Dmain which
// didn't work when druntime was made into a shared library and was linked
// to a program, such as a C++ program, that didn't have a _Dmain.

void
genCmain (Scope *sc)
{
  if (entrypoint)
    return;

  // The D code to be generated is provided by __entrypoint.di
  Module *m = Module::load (Loc(), NULL, Id::entrypoint);
  m->importedFrom = m;
  m->importAll (NULL);
  m->semantic();
  m->semantic2();
  m->semantic3();

  // We are emitting this straight to object file.
  output_modules.push (m);
  entrypoint = m;
  rootmodule = sc->module;
}

static bool
is_system_module(Module *m)
{
  // Don't emit system modules. This includes core.*, std.*, gcc.* and object.
  ModuleDeclaration *md = m->md;

  if(!md)
    return false;

  if (md->packages)
    {
      if (strcmp ((*md->packages)[0]->string, "core") == 0)
        return true;
      if (strcmp ((*md->packages)[0]->string, "std") == 0)
        return true;
      if (strcmp ((*md->packages)[0]->string, "gcc") == 0)
        return true;
    }
  else if (md->id && md->packages == NULL)
    {
      if (strcmp (md->id->string, "object") == 0)
        return true;
      if (strcmp (md->id->string, "__entrypoint") == 0)
        return true;
    }

  return false;
}

static void
write_one_dep(char const* fn, OutBuffer* ob)
{
  ob->writestring ("  ");
  ob->writestring (fn);
  ob->writestring ("\\\n");
}

static void
deps_write (Module *m)
{
  OutBuffer *ob = global.params.makeDeps;

  // Write out object name.
  FileName *fn = m->objfile->name;
  ob->writestring (fn->str);
  ob->writestring (":");

  StringTable dependencies;
  dependencies._init();

  Modules to_explore;
  to_explore.push(m);
  while (to_explore.dim)
  {
    Module* depmod = to_explore.pop();

    if (global.params.makeDepsStyle == 2)
      if (is_system_module(depmod))
        continue;

    const char* str = depmod->srcfile->name->str;

    if (!dependencies.insert(str, strlen(str)))
      continue;

    for (size_t i = 0; i < depmod->aimports.dim; i++)
      to_explore.push(depmod->aimports[i]);

    write_one_dep(str, ob);
  }

  ob->writenl();
}

// Array of all global declarations to pass back to the middle-end.
static GTY(()) vec<tree, va_gc> *global_declarations;

void
d_add_global_declaration(tree decl)
{
  vec_safe_push(global_declarations, decl);
}

// Write out globals.
static void
d_write_global_declarations()
{
  if (vec_safe_length (global_declarations) != 0)
    {
      d_finish_compilation (global_declarations->address(),
			    global_declarations->length());
    }
}

void
d_parse_file()
{
  if (global.params.verbose)
    {
      fprintf(global.stdmsg, "binary    %s\n", global.params.argv0);
      fprintf(global.stdmsg, "version   %s\n", global.version);
    }

  // Start the main input file, if the debug writer wants it.
  if (debug_hooks->start_end_main_source_file)
    (*debug_hooks->start_source_file)(0, main_input_filename);

  for (TY ty = (TY) 0; ty < TMAX; ty = (TY) (ty + 1))
    {
      if (Type::basic[ty] && ty != Terror)
	d_nametype(Type::basic[ty]);
    }

  // Create Modules
  Modules modules;
  modules.reserve(num_in_fnames);

  if (!main_input_filename || !main_input_filename[0])
    {
      error("input file name required; cannot use stdin");
      goto had_errors;
    }

  // In this mode, the first file name is supposed to be a duplicate
  // of one of the input files.
  if (fonly_arg)
    {
      if (strcmp(fonly_arg, main_input_filename))
	error("-fonly= argument is different from main input file name");
      if (strcmp(fonly_arg, in_fnames[0]))
	error("-fonly= argument is different from first input file name");
    }

  for (size_t i = 0; i < num_in_fnames; i++)
    {
      char *fname = xstrdup(in_fnames[i]);

      // Strip path
      const char *path = FileName::name(fname);
      const char *ext = FileName::ext(path);
      char *name;
      size_t pathlen;

      if (ext)
	{
	  // Skip onto '.'
	  ext--;
	  gcc_assert(*ext == '.');
	  pathlen = (ext - path);
	  name = (char *) xmalloc(pathlen + 1);
	  memcpy(name, path, pathlen);
	  // Strip extension
	  name[pathlen] = '\0';

	  if (name[0] == '\0'
	      || strcmp(name, "..") == 0
	      || strcmp(name, ".") == 0)
	    {
	      error("invalid file name '%s'", fname);
	      goto had_errors;
	    }
	}
      else
	{
	  pathlen = strlen(path);
	  name = (char *) xmalloc(pathlen);
	  memcpy(name, path, pathlen);
	  name[pathlen] = '\0';

	  if (name[0] == '\0')
	    {
	      error("invalid file name '%s'", fname);
	      goto had_errors;
	    }
	}

      // At this point, name is the D source file name stripped of
      // its path and extension.
      Identifier *id = Identifier::idPool(name);
      Module *m = new Module(fname, id, global.params.doDocComments,
			     global.params.doHdrGeneration);
      modules.push(m);

      if (!strcmp(in_fnames[i], main_input_filename))
	output_module = m;
    }

  // Current_module shouldn't have any implications before genobjfile...
  // but it does.  We need to know what module in which to insert
  // TemplateInstances during the semantic pass.  In order for
  // -femit-templates to work, template instances must be emitted
  // in every translation unit.  To do this, the TemplateInstaceS have to
  // have toObjFile called in the module being compiled.
  // TemplateInstance puts itself somwhere during ::semantic, thus it has
  // to know the current module.

  gcc_assert(output_module);

  // Read files
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];
      m->read(Loc());
    }

  // Parse files
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf(global.stdmsg, "parse     %s\n", m->toChars());

      if (!Module::rootModule)
	Module::rootModule = m;

      m->importedFrom = m;
      m->parse();
      Target::loadModule(m);

      if (m->isDocFile)
	{
	  gendocfile(m);
	  // Remove m from list of modules
	  modules.remove(i);
	  i--;
	}
    }

  if (global.errors)
    goto had_errors;

  if (global.params.doHdrGeneration)
    {
      /* Generate 'header' import files.
       * Since 'header' import files must be independent of command
       * line switches and what else is imported, they are generated
       * before any semantic analysis.
       */
      for (size_t i = 0; i < modules.dim; i++)
	{
	  Module *m = modules[i];
	  if (fonly_arg && m != output_module)
	    continue;

	  if (global.params.verbose)
	    fprintf(global.stdmsg, "import    %s\n", m->toChars());

	  genhdrfile(m);
	}
    }

  if (global.errors)
    goto had_errors;

  // Load all unconditional imports for better symbol resolving
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf(global.stdmsg, "importall %s\n", m->toChars());

      m->importAll(NULL);
    }

  if (global.errors)
    goto had_errors;

  // Do semantic analysis
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf(global.stdmsg, "semantic  %s\n", m->toChars());

      m->semantic();
    }

  if (global.errors)
    goto had_errors;

  // Do deferred semantic analysis
  Module::dprogress = 1;
  Module::runDeferredSemantic();

  if (Module::deferred.dim)
    {
      for (size_t i = 0; i < Module::deferred.dim; i++)
	{
	  Dsymbol *sd = Module::deferred[i];
	  sd->error("unable to resolve forward reference in definition");
	}
      goto had_errors;
    }

  // Process all built-in modules or functions now for CTFE.
  while (builtin_modules.dim != 0)
    {
      Module *m = builtin_modules.pop();
      d_maybe_set_builtin(m);
    }

  // Do pass 2 semantic analysis
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf(global.stdmsg, "semantic2 %s\n", m->toChars());

      m->semantic2();
    }

  if (global.errors)
    goto had_errors;

  // Do pass 3 semantic analysis
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf(global.stdmsg, "semantic3 %s\n", m->toChars());

      m->semantic3();
    }

  Module::runDeferredSemantic3();

  // Check again, incase semantic3 pass loaded any more modules.
  while (builtin_modules.dim != 0)
    {
      Module *m = builtin_modules.pop();
      d_maybe_set_builtin(m);
    }

  // Do not attempt to generate output files if errors or warnings occurred
  if (global.errors || global.warnings)
    goto had_errors;

  if (global.params.moduleDeps)
    {
      OutBuffer *ob = global.params.moduleDeps;

      if (global.params.moduleDepsFile)
	{
	  File deps(global.params.moduleDepsFile);
	  deps.setbuffer((void *) ob->data, ob->offset);
	  deps.ref = 1;
	  writeFile(Loc(), &deps);
	}
      else
	fprintf(global.stdmsg, "%.*s", (int) ob->offset, (char *) ob->data);
    }

  if (global.params.makeDeps)
    {
      for (size_t i = 0; i < modules.dim; i++)
	{
	  Module *m = modules[i];
	  deps_write(m);
	}

      OutBuffer *ob = global.params.makeDeps;
      if (global.params.makeDepsFile)
	{
	  File deps(global.params.makeDepsFile);
	  deps.setbuffer((void *) ob->data, ob->offset);
	  deps.ref = 1;
	  writeFile(Loc(), &deps);
	}
      else
	fprintf(global.stdmsg, "%.*s", (int) ob->offset, (char *) ob->data);
    }

  if (fonly_arg)
    output_modules.push(output_module);
  else
    output_modules.append(&modules);

  // Generate output files
  if (global.params.doJsonGeneration)
    {
      OutBuffer buf;
      json_generate(&buf, &modules);

      // Write buf to file
      const char *name = global.params.jsonfilename;

      if (name && name[0] == '-' && name[1] == 0)
	{
	  size_t n = fwrite(buf.data, 1, buf.offset, global.stdmsg);
	  gcc_assert(n == buf.offset);
	}
      else
	{
	  const char *jsonfilename;
	  File *jsonfile;

	  if (name && *name)
	    jsonfilename = FileName::defaultExt(name, global.json_ext);
	  else
	    {
	      // Generate json file name from first obj name
	      const char *n = (*global.params.objfiles)[0];
	      n = FileName::name(n);
	      jsonfilename = FileName::forceExt(n, global.json_ext);
	    }

	  ensurePathToNameExists(Loc(), jsonfilename);
	  jsonfile = new File(jsonfilename);
	  jsonfile->setbuffer(buf.data, buf.offset);
	  jsonfile->ref = 1;
	  writeFile(Loc(), jsonfile);
	}
    }

  if (global.params.doDocComments && !global.errors && !errorcount)
    {
      for (size_t i = 0; i < modules.dim; i++)
	{
	  Module *m = modules[i];
	  gendocfile(m);
	}
    }

  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];
      if (fonly_arg && m != output_module)
	continue;

      if (global.params.verbose)
	fprintf(global.stdmsg, "code      %s\n", m->toChars());

      if (!flag_syntax_only)
	{
	  if ((entrypoint != NULL) && (m == rootmodule))
	    entrypoint->genobjfile(false);

	  m->genobjfile(false);
	}
    }

  // And end the main input file, if the debug writer wants it.
  if (debug_hooks->start_end_main_source_file)
    (*debug_hooks->end_source_file)(0);

 had_errors:
  // Add D frontend error count to GCC error count to to exit with error status
  errorcount += (global.errors + global.warnings);

  d_finish_module();

  output_module = NULL;
}

static tree
d_type_for_mode(machine_mode mode, int unsignedp)
{
  if (mode == QImode)
    return unsignedp ? ubyte_type_node : byte_type_node;

  if (mode == HImode)
    return unsignedp ? ushort_type_node : short_type_node;

  if (mode == SImode)
    return unsignedp ? uint_type_node : int_type_node;

  if (mode == DImode)
    return unsignedp ? ulong_type_node : long_type_node;

  if (mode == TYPE_MODE(cent_type_node))
    return unsignedp ? ucent_type_node : cent_type_node;

  if (mode == TYPE_MODE(float_type_node))
    return float_type_node;

  if (mode == TYPE_MODE(double_type_node))
    return double_type_node;

  if (mode == TYPE_MODE(long_double_type_node))
    return long_double_type_node;

  if (mode == TYPE_MODE(build_pointer_type(char8_type_node)))
    return build_pointer_type(char8_type_node);

  if (mode == TYPE_MODE(build_pointer_type(int_type_node)))
    return build_pointer_type(int_type_node);

  if (COMPLEX_MODE_P(mode))
    {
      machine_mode inner_mode;
      tree inner_type;

      if (mode == TYPE_MODE(complex_float_type_node))
	return complex_float_type_node;
      if (mode == TYPE_MODE(complex_double_type_node))
	return complex_double_type_node;
      if (mode == TYPE_MODE(complex_long_double_type_node))
	return complex_long_double_type_node;

      inner_mode = (machine_mode) GET_MODE_INNER(mode);
      inner_type = d_type_for_mode(inner_mode, unsignedp);
      if (inner_type != NULL_TREE)
	return build_complex_type(inner_type);
    }
  else if (VECTOR_MODE_P(mode))
    {
      machine_mode inner_mode = (machine_mode) GET_MODE_INNER(mode);
      tree inner_type = d_type_for_mode(inner_mode, unsignedp);
      if (inner_type != NULL_TREE)
	return build_vector_type_for_mode(inner_type, mode);
    }

  return 0;
}

static tree
d_type_for_size(unsigned bits, int unsignedp)
{
  if (bits <= TYPE_PRECISION(byte_type_node))
    return unsignedp ? ubyte_type_node : byte_type_node;

  if (bits <= TYPE_PRECISION(short_type_node))
    return unsignedp ? ushort_type_node : short_type_node;

  if (bits <= TYPE_PRECISION(int_type_node))
    return unsignedp ? uint_type_node : int_type_node;

  if (bits <= TYPE_PRECISION(long_type_node))
    return unsignedp ? ulong_type_node : long_type_node;

  if (bits <= TYPE_PRECISION(cent_type_node))
    return unsignedp ? ucent_type_node : cent_type_node;

  return 0;
}

static tree
d_signed_or_unsigned_type(int unsignedp, tree type)
{
  if (!INTEGRAL_TYPE_P(type)
      || TYPE_UNSIGNED(type) == (unsigned) unsignedp)
    return type;

  if (TYPE_PRECISION(type) == TYPE_PRECISION(cent_type_node))
    return unsignedp ? ucent_type_node : cent_type_node;

  if (TYPE_PRECISION(type) == TYPE_PRECISION(long_type_node))
    return unsignedp ? ulong_type_node : long_type_node;

  if (TYPE_PRECISION(type) == TYPE_PRECISION(int_type_node))
    return unsignedp ? uint_type_node : int_type_node;

  if (TYPE_PRECISION(type) == TYPE_PRECISION(short_type_node))
    return unsignedp ? ushort_type_node : short_type_node;

  if (TYPE_PRECISION(type) == TYPE_PRECISION(byte_type_node))
    return unsignedp ? ubyte_type_node : byte_type_node;

  return type;
}

tree
d_unsigned_type(tree type)
{
  return d_signed_or_unsigned_type(1, type);
}

tree
d_signed_type(tree type)
{
  return d_signed_or_unsigned_type(0, type);
}

// All promotions for variable arguments are handled by the frontend.

static tree
d_type_promotes_to(tree type)
{
  return type;
}


// This is called by the backend before parsing.  Need to make this do
// something or lang_hooks.clear_binding_stack (lhd_clear_binding_stack)
// loops forever.

static bool
d_global_bindings_p()
{
  if (current_binding_level == global_binding_level)
    return true;

  return !global_binding_level;
}

tree
d_pushdecl (tree decl)
{
  // Should only be for variables OR, should also use TRANSLATION_UNIT for toplevel...
  // current_function_decl could be NULL_TREE (top level)...
  if (DECL_CONTEXT (decl) == NULL_TREE)
    DECL_CONTEXT (decl) = current_function_decl;

  // Put decls on list in reverse order.
  TREE_CHAIN (decl) = current_binding_level->names;
  current_binding_level->names = decl;

  return decl;
}

// Return the list of declarations of the current level.

static tree
d_getdecls()
{
  if (current_binding_level)
    return current_binding_level->names;

  return NULL_TREE;
}


// Get the alias set corresponding to a type or expression.
// Return -1 if we don't do anything special.

static alias_set_type
d_get_alias_set (tree t)
{
  // Permit type-punning when accessing a union, provided the access
  // is directly through the union.
  for (tree u = t; handled_component_p (u); u = TREE_OPERAND (u, 0))
    {
      if (TREE_CODE (u) == COMPONENT_REF
	  && TREE_CODE (TREE_TYPE (TREE_OPERAND (u, 0))) == UNION_TYPE)
	return 0;
    }

  // That's all the expressions we handle.
  if (!TYPE_P (t))
    return get_alias_set (TREE_TYPE (t));

  // For now in D, assume everything aliases everything else,
  // until we define some solid rules.
  return 0;
}

static int
d_types_compatible_p (tree t1, tree t2)
{
  /* Is compatible if types are equivalent */
  if (TYPE_MAIN_VARIANT (t1) == TYPE_MAIN_VARIANT (t2))
    return 1;

  /* Is compatible if aggregates are same type or share the same
     attributes. The frontend should have already ensured that types
     aren't wildly different anyway... */
  if (AGGREGATE_TYPE_P (t1) && AGGREGATE_TYPE_P (t2)
      && TREE_CODE (t1) == TREE_CODE (t2))
    {
      if (TREE_CODE (t1) == ARRAY_TYPE)
	return (TREE_TYPE (t1) == TREE_TYPE (t2));

      return (TYPE_ATTRIBUTES (t1) == TYPE_ATTRIBUTES (t2));
    }
  /* else */
  return 0;
}

static void
d_finish_incomplete_decl (tree decl)
{
  if (VAR_P (decl))
    {
      /* D allows zero-length declarations.  Such a declaration ends up with
	 DECL_SIZE (t) == NULL_TREE which is what the backend function
	 assembler_variable checks.  This could change in later versions...

	 Maybe all of these variables should be aliased to one symbol... */
      if (DECL_SIZE (decl) == 0)
	{
	  DECL_SIZE (decl) = bitsize_zero_node;
	  DECL_SIZE_UNIT (decl) = size_zero_node;
	}
    }
}


// Return the true debug type for TYPE.

static classify_record
d_classify_record (tree type)
{
  Type *dtype = TYPE_LANG_FRONTEND (type);

  if (dtype && dtype->ty == Tclass)
    {
      TypeClass *tclass = (TypeClass *) dtype;

      // extern(C++) interfaces get emitted as classes.
      if (tclass->sym->isInterfaceDeclaration()
	  && !tclass->sym->isCPPinterface())
	return RECORD_IS_INTERFACE;

      return RECORD_IS_CLASS;
    }

  return RECORD_IS_STRUCT;
}


struct lang_type *
build_lang_type (Type *t)
{
  unsigned sz = sizeof (lang_type);
  struct lang_type *lt = ggc_alloc_cleared_lang_type (sz);
  lt->type = t;
  return lt;
}

struct lang_decl *
build_lang_decl (Declaration *d)
{
  unsigned sz = sizeof (lang_decl);
  struct lang_decl *ld = ggc_alloc_cleared_lang_decl (sz);
  ld->decl = d;
  return ld;
}


// This preserves trees we create from the garbage collector.
static GTY(()) tree d_keep_list = NULL_TREE;

void
d_keep (tree t)
{
  d_keep_list = tree_cons (NULL_TREE, t, d_keep_list);
}

static GTY(()) tree d_eh_personality_decl;

/* Return the GDC personality function decl.  */
static tree
d_eh_personality()
{
  if (!d_eh_personality_decl)
    {
      d_eh_personality_decl
	= build_personality_function ("gdc");
    }
  return d_eh_personality_decl;
}

static tree
d_build_eh_type_type (tree type)
{
  Type *dtype = TYPE_LANG_FRONTEND (type);
  Symbol *sym;

  if (dtype)
    dtype = dtype->toBasetype();

  gcc_assert (dtype && dtype->ty == Tclass);
  sym = ((TypeClass *) dtype)->sym->toSymbol();

  return convert (ptr_type_node, build_address (sym->Stree));
}

struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

#include "gt-d-d-lang.h"
#include "gtype-d.h"
