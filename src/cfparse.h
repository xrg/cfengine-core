#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define	ID	257
# define	LIST	258
# define	ITEM	259
# define	VARPATH	260
# define	PATH	261
# define	LBRACK	262
# define	RBRACK	263
# define	CONTROL	264
# define	GROUPS	265
# define	ARROW	266
# define	EQUALS	267
# define	EDITFILES	268
# define	QSTRING	269
# define	VARITEM	270
# define	WILDCARD	271
# define	LBRACE	272
# define	RBRACE	273
# define	PARSECLASS	274
# define	LARROW	275
# define	ACL	276
# define	ADMIT	277
# define	DENY	278
# define	FILTERS	279
# define	STRATEGIES	280
# define	ACTIONTYPE	281


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
