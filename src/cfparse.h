#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define	LVALUE	257
# define	ID	258
# define	LIST	259
# define	ITEM	260
# define	VAROBJ	261
# define	LBRACK	262
# define	RBRACK	263
# define	CONTROL	264
# define	GROUPS	265
# define	ARROW	266
# define	EQUALS	267
# define	EDITFILES	268
# define	QSTRING	269
# define	WILDCARD	270
# define	RVALUE	271
# define	BCLASS	272
# define	LBRACE	273
# define	RBRACE	274
# define	PARSECLASS	275
# define	LARROW	276
# define	OPTION	277
# define	FUNCTION	278
# define	ACL	279
# define	ADMIT	280
# define	DENY	281
# define	FILTERS	282
# define	STRATEGIES	283
# define	ACTIONTYPE	284
# define	ACCESSOBJ	285


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
