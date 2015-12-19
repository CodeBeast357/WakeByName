/* Getopt for Microsoft C
This code is a modification of the Free Software Foundation, Inc.
Getopt library for parsing command line argument the purpose was
to provide a Microsoft Visual C friendly derivative. This code
provides functionality for both Unicode and Multibyte builds.

Date: 02/03/2011 - Ludvik Jerabek - Initial Release
Version: 1.0
Comment: Supports getopt, getopt_long, and getopt_long_only
and POSIXLY_CORRECT environment flag
License: LGPL

Revisions:

02/03/2011 - Ludvik Jerabek - Initial Release
02/20/2011 - Ludvik Jerabek - Fixed compiler warnings at Level 4
07/05/2011 - Ludvik Jerabek - Added no_argument, required_argument, optional_argument defs
08/03/2011 - Ludvik Jerabek - Fixed non-argument runtime bug which caused runtime exception
08/09/2011 - Ludvik Jerabek - Added code to export functions for DLL and LIB
02/15/2012 - Ludvik Jerabek - Fixed _GETOPT_THROW definition missing in implementation file
08/01/2012 - Ludvik Jerabek - Created separate functions for char and wchar_t characters so single dll can do both unicode and ansi
10/15/2012 - Ludvik Jerabek - Modified to match latest GNU features
06/19/2015 - Ludvik Jerabek - Fixed maximum option limitation caused by option_a (255) and option_w (65535) structure val variable
18/12/2015 - CodeBeast357 - Resolved all string related functions to _TCHAR

**DISCLAIMER**
THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING, BUT Not LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, OR NON-INFRINGEMENT. SOME JURISDICTIONS DO NOT ALLOW THE
EXCLUSION OF IMPLIED WARRANTIES, SO THE ABOVE EXCLUSION MAY NOT
APPLY TO YOU. IN NO EVENT WILL I BE LIABLE TO ANY PARTY FOR ANY
DIRECT, INDIRECT, SPECIAL OR OTHER CONSEQUENTIAL DAMAGES FOR ANY
USE OF THIS MATERIAL INCLUDING, WITHOUT LIMITATION, ANY LOST
PROFITS, BUSINESS INTERRUPTION, LOSS OF PROGRAMS OR OTHER DATA ON
YOUR INFORMATION HANDLING SYSTEM OR OTHERWISE, EVEN If WE ARE
EXPRESSLY ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/
#include "stdafx.h"
#include <malloc.h>

#ifdef __cplusplus
#define _GETOPT_THROW throw()
#else
#define _GETOPT_THROW
#endif

INT optind = 1;
INT opterr = 1;
INT optopt = '?';
enum ENUM_ORDERING { REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER };

static struct _getopt_data {
    INT optind;
    INT opterr;
    INT optopt;
    _TCHAR *optarg;
    INT __initialized;
    _TCHAR *__nextchar;
    enum ENUM_ORDERING __ordering;
    INT __posixly_correct;
    INT __first_nonopt;
    INT __last_nonopt;
} getopt_data;
_TCHAR *optarg;

static void exchange(_TCHAR **argv, struct _getopt_data *d) {
    INT bottom = d->__first_nonopt;
    INT middle = d->__last_nonopt;
    INT top = d->optind;
    _TCHAR *tem;
    while (top > middle && middle > bottom) {
        if (top - middle > middle - bottom) {
            INT len = middle - bottom;
            register INT i;
            for (i = 0; i < len; i++) {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[top - (middle - bottom) + i];
                argv[top - (middle - bottom) + i] = tem;
            }
            top -= len;
        } else {
            INT len = top - middle;
            register INT i;
            for (i = 0; i < len; i++) {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[middle + i];
                argv[middle + i] = tem;
            }
            bottom += len;
        }
    }
    d->__first_nonopt += (d->optind - d->__last_nonopt);
    d->__last_nonopt = d->optind;
}
static const _TCHAR *_getopt_initialize(const _TCHAR *optstring, struct _getopt_data *d, INT posixly_correct) {
    d->__first_nonopt = d->__last_nonopt = d->optind;
    d->__nextchar = NULL;
    d->__posixly_correct = posixly_correct | !!_tgetenv(_T("POSIXLY_CORRECT"));
    if (optstring[0] == _T('-')) {
        d->__ordering = RETURN_IN_ORDER;
        ++optstring;
    } else if (optstring[0] == _T('+')) {
        d->__ordering = REQUIRE_ORDER;
        ++optstring;
    } else if (d->__posixly_correct)
        d->__ordering = REQUIRE_ORDER;
    else
        d->__ordering = PERMUTE;
    return optstring;
}
INT _getopt_internal_r(INT argc, _TCHAR *const *argv, const _TCHAR *optstring, const struct option *longopts, INT *longind, INT long_only, struct _getopt_data *d, INT posixly_correct) {
    INT print_errors = d->opterr;
    if (argc < 1)
        return -1;
    d->optarg = NULL;
    if (d->optind == 0 || !d->__initialized) {
        if (d->optind == 0)
            d->optind = 1;
        optstring = _getopt_initialize(optstring, d, posixly_correct);
        d->__initialized = 1;
    } else if (optstring[0] == _T('-') || optstring[0] == _T('+'))
        optstring++;
    if (optstring[0] == _T(':'))
        print_errors = 0;
    if (d->__nextchar == NULL || *d->__nextchar == _T('\0')) {
        if (d->__last_nonopt > d->optind)
            d->__last_nonopt = d->optind;
        if (d->__first_nonopt > d->optind)
            d->__first_nonopt = d->optind;
        if (d->__ordering == PERMUTE) {
            if (d->__first_nonopt != d->__last_nonopt && d->__last_nonopt != d->optind)
                exchange((_TCHAR **)argv, d);
            else if (d->__last_nonopt != d->optind)
                d->__first_nonopt = d->optind;
            while (d->optind < argc && (argv[d->optind][0] != _T('-') || argv[d->optind][1] == _T('\0')))
                d->optind++;
            d->__last_nonopt = d->optind;
        }
        if (d->optind != argc && !_tcscmp(argv[d->optind], _T("--"))) {
            d->optind++;
            if (d->__first_nonopt != d->__last_nonopt && d->__last_nonopt != d->optind)
                exchange((_TCHAR **)argv, d);
            else if (d->__first_nonopt == d->__last_nonopt)
                d->__first_nonopt = d->optind;
            d->__last_nonopt = argc;
            d->optind = argc;
        }
        if (d->optind == argc) {
            if (d->__first_nonopt != d->__last_nonopt)
                d->optind = d->__first_nonopt;
            return -1;
        }
        if ((argv[d->optind][0] != _T('-') || argv[d->optind][1] == _T('\0'))) {
            if (d->__ordering == REQUIRE_ORDER)
                return -1;
            d->optarg = argv[d->optind++];
            return 1;
        }
        d->__nextchar = (argv[d->optind] + 1 + (longopts != NULL && argv[d->optind][1] == _T('-')));
    }
    if (longopts != NULL && (argv[d->optind][1] == _T('-') || (long_only && (argv[d->optind][2] || !_tcschr(optstring, argv[d->optind][1]))))) {
        _TCHAR *nameend;
        UINT namelen;
        const struct option *p;
        const struct option *pfound = NULL;
        struct option_list {
            const struct option *p;
            struct option_list *next;
        } *ambig_list = NULL;
        INT exact = 0;
        INT indfound = -1;
        INT option_index;
        for (nameend = d->__nextchar; *nameend && *nameend != _T('='); nameend++);
        namelen = (UINT)(nameend - d->__nextchar);
        for (p = longopts, option_index = 0; p->name; p++, option_index++)
            if (!_tcsncmp(p->name, d->__nextchar, namelen)) {
                if (namelen == (UINT)_tcslen(p->name)) {
                    pfound = p;
                    indfound = option_index;
                    exact = 1;
                    break;
                } else if (pfound == NULL) {
                    pfound = p;
                    indfound = option_index;
                } else if (long_only || pfound->has_arg != p->has_arg || pfound->flag != p->flag || pfound->val != p->val) {
                    struct option_list *newp = (struct option_list*)_malloca(sizeof(*newp));
                    newp->p = p;
                    newp->next = ambig_list;
                    ambig_list = newp;
                }
            }
        if (ambig_list != NULL && !exact) {
            if (print_errors) {
                struct option_list first;
                first.p = pfound;
                first.next = ambig_list;
                ambig_list = &first;
                _ftprintf(stderr, _T("%s: option '%s' is ambiguous; possibilities:"), argv[0], argv[d->optind]);
                do {
                    _ftprintf(stderr, _T(" '--%s'"), ambig_list->p->name);
                    ambig_list = ambig_list->next;
                } while (ambig_list != NULL);
                fputc(_T('\n'), stderr);
            }
            d->__nextchar += _tcslen(d->__nextchar);
            d->optind++;
            d->optopt = 0;
            return _T('?');
        }
        if (pfound != NULL) {
            option_index = indfound;
            d->optind++;
            if (*nameend) {
                if (pfound->has_arg)
                    d->optarg = nameend + 1;
                else {
                    if (print_errors) {
                        if (argv[d->optind - 1][1] == _T('-')) {
                            _ftprintf(stderr, _T("%s: option '--%s' doesn't allow an argument\n"), argv[0], pfound->name);
                        } else {
                            _ftprintf(stderr, _T("%s: option '%c%s' doesn't allow an argument\n"), argv[0], argv[d->optind - 1][0], pfound->name);
                        }
                    }
                    d->__nextchar += _tcslen(d->__nextchar);
                    d->optopt = pfound->val;
                    return _T('?');
                }
            } else if (pfound->has_arg == 1) {
                if (d->optind < argc)
                    d->optarg = argv[d->optind++];
                else {
                    if (print_errors) {
                        _ftprintf(stderr, _T("%s: option '--%s' requires an argument\n"), argv[0], pfound->name);
                    }
                    d->__nextchar += _tcslen(d->__nextchar);
                    d->optopt = pfound->val;
                    return optstring[0] == _T(':') ? _T(':') : _T('?');
                }
            }
            d->__nextchar += _tcslen(d->__nextchar);
            if (longind != NULL)
                *longind = option_index;
            if (pfound->flag) {
                *(pfound->flag) = pfound->val;
                return 0;
            }
            return pfound->val;
        }
        if (!long_only || argv[d->optind][1] == _T('-') || _tcschr(optstring, *d->__nextchar) == NULL) {
            if (print_errors) {
                if (argv[d->optind][1] == _T('-')) {
                    _ftprintf(stderr, _T("%s: unrecognized option '--%s'\n"), argv[0], d->__nextchar);
                } else {
                    _ftprintf(stderr, _T("%s: unrecognized option '%c%s'\n"), argv[0], argv[d->optind][0], d->__nextchar);
                }
            }
            d->__nextchar = (_TCHAR *)_T("");
            d->optind++;
            d->optopt = 0;
            return _T('?');
        }
    }
    {
        _TCHAR c = *d->__nextchar++;
        _TCHAR *temp = (_TCHAR*)_tcschr(optstring, c);
        if (*d->__nextchar == _T('\0'))
            ++d->optind;
        if (temp == NULL || c == _T(':') || c == _T(';')) {
            if (print_errors) {
                _ftprintf(stderr, _T("%s: invalid option -- '%c'\n"), argv[0], c);
            }
            d->optopt = c;
            return _T('?');
        }
        if (temp[0] == _T('W') && temp[1] == _T(';')) {
            _TCHAR *nameend;
            const struct option *p;
            const struct option *pfound = NULL;
            INT exact = 0;
            INT ambig = 0;
            INT indfound = 0;
            INT option_index;
            if (longopts == NULL)
                goto no_longs;
            if (*d->__nextchar != _T('\0')) {
                d->optarg = d->__nextchar;
                d->optind++;
            } else if (d->optind == argc) {
                if (print_errors) {
                    _ftprintf(stderr, _T("%s: option requires an argument -- '%c'\n"), argv[0], c);
                }
                d->optopt = c;
                if (optstring[0] == _T(':'))
                    c = _T(':');
                else
                    c = _T('?');
                return c;
            } else
                d->optarg = argv[d->optind++];
            for (d->__nextchar = nameend = d->optarg; *nameend && *nameend != _T('='); nameend++);
            for (p = longopts, option_index = 0; p->name; p++, option_index++)
                if (!_tcsncmp(p->name, d->__nextchar, nameend - d->__nextchar)) {
                    if ((UINT)(nameend - d->__nextchar) == _tcslen(p->name)) {
                        pfound = p;
                        indfound = option_index;
                        exact = 1;
                        break;
                    } else if (pfound == NULL) {
                        pfound = p;
                        indfound = option_index;
                    } else if (long_only || pfound->has_arg != p->has_arg || pfound->flag != p->flag || pfound->val != p->val)
                        ambig = 1;
                }
            if (ambig && !exact) {
                if (print_errors) {
                    _ftprintf(stderr, _T("%s: option '-W %s' is ambiguous\n"), argv[0], d->optarg);
                }
                d->__nextchar += _tcslen(d->__nextchar);
                d->optind++;
                return _T('?');
            }
            if (pfound != NULL) {
                option_index = indfound;
                if (*nameend) {
                    if (pfound->has_arg)
                        d->optarg = nameend + 1;
                    else {
                        if (print_errors) {
                            _ftprintf(stderr, _T("%s: option '-W %s' doesn't allow an argument\n"), argv[0], pfound->name);
                        }
                        d->__nextchar += _tcslen(d->__nextchar);
                        return _T('?');
                    }
                } else if (pfound->has_arg == 1) {
                    if (d->optind < argc)
                        d->optarg = argv[d->optind++];
                    else {
                        if (print_errors) {
                            _ftprintf(stderr, _T("%s: option '-W %s' requires an argument\n"), argv[0], pfound->name);
                        }
                        d->__nextchar += _tcslen(d->__nextchar);
                        return optstring[0] == _T(':') ? _T(':') : _T('?');
                    }
                } else
                    d->optarg = NULL;
                d->__nextchar += _tcslen(d->__nextchar);
                if (longind != NULL)
                    *longind = option_index;
                if (pfound->flag) {
                    *(pfound->flag) = pfound->val;
                    return 0;
                }
                return pfound->val;
            }
        no_longs:
            d->__nextchar = NULL;
            return _T('W');
        }
        if (temp[1] == _T(':')) {
            if (temp[2] == _T(':')) {
                if (*d->__nextchar != _T('\0')) {
                    d->optarg = d->__nextchar;
                    d->optind++;
                } else
                    d->optarg = NULL;
                d->__nextchar = NULL;
            } else {
                if (*d->__nextchar != _T('\0')) {
                    d->optarg = d->__nextchar;
                    d->optind++;
                } else if (d->optind == argc) {
                    if (print_errors) {
                        _ftprintf(stderr, _T("%s: option requires an argument -- '%c'\n"), argv[0], c);
                    }
                    d->optopt = c;
                    if (optstring[0] == _T(':'))
                        c = _T(':');
                    else
                        c = _T('?');
                } else
                    d->optarg = argv[d->optind++];
                d->__nextchar = NULL;
            }
        }
        return c;
    }
}
INT _getopt_internal(INT argc, _TCHAR *const *argv, const _TCHAR *optstring, const struct option *longopts, INT *longind, INT long_only, INT posixly_correct) {
    INT result;
    getopt_data.optind = optind;
    getopt_data.opterr = opterr;
    result = _getopt_internal_r(argc, argv, optstring, longopts, longind, long_only, &getopt_data, posixly_correct);
    optind = getopt_data.optind;
    optarg = getopt_data.optarg;
    optopt = getopt_data.optopt;
    return result;
}
INT getopt(INT argc, _TCHAR *const *argv, const _TCHAR *optstring) _GETOPT_THROW
{
    return _getopt_internal(argc, argv, optstring, (const struct option *) 0, (INT *)0, 0, 0);
}
INT getopt_long(INT argc, _TCHAR *const *argv, const _TCHAR *options, const struct option *long_options, INT *opt_index) _GETOPT_THROW
{
    return _getopt_internal(argc, argv, options, long_options, opt_index, 0, 0);
}
INT getopt_long_only(INT argc, _TCHAR *const *argv, const _TCHAR *options, const struct option *long_options, INT *opt_index) _GETOPT_THROW
{
    return _getopt_internal(argc, argv, options, long_options, opt_index, 1, 0);
}
INT _getopt_long_r(INT argc, _TCHAR *const *argv, const _TCHAR *options, const struct option *long_options, INT *opt_index, struct _getopt_data *d) {
    return _getopt_internal_r(argc, argv, options, long_options, opt_index, 0, d, 0);
}
INT _getopt_long_only_r(INT argc, _TCHAR *const *argv, const _TCHAR *options, const struct option *long_options, INT *opt_index, struct _getopt_data *d) {
    return _getopt_internal_r(argc, argv, options, long_options, opt_index, 1, d, 0);
}
