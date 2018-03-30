/* d-lang.cc -- Language-dependent hooks for D.
   Copyright (C) 2006-2018 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/aggregate.h"
#include "dfrontend/cond.h"
#include "dfrontend/hdrgen.h"
#include "dfrontend/json.h"
#include "dfrontend/module.h"
#include "dfrontend/mtype.h"
#include "dfrontend/target.h"

#include "options.h"
#include "d-system.h"
#include "varasm.h"
#include "output.h"
#include "debug.h"

#include "d-tree.h"
#include "d-frontend.h"
#include "id.h"

/* Array of D frontend type/decl nodes.  */
tree d_global_trees[DTI_MAX];

/* Options handled by the compiler that are separate from the frontend.  */
struct d_option_data
{
  const char *fonly;                /* -fonly=<arg>  */
  const char *multilib;             /* -imultilib <dir>  */
  const char *prefix;               /* -iprefix <dir>  */

  bool deps;                        /* -M  */
  bool deps_skip_system;            /* -MM  */
  const char *deps_filename;        /* -M[M]D  */
  const char *deps_filename_user;   /* -MF <arg>  */
  OutBuffer *deps_target;           /* -M[QT] <arg> */
  bool deps_phony;                  /* -MP  */

  bool stdinc;                      /* -nostdinc  */
}
d_option;

/* List of modules being compiled.  */
static Modules builtin_modules;

/* Module where `C main' is defined, compiled in if needed.  */
static Module *entrypoint_module = NULL;
static Module *entrypoint_root_module = NULL;

/* The current and global binding level in effect.  */
struct binding_level *current_binding_level;
struct binding_level *global_binding_level;

/* The context to be used for global declarations.  */
static GTY(()) tree global_context;

/* Array of all global declarations to pass back to the middle-end.  */
static GTY(()) vec<tree, va_gc> *global_declarations;

/* Support for GCC-style command-line make dependency generation.
   Adds TARGET to the make dependencies target buffer.
   QUOTED is true if the string should be quoted.  */

static void
deps_add_target (const char *target, bool quoted)
{
  if (!d_option.deps_target)
    d_option.deps_target = new OutBuffer ();
  else
    d_option.deps_target->writeByte (' ');

  d_option.deps_target->reserve (strlen (target));

  if (!quoted)
    {
      d_option.deps_target->writestring (target);
      return;
    }

  /* Quote characters in target which are significant to Make.  */
  for (const char *p = target; *p != '\0'; p++)
    {
      switch (*p)
        {
        case ' ':
        case '\t':
          for (const char *q = p - 1; target <= q && *q == '\\';  q--)
	    d_option.deps_target->writeByte ('\\');
	  d_option.deps_target->writeByte ('\\');
          break;

        case '$':
	  d_option.deps_target->writeByte ('$');
          break;

        case '#':
	  d_option.deps_target->writeByte ('\\');
          break;

        default:
          break;
        }

      d_option.deps_target->writeByte (*p);
    }
}

/* Write out all dependencies of a given MODULE to the specified BUFFER.
   COLMAX is the number of columns to word-wrap at (0 means don't wrap).  */

static void
deps_write (Module *module, OutBuffer *buffer, unsigned colmax = 72)
{
  static StringTable *dependencies = NULL;

  if (dependencies)
    dependencies->reset ();
  else
    {
      dependencies = new StringTable ();
      dependencies->_init ();
    }

  Modules modlist;
  modlist.push (module);

  Modules phonylist;

  const char *str;
  unsigned size;
  unsigned column = 0;

  /* Write out make target module name.  */
  if (d_option.deps_target)
    {
      size = d_option.deps_target->offset;
      str = d_option.deps_target->extractString ();
    }
  else
    {
      str = module->objfile->name->str;
      size = strlen (str);
    }

  buffer->writestring (str);
  column = size;
  buffer->writestring (":");
  column++;

  /* Write out all make dependencies.  */
  while (modlist.dim > 0)
  {
    Module *depmod = modlist.pop ();

    str = depmod->srcfile->name->str;
    size = strlen (str);

    /* Skip dependencies that have already been written.  */
    if (!dependencies->insert (str, size, NULL))
      continue;

    column += size;

    if (colmax && column > colmax)
      {
	buffer->writestring (" \\\n ");
	column = size + 1;
      }
    else
      {
	buffer->writestring (" ");
	column++;
      }

    buffer->writestring (str);

    /* Add to list of phony targets if is not being compile.  */
    if (d_option.deps_phony && !depmod->isRoot ())
      phonylist.push (depmod);

    /* Search all imports of the written dependency.  */
    for (size_t i = 0; i < depmod->aimports.dim; i++)
      {
	Module *m = depmod->aimports[i];

	/* Ignore compiler-generated modules.  */
	if (m->ident == Identifier::idPool ("__entrypoint")
	    && m->parent == NULL)
	  continue;

	/* Don't search system installed modules, this includes
	   object, core.*, std.*, and gcc.* packages.  */
	if (d_option.deps_skip_system)
	  {
	    if (m->ident == Identifier::idPool ("object") && m->parent == NULL)
	      continue;

	    if (m->md && m->md->packages)
	      {
		Identifier *package = (*m->md->packages)[0];

		if (package == Identifier::idPool ("core")
		    || package == Identifier::idPool ("std")
		    || package == Identifier::idPool ("gcc"))
		  continue;
	      }
	  }

	modlist.push (m);
      }
  }

  buffer->writenl ();

  /* Write out all phony targets.  */
  for (size_t i = 0; i < phonylist.dim; i++)
    {
      Module *m = phonylist[i];

      buffer->writenl ();
      buffer->writestring (m->srcfile->name->str);
      buffer->writestring (":\n");
    }
}

/* Implements the lang_hooks.init_options routine for language D.
   This initializes the global state for the D frontend before calling
   the option handlers.  */

static void
d_init_options (unsigned int, cl_decoded_option *decoded_options)
{
  /* Set default values.  */
  global._init ();

  global.compiler.vendor = lang_hooks.name;

  global.params.argv0 = xstrdup (decoded_options[0].arg);
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
  global.params.hdrStripPlainFunctions = true;
  global.params.betterC = false;
  global.params.allInst = false;

  global.params.linkswitches = new Strings ();
  global.params.libfiles = new Strings ();
  global.params.objfiles = new Strings ();
  global.params.ddocfiles = new Strings ();

  global.params.imppath = new Strings ();
  global.params.fileImppath = new Strings ();
  global.params.modFileAliasStrings = new Strings ();

  /* Extra GDC-specific options.  */
  d_option.fonly = NULL;
  d_option.multilib = NULL;
  d_option.prefix = NULL;
  d_option.deps = false;
  d_option.deps_skip_system = false;
  d_option.deps_filename = NULL;
  d_option.deps_filename_user = NULL;
  d_option.deps_target = NULL;
  d_option.deps_phony = false;
  d_option.stdinc = true;
}

/* Implements the lang_hooks.init_options_struct routine for language D.
   Initializes the options structure OPTS.  */

static void
d_init_options_struct (gcc_options *opts)
{
  /* GCC options.  */
  opts->x_flag_exceptions = 1;

  /* Avoid range issues for complex multiply and divide.  */
  opts->x_flag_complex_method = 2;

  /* Unlike C, there is no global 'errno' variable.  */
  opts->x_flag_errno_math = 0;
  opts->frontend_set_flag_errno_math = true;

  /* Keep in sync with existing -fbounds-check flag.  */
  opts->x_flag_bounds_check = global.params.useArrayBounds;

  /* D says that signed overflow is precisely defined.  */
  opts->x_flag_wrapv = 1;
}

/* Implements the lang_hooks.lang_mask routine for language D.
   Returns language mask for option parsing.  */

static unsigned int
d_option_lang_mask (void)
{
  return CL_D;
}

/* Implements the lang_hooks.init routine for language D.  */

static bool
d_init (void)
{
  Type::_init ();
  Id::initialize ();
  Module::_init ();
  Expression::_init ();
  Objc::_init ();

  /* Backend init.  */
  global_binding_level = ggc_alloc_cleared_binding_level ();
  current_binding_level = global_binding_level;

  /* This allows the code in d-builtins.cc to not have to worry about
     converting (C signed char *) to (D char *) for string arguments of
     built-in functions.  The parameter (signed_char = false) specifies
     whether char is signed.  */
  build_common_tree_nodes (false, false);

  d_init_builtins ();

  if (flag_exceptions)
    using_eh_for_cleanups ();

  if (!supports_one_only ())
    flag_weak = 0;

  /* This is the C main, not the D main.  */
  main_identifier_node = get_identifier ("main");

  Target::_init ();
  d_init_versions ();

  /* Insert all library-configured identifiers and import paths.  */
  add_import_paths (d_option.prefix, d_option.multilib, d_option.stdinc);

  return 1;
}

/* Implements the lang_hooks.init_ts routine for language D.  */

static void
d_init_ts (void)
{
  MARK_TS_TYPED (IASM_EXPR);
  MARK_TS_TYPED (FLOAT_MOD_EXPR);
  MARK_TS_TYPED (UNSIGNED_RSHIFT_EXPR);
}

/* Implements the lang_hooks.handle_option routine for language D.
   Handles D specific options.  Return false if we didn't do anything.  */

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
    case OPT_fall_instantiations:
      global.params.allInst = value;
      break;

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
	  int level = integral_argument (arg);
	  if (level != -1)
	    {
	      DebugCondition::setGlobalLevel (level);
	      break;
	    }
	}

      if (Identifier::isValidIdentifier (CONST_CAST (char *, arg)))
	{
	  DebugCondition::addGlobalIdent (arg);
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

    case OPT_fdump_d_original:
      global.params.vcg_ast = value;
      break;

    case OPT_fignore_unknown_pragmas:
      global.params.ignoreUnsupportedPragmas = value;
      break;

    case OPT_finvariants:
      global.params.useInvariants = value;
      break;

    case OPT_fmodule_file_:
      global.params.modFileAliasStrings->push (arg);
      if (!strchr (arg, '='))
	error ("bad argument for -fmodule-file");
      break;

    case OPT_fmoduleinfo:
      global.params.betterC = !value;
      break;

    case OPT_fonly_:
      d_option.fonly = arg;
      break;

    case OPT_fpostconditions:
      global.params.useOut = value;
      break;

    case OPT_fpreconditions:
      global.params.useIn = value;
      break;

    case OPT_fproperty:
      global.params.enforcePropertySyntax = value;
      break;

    case OPT_frelease:
      global.params.release = value;
      break;

    case OPT_fswitch_errors:
      global.params.useSwitchError = value;
      break;

    case OPT_ftransition_all:
      global.params.vtls = value;
      global.params.vfield = value;
      global.params.vcomplex = value;
      break;

    case OPT_ftransition_checkimports:
      global.params.check10378 = value;
      break;

    case OPT_ftransition_complex:
      global.params.vcomplex = value;
      break;

    case OPT_ftransition_dip1000:
      global.params.vsafe = value;
      global.params.useDIP25 = value;
      break;

    case OPT_ftransition_dip25:
      global.params.useDIP25 = value;
      break;

    case OPT_ftransition_field:
      global.params.vfield = value;
      break;

    case OPT_ftransition_import:
      global.params.bug10378 = value;
      break;

    case OPT_ftransition_nogc:
      global.params.vgc = value;
      break;

    case OPT_ftransition_tls:
      global.params.vtls = value;
      break;

    case OPT_funittest:
      global.params.useUnitTests = value;
      break;

    case OPT_fversion_:
      if (ISDIGIT (arg[0]))
	{
	  int level = integral_argument (arg);
	  if (level != -1)
	    {
	      VersionCondition::setGlobalLevel (level);
	      break;
	    }
	}

      if (Identifier::isValidIdentifier (CONST_CAST (char *, arg)))
	{
	  VersionCondition::addGlobalIdent (arg);
	  break;
	}

      error ("bad argument for -fversion '%s'", arg);
      break;

    case OPT_H:
      global.params.doHdrGeneration = true;
      break;

    case OPT_Hd:
      global.params.doHdrGeneration = true;
      global.params.hdrdir = arg;
      break;

    case OPT_Hf:
      global.params.doHdrGeneration = true;
      global.params.hdrname = arg;
      break;

    case OPT_imultilib:
      d_option.multilib = arg;
      break;

    case OPT_iprefix:
      d_option.prefix = arg;
      break;

    case OPT_I:
      global.params.imppath->push (arg);
      break;

    case OPT_J:
      global.params.fileImppath->push (arg);
      break;

    case OPT_MM:
      d_option.deps_skip_system = true;
      /* fall through */

    case OPT_M:
      d_option.deps = true;
      break;

    case OPT_MMD:
      d_option.deps_skip_system = true;
      /* fall through */

    case OPT_MD:
      d_option.deps = true;
      d_option.deps_filename = arg;
      break;

    case OPT_MF:
      /* If specified multiple times, last one wins.  */
      d_option.deps_filename_user = arg;
      break;

    case OPT_MP:
      d_option.deps_phony = true;
      break;

    case OPT_MQ:
      deps_add_target (arg, true);
      break;

    case OPT_MT:
      deps_add_target (arg, false);
      break;

    case OPT_nostdinc:
      d_option.stdinc = false;
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

    case OPT_Wspeculative:
      if (value)
	global.params.showGaggedErrors = 1;
      break;

    case OPT_Xf:
      global.params.jsonfilename = arg;
      /* fall through */

    case OPT_X:
      global.params.doJsonGeneration = true;
      break;

    default:
      break;
    }

  D_handle_option_auto (&global_options, &global_options_set,
			scode, arg, value,
			d_option_lang_mask (), kind,
			loc, handlers, global_dc);

  return result;
}

/* Implements the lang_hooks.post_options routine for language D.
   Deal with any options that imply the turning on/off of features.
   FN is the main input filename passed on the command line.  */

static bool
d_post_options (const char ** fn)
{
  /* Verify the input file name.  */
  const char *filename = *fn;
  if (!filename || strcmp (filename, "-") == 0)
    filename = "";

  /* The front end considers the first input file to be the main one.  */
  *fn = filename;

  /* Release mode doesn't turn off bounds checking for safe functions.  */
  if (global.params.useArrayBounds == BOUNDSCHECKdefault)
    {
      global.params.useArrayBounds = global.params.release
	? BOUNDSCHECKsafeonly : BOUNDSCHECKon;
      flag_bounds_check = !global.params.release;
    }

  if (global.params.release)
    {
      if (!global_options_set.x_flag_invariants)
	global.params.useInvariants = false;

      if (!global_options_set.x_flag_preconditions)
	global.params.useIn = false;

      if (!global_options_set.x_flag_postconditions)
	global.params.useOut = false;

      if (!global_options_set.x_flag_assert)
	global.params.useAssert = false;

      if (!global_options_set.x_flag_switch_errors)
	global.params.useSwitchError = false;
    }

  /* Error about use of deprecated features.  */
  if (global.params.useDeprecated == 2 && global.params.warnings == 1)
    global.params.useDeprecated = 0;

  /* Make -fmax-errors visible to frontend's diagnostic machinery.  */
  if (global_options_set.x_flag_max_errors)
    global.errorLimit = flag_max_errors;

  if (flag_excess_precision_cmdline == EXCESS_PRECISION_DEFAULT)
    flag_excess_precision_cmdline = EXCESS_PRECISION_STANDARD;

  if (global.params.useUnitTests)
    global.params.useAssert = true;

  global.params.symdebug = write_symbols != NO_DEBUG;
  global.params.useInline = flag_inline_functions;

  if (global.params.useInline)
    global.params.hdrStripPlainFunctions = false;

  global.params.obj = !flag_syntax_only;

  /* Has no effect yet.  */
  global.params.pic = flag_pic != 0;

  if (warn_return_type == -1)
    warn_return_type = 0;

  return false;
}

/* Return TRUE if an operand OP of a given TYPE being copied has no data.
   The middle-end does a similar check with zero sized types.  */

static bool
empty_modify_p (tree type, tree op)
{
  tree_code code = TREE_CODE (op);
  switch (code)
    {
    case COMPOUND_EXPR:
      return empty_modify_p (type, TREE_OPERAND (op, 1));

    case CONSTRUCTOR:
      /* Non-empty construcors are valid.  */
      if (CONSTRUCTOR_NELTS (op) != 0 || TREE_CLOBBER_P (op))
	return false;
      break;

    case CALL_EXPR:
      /* Leave nrvo alone because it isn't a copy.  */
      if (CALL_EXPR_RETURN_SLOT_OPT (op))
	return false;
      break;

    default:
      /* If the operand doesn't have a simple form.  */
      if (!is_gimple_lvalue (op) && !INDIRECT_REF_P (op))
	return false;
      break;
    }

  return empty_aggregate_p (type);
}

/* Implements the lang_hooks.gimplify_expr routine for language D.
   Do gimplification of D specific expression trees in EXPR_P.  */

int
d_gimplify_expr (tree *expr_p, gimple_seq *pre_p,
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

      if (!error_operand_p (op0) && !error_operand_p (op1)
	  && (AGGREGATE_TYPE_P (TREE_TYPE (op0))
	      || AGGREGATE_TYPE_P (TREE_TYPE (op1)))
	  && !useless_type_conversion_p (TREE_TYPE (op1), TREE_TYPE (op0)))
	{
	  /* If the back end isn't clever enough to know that the lhs and rhs
	     types are the same, add an explicit conversion.  */
	  TREE_OPERAND (*expr_p, 1) = build1 (VIEW_CONVERT_EXPR,
					      TREE_TYPE (op0), op1);
	  ret = GS_OK;
	}
      else if (empty_modify_p (TREE_TYPE (op0), op1))
	{
	  /* Remove any copies of empty aggregates.  Also drop volatile
	     loads on the RHS to avoid infinite recursion from
	     gimplify_expr trying to load the value.  */
	  gimplify_expr (&TREE_OPERAND (*expr_p, 0), pre_p, post_p,
			 is_gimple_lvalue, fb_lvalue);
	  if (TREE_SIDE_EFFECTS (op1))
	    {
	      if (TREE_THIS_VOLATILE (op1)
		  && (REFERENCE_CLASS_P (op1) || DECL_P (op1)))
		op1 = build_fold_addr_expr (op1);

	      gimplify_and_add (op1, pre_p);
	    }
	  *expr_p = TREE_OPERAND (*expr_p, 0);
	  ret = GS_OK;
	}
      break;

    case ADDR_EXPR:
      op0 = TREE_OPERAND (*expr_p, 0);
      /* Constructors are not lvalues, so make them one.  */
      if (TREE_CODE (op0) == CONSTRUCTOR)
	{
	  TREE_OPERAND (*expr_p, 0) = force_target_expr (op0);
	  ret = GS_OK;
	}
      break;

    case CALL_EXPR:
      if (CALL_EXPR_ARGS_ORDERED (*expr_p))
	{
	  /* Strictly evaluate all arguments from left to right.  */
	  int nargs = call_expr_nargs (*expr_p);
	  location_t loc = EXPR_LOC_OR_LOC (*expr_p, input_location);

	  /* No need to enforce evaluation order if only one argument.  */
	  if (nargs < 2)
	    break;

	  /* Or if all arguments are already free of side-effects.  */
	  bool has_side_effects = false;
	  for (int i = 0; i < nargs; i++)
	    {
	      if (TREE_SIDE_EFFECTS (CALL_EXPR_ARG (*expr_p, i)))
		{
		  has_side_effects = true;
		  break;
		}
	    }

	  if (!has_side_effects)
	    break;

	  /* Leave the last argument for gimplify_call_expr.  */
	  for (int i = 0; i < nargs - 1; i++)
	    {
	      tree new_arg = CALL_EXPR_ARG (*expr_p, i);

	      /* If argument has a side-effect, gimplify_arg will handle it.  */
	      if (gimplify_arg (&new_arg, pre_p, loc) == GS_ERROR)
		ret = GS_ERROR;

	      /* Even if an argument itself doesn't have any side-effects, it
		 might be altered by another argument in the list.  */
	      if (new_arg == CALL_EXPR_ARG (*expr_p, i)
		  && !really_constant_p (new_arg))
		new_arg = get_formal_tmp_var (new_arg, pre_p);

	      CALL_EXPR_ARG (*expr_p, i) = new_arg;
	    }

	  if (ret != GS_ERROR)
	    ret = GS_OK;
	}
      break;

    case UNSIGNED_RSHIFT_EXPR:
      /* Convert op0 to an unsigned type.  */
      op0 = TREE_OPERAND (*expr_p, 0);
      op1 = TREE_OPERAND (*expr_p, 1);

      type = d_unsigned_type (TREE_TYPE (op0));

      *expr_p = convert (TREE_TYPE (*expr_p),
			 build2 (RSHIFT_EXPR, type, convert (type, op0), op1));
      ret = GS_OK;
      break;

    case FLOAT_MOD_EXPR:
    case IASM_EXPR:
      gcc_unreachable ();

    default:
      break;
    }

  return ret;
}

/* Add the module M to the list of modules that may declare GCC builtins.
   These are scanned after first semantic and before codegen passes.
   See d_maybe_set_builtin() for the implementation.  */

void
d_add_builtin_module (Module *m)
{
  builtin_modules.push (m);
}

/* Record the entrypoint module ENTRY which will be compiled in the current
   compilation.  ROOT is the module scope where this was requested from.  */

void
d_add_entrypoint_module (Module *entry, Module *root)
{
  /* We are emitting this straight to object file.  */
  entrypoint_module = entry;
  entrypoint_root_module = root;
}

// Write out globals.
static void
d_write_global_declarations()
{
  d_finish_ctor_lists ();
  d_finish_compilation (vec_safe_address (global_declarations),
			vec_safe_length (global_declarations));
}

/* Implements the lang_hooks.parse_file routine for language D.  */

void
d_parse_file (void)
{
  if (global.params.verbose)
    {
      fprintf (global.stdmsg, "binary    %s\n", global.params.argv0);
      fprintf (global.stdmsg, "version   %s\n", global.version);

      if (global.params.versionids)
	{
	  fprintf (global.stdmsg, "predefs  ");
	  for (size_t i = 0; i < global.params.versionids->dim; i++)
	    {
	      const char *s = (*global.params.versionids)[i];
	      fprintf (global.stdmsg, " %s", s);
	    }
	  fprintf (global.stdmsg, "\n");
	}
    }

  /* Start the main input file, if the debug writer wants it.  */
  if (debug_hooks->start_end_main_source_file)
    debug_hooks->start_source_file (0, main_input_filename);

  /* Create Module's for all sources we will load.  */
  Modules modules;
  modules.reserve (num_in_fnames);

  /* In this mode, the first file name is supposed to be a duplicate
     of one of the input files.  */
  if (d_option.fonly && strcmp (d_option.fonly, main_input_filename) != 0)
    error ("-fonly= argument is different from first input file name");

  for (size_t i = 0; i < num_in_fnames; i++)
    {
      if (strcmp (in_fnames[i], "-") == 0)
	{
	  /* Handling stdin, generate a unique name for the module.  */
	  obstack buffer;
	  gcc_obstack_init (&buffer);
	  int c;

	  Module *m = Module::create (in_fnames[i],
				      Identifier::generateId ("__stdin"),
				      global.params.doDocComments,
				      global.params.doHdrGeneration);
	  modules.push (m);

	  /* Load the entire contents of stdin into memory.  */
	  while ((c = getc (stdin)) != EOF)
	    obstack_1grow (&buffer, c);

	  if (!obstack_object_size (&buffer))
	    obstack_1grow (&buffer, '\0');

	  /* Overwrite the source file for the module, the one created by
	     Module::create would have a forced a `.d' suffix.  */
	  m->srcfile = File::create ("<stdin>");
	  m->srcfile->len = obstack_object_size (&buffer);
	  m->srcfile->buffer = (unsigned char *) obstack_finish (&buffer);

	  /* Tell the front-end not to free the buffer after parsing.  */
	  m->srcfile->ref = 1;
	}
      else
	{
	  /* Handling a D source file, strip off the path and extension.  */
	  const char *basename = FileName::name (in_fnames[i]);
	  const char *name = FileName::removeExt (basename);

	  Module *m = Module::create (in_fnames[i], Identifier::idPool (name),
				      global.params.doDocComments,
				      global.params.doHdrGeneration);
	  modules.push (m);
	  FileName::free (name);
	}
    }

  /* Read all D source files.  */
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];
      m->read (Loc ());
    }

  /* Parse all D source files.  */
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf (global.stdmsg, "parse     %s\n", m->toChars ());

      if (!Module::rootModule)
	Module::rootModule = m;

      m->importedFrom = m;
      m->parse ();
      Target::loadModule (m);

      if (m->isDocFile)
	{
	  gendocfile (m);
	  /* Remove M from list of modules.  */
	  modules.remove (i);
	  i--;
	}
    }

  if (global.errors)
    goto had_errors;

  if (global.params.doHdrGeneration)
    {
      /* Generate 'header' import files.  Since 'header' import files must be
	 independent of command line switches and what else is imported, they
	 are generated before any semantic analysis.  */
      for (size_t i = 0; i < modules.dim; i++)
	{
	  Module *m = modules[i];
	  if (d_option.fonly && m != Module::rootModule)
	    continue;

	  if (global.params.verbose)
	    fprintf (global.stdmsg, "import    %s\n", m->toChars ());

	  genhdrfile (m);
	}
    }

  if (global.errors)
    goto had_errors;

  /* Load all unconditional imports for better symbol resolving.  */
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf (global.stdmsg, "importall %s\n", m->toChars ());

      m->importAll (NULL);
    }

  if (global.errors)
    goto had_errors;

  /* Do semantic analysis.  */
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf (global.stdmsg, "semantic  %s\n", m->toChars ());

      m->semantic (NULL);
    }

  /* Do deferred semantic analysis.  */
  Module::dprogress = 1;
  Module::runDeferredSemantic ();

  if (Module::deferred.dim)
    {
      for (size_t i = 0; i < Module::deferred.dim; i++)
	{
	  Dsymbol *sd = Module::deferred[i];
	  sd->error ("unable to resolve forward reference in definition");
	}
    }

  /* Process all built-in modules or functions now for CTFE.  */
  while (builtin_modules.dim != 0)
    {
      Module *m = builtin_modules.pop ();
      d_maybe_set_builtin (m);
    }

  /* Do pass 2 semantic analysis.  */
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf (global.stdmsg, "semantic2 %s\n", m->toChars ());

      m->semantic2 (NULL);
    }

  Module::runDeferredSemantic2 ();

  if (global.errors)
    goto had_errors;

  /* Do pass 3 semantic analysis.  */
  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];

      if (global.params.verbose)
	fprintf (global.stdmsg, "semantic3 %s\n", m->toChars ());

      m->semantic3 (NULL);
    }

  Module::runDeferredSemantic3 ();

  /* Check again, incase semantic3 pass loaded any more modules.  */
  while (builtin_modules.dim != 0)
    {
      Module *m = builtin_modules.pop ();
      d_maybe_set_builtin (m);
    }

  /* Do not attempt to generate output files if errors or warnings occurred.  */
  if (global.errors || global.warnings)
    goto had_errors;

  /* Generate output files.  */

  if (Module::rootModule)
    {
      /* Declare the name of the root module as the first global name in order
	 to make the middle-end fully deterministic.  */
      OutBuffer buf;
      mangleToBuffer (Module::rootModule, &buf);
      first_global_object_name = buf.extractString ();
    }

  /* Module dependencies (imports, file, version, debug, lib).  */
  if (global.params.moduleDeps)
    {
      OutBuffer *buf = global.params.moduleDeps;

      if (global.params.moduleDepsFile)
	{
	  File *fdeps = File::create (global.params.moduleDepsFile);
	  fdeps->setbuffer ((void *) buf->data, buf->offset);
	  fdeps->ref = 1;
	  writeFile (Loc (), fdeps);
	}
      else
	fprintf (global.stdmsg, "%.*s", (int) buf->offset, (char *) buf->data);
    }

  /* Make dependencies.  */
  if (d_option.deps)
    {
      OutBuffer buf;

      for (size_t i = 0; i < modules.dim; i++)
	deps_write (modules[i], &buf);

      /* -MF <arg> overrides -M[M]D.  */
      if (d_option.deps_filename_user)
	d_option.deps_filename = d_option.deps_filename_user;

      if (d_option.deps_filename)
	{
	  File *fdeps = File::create (d_option.deps_filename);
	  fdeps->setbuffer ((void *) buf.data, buf.offset);
	  fdeps->ref = 1;
	  writeFile (Loc (), fdeps);
	}
      else
	fprintf (global.stdmsg, "%.*s", (int) buf.offset, (char *) buf.data);
    }

  /* Generate JSON files.  */
  if (global.params.doJsonGeneration)
    {
      OutBuffer buf;
      json_generate (&buf, &modules);

      const char *name = global.params.jsonfilename;

      if (name && (name[0] != '-' || name[1] != '\0'))
	{
	  const char *nameext = FileName::defaultExt (name, global.json_ext);
	  File *fjson = File::create (nameext);
	  fjson->setbuffer ((void *) buf.data, buf.offset);
	  fjson->ref = 1;
	  writeFile (Loc (), fjson);
	}
      else
	fprintf (global.stdmsg, "%.*s", (int) buf.offset, (char *) buf.data);
    }

  /* Generate Ddoc files.  */
  if (global.params.doDocComments && !global.errors && !errorcount)
    {
      for (size_t i = 0; i < modules.dim; i++)
	{
	  Module *m = modules[i];
	  gendocfile (m);
	}
    }

  /* Handle -fdump-d-original.  */
  if (global.params.vcg_ast)
    {
      for (size_t i = 0; i < modules.dim; i++)
	{
	  Module *m = modules[i];
	  OutBuffer buf;
	  buf.doindent = 1;

	  HdrGenState hgs;
	  hgs.fullDump = true;

	  toCBuffer (m, &buf, &hgs);
	  fprintf (global.stdmsg, "%.*s", (int) buf.offset, (char *) buf.data);
	}
    }

  for (size_t i = 0; i < modules.dim; i++)
    {
      Module *m = modules[i];
      if (d_option.fonly && m != Module::rootModule)
	continue;

      if (global.params.verbose)
	fprintf (global.stdmsg, "code      %s\n", m->toChars ());

      if (!flag_syntax_only)
	{
	  if ((entrypoint_module != NULL) && (m == entrypoint_root_module))
	    build_decl_tree (entrypoint_module);

	  build_decl_tree (m);
	}
    }

  /* And end the main input file, if the debug writer wants it.  */
  if (debug_hooks->start_end_main_source_file)
    debug_hooks->end_source_file (0);

 had_errors:
  /* Add the D frontend error count to the GCC error count to correctly
     exit with an error status.  */
  errorcount += (global.errors + global.warnings);
}

/* Implements the lang_hooks.types.type_for_mode routine for language D.  */

static tree
d_type_for_mode (machine_mode mode, int unsignedp)
{
  if (mode == QImode)
    return unsignedp ? ubyte_type_node : byte_type_node;

  if (mode == HImode)
    return unsignedp ? ushort_type_node : short_type_node;

  if (mode == SImode)
    return unsignedp ? uint_type_node : int_type_node;

  if (mode == DImode)
    return unsignedp ? ulong_type_node : long_type_node;

  if (mode == TYPE_MODE (cent_type_node))
    return unsignedp ? ucent_type_node : cent_type_node;

  if (mode == TYPE_MODE (float_type_node))
    return float_type_node;

  if (mode == TYPE_MODE (double_type_node))
    return double_type_node;

  if (mode == TYPE_MODE (long_double_type_node))
    return long_double_type_node;

  if (mode == TYPE_MODE (build_pointer_type (char8_type_node)))
    return build_pointer_type (char8_type_node);

  if (mode == TYPE_MODE (build_pointer_type (int_type_node)))
    return build_pointer_type (int_type_node);

  if (COMPLEX_MODE_P (mode))
    {
      machine_mode inner_mode;
      tree inner_type;

      if (mode == TYPE_MODE (complex_float_type_node))
	return complex_float_type_node;
      if (mode == TYPE_MODE (complex_double_type_node))
	return complex_double_type_node;
      if (mode == TYPE_MODE (complex_long_double_type_node))
	return complex_long_double_type_node;

      inner_mode = (machine_mode) GET_MODE_INNER (mode);
      inner_type = d_type_for_mode (inner_mode, unsignedp);
      if (inner_type != NULL_TREE)
	return build_complex_type (inner_type);
    }
  else if (VECTOR_MODE_P (mode))
    {
      machine_mode inner_mode = (machine_mode) GET_MODE_INNER (mode);
      tree inner_type = d_type_for_mode (inner_mode, unsignedp);
      if (inner_type != NULL_TREE)
	return build_vector_type_for_mode (inner_type, mode);
    }

  return 0;
}

/* Implements the lang_hooks.types.type_for_size routine for language D.  */

static tree
d_type_for_size (unsigned bits, int unsignedp)
{
  if (bits <= TYPE_PRECISION (byte_type_node))
    return unsignedp ? ubyte_type_node : byte_type_node;

  if (bits <= TYPE_PRECISION (short_type_node))
    return unsignedp ? ushort_type_node : short_type_node;

  if (bits <= TYPE_PRECISION (int_type_node))
    return unsignedp ? uint_type_node : int_type_node;

  if (bits <= TYPE_PRECISION (long_type_node))
    return unsignedp ? ulong_type_node : long_type_node;

  if (bits <= TYPE_PRECISION (cent_type_node))
    return unsignedp ? ucent_type_node : cent_type_node;

  return 0;
}

/* Return the signed or unsigned version of TYPE, an integral type, the
   signedness being specified by UNSIGNEDP.  */

static tree
d_signed_or_unsigned_type (int unsignedp, tree type)
{
  if (TYPE_UNSIGNED (type) == (unsigned) unsignedp)
    return type;

  if (TYPE_PRECISION (type) == TYPE_PRECISION (cent_type_node))
    return unsignedp ? ucent_type_node : cent_type_node;

  if (TYPE_PRECISION (type) == TYPE_PRECISION (long_type_node))
    return unsignedp ? ulong_type_node : long_type_node;

  if (TYPE_PRECISION (type) == TYPE_PRECISION (int_type_node))
    return unsignedp ? uint_type_node : int_type_node;

  if (TYPE_PRECISION (type) == TYPE_PRECISION (short_type_node))
    return unsignedp ? ushort_type_node : short_type_node;

  if (TYPE_PRECISION (type) == TYPE_PRECISION (byte_type_node))
    return unsignedp ? ubyte_type_node : byte_type_node;

  return signed_or_unsigned_type_for (unsignedp, type);
}

/* Return the unsigned version of TYPE, an integral type.  */

tree
d_unsigned_type (tree type)
{
  return d_signed_or_unsigned_type (1, type);
}

/* Return the signed version of TYPE, an integral type.  */

tree
d_signed_type (tree type)
{
  return d_signed_or_unsigned_type (0, type);
}

/* Implements the lang_hooks.types.type_promotes_to routine for language D.
   All promotions for variable arguments are handled by the D frontend.  */

static tree
d_type_promotes_to (tree type)
{
  return type;
}

/* Implements the lang_hooks.decls.global_bindings_p routine for language D.
   Return true if we are in the global binding level.  */

static bool
d_global_bindings_p (void)
{
  return (current_binding_level == global_binding_level);
}

/* Return global_context, but create it first if need be.  */

static tree
get_global_context (void)
{
  if (!global_context)
    {
      global_context = build_translation_unit_decl (NULL_TREE);
      // Not supported in GCC <= 4.9
      // debug_hooks->register_main_translation_unit (global_context);
    }

  return global_context;
}

/* Implements the lang_hooks.decls.pushdecl routine for language D.
   Record DECL as belonging to the current lexical scope.  */

tree
d_pushdecl (tree decl)
{
  /* Set the context of the decl.  If current_function_decl did not help in
     determining the context, use global scope.  */
  if (!DECL_CONTEXT (decl))
    {
      if (current_function_decl)
	DECL_CONTEXT (decl) = current_function_decl;
      else
	DECL_CONTEXT (decl) = get_global_context ();
    }

  /* Put decls on list in reverse order.  */
  if (TREE_STATIC (decl) || d_global_bindings_p ())
    vec_safe_push (global_declarations, decl);
  else
    {
      TREE_CHAIN (decl) = current_binding_level->names;
      current_binding_level->names = decl;
    }

  return decl;
}

/* Implements the lang_hooks.decls.getdecls routine for language D.
   Return the list of declarations of the current level.  */

static tree
d_getdecls (void)
{
  if (current_binding_level)
    return current_binding_level->names;

  return NULL_TREE;
}


/* Implements the lang_hooks.get_alias_set routine for language D.
   Get the alias set corresponding the type or expression T.
   Return -1 if we don't do anything special.  */

static alias_set_type
d_get_alias_set (tree t)
{
  /* Permit type-punning when accessing a union, provided the access
     is directly through the union.  */
  for (tree u = t; handled_component_p (u); u = TREE_OPERAND (u, 0))
    {
      if (TREE_CODE (u) == COMPONENT_REF
	  && TREE_CODE (TREE_TYPE (TREE_OPERAND (u, 0))) == UNION_TYPE)
	return 0;
    }

  /* That's all the expressions we handle.  */
  if (!TYPE_P (t))
    return get_alias_set (TREE_TYPE (t));

  /* For now in D, assume everything aliases everything else,
     until we define some solid rules.  */
  return 0;
}

/* Implements the lang_hooks.types_compatible_p routine for language D.
   Compares two types for equivalence in the D programming language.
   This routine should only return 1 if it is sure, even though the frontend
   should have already ensured that all types are compatible before handing
   over the parsed ASTs to the code generator.  */

static int
d_types_compatible_p (tree x, tree y)
{
  Type *tx = TYPE_LANG_FRONTEND (x);
  Type *ty = TYPE_LANG_FRONTEND (y);

  /* Try validating the types in the frontend.  */
  if (tx != NULL && ty != NULL)
    {
      /* Types are equivalent.  */
      if (same_type_p (tx, ty))
	return true;

      /* Type system allows implicit conversion between.  */
      if (tx->implicitConvTo (ty) || ty->implicitConvTo (tx))
	return true;
    }

  /* Fallback on using type flags for comparison.  E.g: all dynamic arrays
     are distinct types in D, but are VIEW_CONVERT compatible.  */
  if (TREE_CODE (x) == RECORD_TYPE && TREE_CODE (y) == RECORD_TYPE)
    {
      if (TYPE_DYNAMIC_ARRAY (x) && TYPE_DYNAMIC_ARRAY (y))
	return true;

      if (TYPE_DELEGATE (x) && TYPE_DELEGATE (y))
	return true;

      if (TYPE_ASSOCIATIVE_ARRAY (x) && TYPE_ASSOCIATIVE_ARRAY (y))
	return true;
    }

  return false;
}

/* Implements the lang_hooks.finish_incomplete_decl routine for language D.  */

static void
d_finish_incomplete_decl (tree decl)
{
  if (VAR_P (decl))
    {
      /* D allows zero-length declarations.  Such a declaration ends up with
	 DECL_SIZE (t) == NULL_TREE which is what the backend function
	 assembler_variable checks.  This could change in later versions, or
	 maybe all of these variables should be aliased to one symbol. */
      if (DECL_SIZE (decl) == 0)
	{
	  DECL_SIZE (decl) = bitsize_zero_node;
	  DECL_SIZE_UNIT (decl) = size_zero_node;
	}
    }
}

/* Implements the lang_hooks.types.classify_record routine for language D.
   Return the true debug type for TYPE.  */

static classify_record
d_classify_record (tree type)
{
  Type *t = TYPE_LANG_FRONTEND (type);

  if (t && t->ty == Tclass)
    {
      TypeClass *tc = (TypeClass *) t;

      /* extern(C++) interfaces get emitted as classes.  */
      if (tc->sym->isInterfaceDeclaration ()
	  && !tc->sym->isCPPinterface ())
	return RECORD_IS_INTERFACE;

      return RECORD_IS_CLASS;
    }

  return RECORD_IS_STRUCT;
}

/* Implements the lang_hooks.tree_size routine for language D.
   Determine the size of our tcc_constant or tcc_exceptional nodes.  */

static size_t
d_tree_size (tree_code code)
{
  switch (code)
    {
    case FUNCFRAME_INFO:
      return sizeof (tree_frame_info);

    default:
      gcc_unreachable ();
    }
}

/* Implements the lang_hooks.print_xnode routine for language D.  */

static void
d_print_xnode (FILE *file, tree node, int indent)
{
  switch (TREE_CODE (node))
    {
    case FUNCFRAME_INFO:
      print_node (file, "frame_type", FRAMEINFO_TYPE (node), indent + 4);
      break;

    default:
      break;
    }
}

/* Return which tree structure is used by NODE, or TS_D_GENERIC if NODE
   is one of the language-independent trees.  */

d_tree_node_structure_enum
d_tree_node_structure (lang_tree_node *t)
{
  switch (TREE_CODE (&t->generic))
    {
    case IDENTIFIER_NODE:
      return TS_D_IDENTIFIER;

    case FUNCFRAME_INFO:
      return TS_D_FRAMEINFO;

    default:
      return TS_D_GENERIC;
    }
}

/* Allocate and return a lang specific structure for the frontend type.  */

struct lang_type *
build_lang_type (Type *t)
{
  unsigned sz = sizeof (struct lang_type);
  struct lang_type *lt = ggc_alloc_cleared_lang_type (sz);
  lt->type = t;
  return lt;
}

/* Allocate and return a lang specific structure for the frontend decl.  */

struct lang_decl *
build_lang_decl (Declaration *d)
{
  unsigned sz = sizeof (struct lang_decl);
  /* For compiler generated runtime typeinfo, a lang_decl is allocated even if
     there's no associated frontend symbol to refer to (yet).  If the symbol
     appears later in the compilation, then the slot will be re-used.  */
  if (d == NULL)
    return ggc_alloc_cleared_lang_decl (sz);

  struct lang_decl *ld = (d->csym) ? DECL_LANG_SPECIFIC (d->csym) : NULL;
  if (ld == NULL)
    ld = ggc_alloc_cleared_lang_decl (sz);

  if (ld->decl == NULL)
    ld->decl = d;

  return ld;
}

/* Implements the lang_hooks.dup_lang_specific_decl routine for language D.
   Replace the DECL_LANG_SPECIFIC field of NODE with a copy.  */

static void
d_dup_lang_specific_decl (tree node)
{
  if (! DECL_LANG_SPECIFIC (node))
    return;

  unsigned sz = sizeof (struct lang_decl);
  struct lang_decl *ld = ggc_alloc_lang_decl (sz);
  memcpy (ld, DECL_LANG_SPECIFIC (node), sizeof (struct lang_decl));
  DECL_LANG_SPECIFIC (node) = ld;
}

/* This preserves trees we create from the garbage collector.  */

static GTY(()) tree d_keep_list = NULL_TREE;

void
d_keep (tree t)
{
  d_keep_list = tree_cons (NULL_TREE, t, d_keep_list);
}

/* Implements the lang_hooks.eh_personality routine for language D.
   Return the GDC personality function decl.  */

static GTY(()) tree d_eh_personality_decl;

static tree
d_eh_personality (void)
{
  if (!d_eh_personality_decl)
    {
      d_eh_personality_decl
	= build_personality_function ("gdc");
    }
  return d_eh_personality_decl;
}

/* Implements the lang_hooks.eh_runtime_type routine for language D.  */

static tree
d_build_eh_runtime_type (tree type)
{
  Type *t = TYPE_LANG_FRONTEND (type);

  if (t != NULL)
    t = t->toBasetype ();

  gcc_assert (t != NULL && t->ty == Tclass);
  ClassDeclaration *cd = ((TypeClass *) t)->sym;
  tree decl;

  if (cd->cpp)
    decl = get_cpp_typeinfo_decl (cd);
  else
    decl = get_classinfo_decl (cd);

  return convert (ptr_type_node, build_address (decl));
}

/* Definitions for our language-specific hooks.  */

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
#undef LANG_HOOKS_GET_ALIAS_SET
#undef LANG_HOOKS_TYPES_COMPATIBLE_P
#undef LANG_HOOKS_BUILTIN_FUNCTION
#undef LANG_HOOKS_REGISTER_BUILTIN_TYPE
#undef LANG_HOOKS_FINISH_INCOMPLETE_DECL
#undef LANG_HOOKS_GIMPLIFY_EXPR
#undef LANG_HOOKS_CLASSIFY_RECORD
#undef LANG_HOOKS_TREE_SIZE
#undef LANG_HOOKS_PRINT_XNODE
#undef LANG_HOOKS_DUP_LANG_SPECIFIC_DECL
#undef LANG_HOOKS_EH_PERSONALITY
#undef LANG_HOOKS_EH_RUNTIME_TYPE
#undef LANG_HOOKS_PUSHDECL
#undef LANG_HOOKS_GETDECLS
#undef LANG_HOOKS_GLOBAL_BINDINGS_P
#undef LANG_HOOKS_WRITE_GLOBALS
#undef LANG_HOOKS_TYPE_FOR_MODE
#undef LANG_HOOKS_TYPE_FOR_SIZE
#undef LANG_HOOKS_TYPE_PROMOTES_TO

#define LANG_HOOKS_NAME			    "GNU D"
#define LANG_HOOKS_INIT			    d_init
#define LANG_HOOKS_INIT_TS		    d_init_ts
#define LANG_HOOKS_INIT_OPTIONS		    d_init_options
#define LANG_HOOKS_INIT_OPTIONS_STRUCT	    d_init_options_struct
#define LANG_HOOKS_OPTION_LANG_MASK	    d_option_lang_mask
#define LANG_HOOKS_HANDLE_OPTION	    d_handle_option
#define LANG_HOOKS_POST_OPTIONS		    d_post_options
#define LANG_HOOKS_PARSE_FILE		    d_parse_file
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE   d_langhook_common_attribute_table
#define LANG_HOOKS_ATTRIBUTE_TABLE          d_langhook_attribute_table
#define LANG_HOOKS_GET_ALIAS_SET	    d_get_alias_set
#define LANG_HOOKS_TYPES_COMPATIBLE_P	    d_types_compatible_p
#define LANG_HOOKS_BUILTIN_FUNCTION	    d_builtin_function
#define LANG_HOOKS_REGISTER_BUILTIN_TYPE    d_register_builtin_type
#define LANG_HOOKS_FINISH_INCOMPLETE_DECL   d_finish_incomplete_decl
#define LANG_HOOKS_GIMPLIFY_EXPR	    d_gimplify_expr
#define LANG_HOOKS_CLASSIFY_RECORD	    d_classify_record
#define LANG_HOOKS_TREE_SIZE		    d_tree_size
#define LANG_HOOKS_PRINT_XNODE		    d_print_xnode
#define LANG_HOOKS_DUP_LANG_SPECIFIC_DECL   d_dup_lang_specific_decl
#define LANG_HOOKS_EH_PERSONALITY	    d_eh_personality
#define LANG_HOOKS_EH_RUNTIME_TYPE	    d_build_eh_runtime_type
#define LANG_HOOKS_PUSHDECL		    d_pushdecl
#define LANG_HOOKS_GETDECLS		    d_getdecls
#define LANG_HOOKS_GLOBAL_BINDINGS_P	    d_global_bindings_p
#define LANG_HOOKS_WRITE_GLOBALS	    d_write_global_declarations
#define LANG_HOOKS_TYPE_FOR_MODE	    d_type_for_mode
#define LANG_HOOKS_TYPE_FOR_SIZE	    d_type_for_size
#define LANG_HOOKS_TYPE_PROMOTES_TO	    d_type_promotes_to

struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

#include "gt-d-d-lang.h"
#include "gtype-d.h"
