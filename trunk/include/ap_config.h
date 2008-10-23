/**
 * @file ap_config.h
 * @brief Symbol export macros and hook functions
 */

#ifndef AP_CONFIG_H
#define AP_CONFIG_H

#include "apr.h"
#include "apr_hooks.h"
#include "apr_optional_hooks.h"

#if !defined(WIN32)
/**
 * Apache Core dso functions are declared with AP_DECLARE(), so they may
 * use the most appropriate calling convention.  Hook functions and other
 * Core functions with variable arguments must use AP_DECLARE_NONSTD().
 * @code
 * AP_DECLARE(rettype) ap_func(args)
 * @endcode
 */
#define AP_DECLARE(type)            type

/**
 * Apache Core dso variable argument and hook functions are declared with 
 * AP_DECLARE_NONSTD(), as they must use the C language calling convention.
 * @see AP_DECLARE
 * @code
 * AP_DECLARE_NONSTD(rettype) ap_func(args [...])
 * @endcode
 */
#define AP_DECLARE_NONSTD(type)     type

/**
 * Apache Core dso variables are declared with AP_MODULE_DECLARE_DATA.
 * This assures the appropriate indirection is invoked at compile time.
 *
 * @note AP_DECLARE_DATA extern type apr_variable; syntax is required for
 * declarations within headers to properly import the variable.
 * @code
 * AP_DECLARE_DATA type apr_variable
 * @endcode
 */
#define AP_DECLARE_DATA

#elif defined(AP_DECLARE_STATIC)
#define AP_DECLARE(type)            type __stdcall
#define AP_DECLARE_NONSTD(type)     type
#define AP_DECLARE_DATA
#elif defined(AP_DECLARE_EXPORT)
#define AP_DECLARE(type)            __declspec(dllexport) type __stdcall
#define AP_DECLARE_NONSTD(type)     __declspec(dllexport) type
#define AP_DECLARE_DATA             __declspec(dllexport)
#else
#define AP_DECLARE(type)            __declspec(dllimport) type __stdcall
#define AP_DECLARE_NONSTD(type)     __declspec(dllimport) type
#define AP_DECLARE_DATA             __declspec(dllimport)
#endif

/**
 * Declare a hook function
 * @param ret The return type of the hook
 * @param name The hook's name (as a literal)
 * @param args The arguments the hook function takes, in brackets.
 */
#define AP_DECLARE_HOOK(ret,name,args) \
	APR_DECLARE_EXTERNAL_HOOK(ap,AP,ret,name,args)

/** @internal */
#define AP_IMPLEMENT_HOOK_BASE(name) \
	APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ap,AP,name)

/**
 * Implement an Apache core hook that has no return code, and
 * therefore runs all of the registered functions. The implementation
 * is called ap_run_<i>name</i>.
 *
 * @param name The name of the hook
 * @param args_decl The declaration of the arguments for the hook, for example
 * "(int x,void *y)"
 * @param args_use The arguments for the hook as used in a call, for example
 * "(x,y)"
 * @note If IMPLEMENTing a hook that is not linked into the Apache core,
 * (e.g. within a dso) see APR_IMPLEMENT_EXTERNAL_HOOK_VOID.
 */
#define AP_IMPLEMENT_HOOK_VOID(name,args_decl,args_use) \
	APR_IMPLEMENT_EXTERNAL_HOOK_VOID(ap,AP,name,args_decl,args_use)

/**
 * Implement an Apache core hook that runs until one of the functions
 * returns something other than ok or decline. That return value is
 * then returned from the hook runner. If the hooks run to completion,
 * then ok is returned. Note that if no hook runs it would probably be
 * more correct to return decline, but this currently does not do
 * so. The implementation is called ap_run_<i>name</i>.
 *
 * @param ret The return type of the hook (and the hook runner)
 * @param name The name of the hook
 * @param args_decl The declaration of the arguments for the hook, for example
 * "(int x,void *y)"
 * @param args_use The arguments for the hook as used in a call, for example
 * "(x,y)"
 * @param ok The "ok" return value
 * @param decline The "decline" return value
 * @return ok, decline or an error.
 * @note If IMPLEMENTing a hook that is not linked into the Apache core,
 * (e.g. within a dso) see APR_IMPLEMENT_EXTERNAL_HOOK_RUN_ALL.
 */
#define AP_IMPLEMENT_HOOK_RUN_ALL(ret,name,args_decl,args_use,ok,decline) \
	APR_IMPLEMENT_EXTERNAL_HOOK_RUN_ALL(ap,AP,ret,name,args_decl, \
                                            args_use,ok,decline)

/**
 * Implement a hook that runs until a function returns something other than 
 * decline. If all functions return decline, the hook runner returns decline. 
 * The implementation is called ap_run_<i>name</i>.
 *
 * @param ret The return type of the hook (and the hook runner)
 * @param name The name of the hook
 * @param args_decl The declaration of the arguments for the hook, for example
 * "(int x,void *y)"
 * @param args_use The arguments for the hook as used in a call, for example
 * "(x,y)"
 * @param decline The "decline" return value
 * @return decline or an error.
 * @note If IMPLEMENTing a hook that is not linked into the Apache core
 * (e.g. within a dso) see APR_IMPLEMENT_EXTERNAL_HOOK_RUN_FIRST.
 */
#define AP_IMPLEMENT_HOOK_RUN_FIRST(ret,name,args_decl,args_use,decline) \
	APR_IMPLEMENT_EXTERNAL_HOOK_RUN_FIRST(ap,AP,ret,name,args_decl, \
                                              args_use,decline)

/* Note that the other optional hook implementations are straightforward but
 * have not yet been needed
 */

/**
 * Implement an optional hook. This is exactly the same as a standard hook
 * implementation, except the hook is optional.
 * @see AP_IMPLEMENT_HOOK_RUN_ALL
 */
#define AP_IMPLEMENT_OPTIONAL_HOOK_RUN_ALL(ret,name,args_decl,args_use,ok, \
					   decline) \
	APR_IMPLEMENT_OPTIONAL_HOOK_RUN_ALL(ap,AP,ret,name,args_decl, \
                                            args_use,ok,decline)

/**
 * Hook an optional hook. Unlike static hooks, this uses a macro instead of a
 * function.
 */
#define AP_OPTIONAL_HOOK(name,fn,pre,succ,order) \
        APR_OPTIONAL_HOOK(ap,name,fn,pre,succ,order)

#if !defined(WIN32) && !defined(NETWARE)
#include "ap_config_auto.h"
#endif
#if defined(NETWARE)
#define AP_NONBLOCK_WHEN_MULTI_LISTEN 1
#endif

/* TODO - We need to put OS detection back to make all the following work */

#if defined(SUNOS4) || defined(IRIX) || defined(NEXT) || defined(AUX3) \
    || defined (UW) || defined(LYNXOS) || defined(TPF)
/* These systems don't do well with any lingering close code; I don't know
 * why -- manoj */
#define NO_LINGCLOSE
#endif

/* If APR has OTHER_CHILD logic, use reliable piped logs. */
#if APR_HAS_OTHER_CHILD
#define AP_HAVE_RELIABLE_PIPED_LOGS TRUE
#endif

/* Presume that the compiler supports C99-style designated
 * initializers if using GCC (but not G++), or for any other compiler
 * which claims C99 support. */
#if (defined(__GNUC__) && !defined(__cplusplus))                \
     || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#define AP_HAVE_DESIGNATED_INITIALIZER
#endif

#endif /* AP_CONFIG_H */
