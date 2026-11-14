#ifndef JROGUE_LANGUAGE_H
#define JROGUE_LANGUAGE_H

/*
 * Language priority at startup:
 *   ROGUEOPTS language=ja/en/auto > build default > OS language.
 * Build DEFAULT_LANGUAGE: 0=auto (default), 1=ja, 2=en.
 * In auto mode, macOS/Linux use LC_ALL -> LC_MESSAGES -> LANG;
 * Windows uses the Windows user interface language.
 * The in-game menu offers Japanese and English only (not auto).
 */
#ifndef DEFAULT_LANGUAGE
#define DEFAULT_LANGUAGE 0
#endif

/*
 * DEFAULT_JAPANESE is derived internally from DEFAULT_LANGUAGE.
 * External build rules should define DEFAULT_LANGUAGE only; deriving this
 * helper here avoids inconsistent language defaults in static initializers.
 */
#ifdef DEFAULT_JAPANESE
#undef DEFAULT_JAPANESE
#endif
#if DEFAULT_LANGUAGE == 1
#define DEFAULT_JAPANESE 1
#else
#define DEFAULT_JAPANESE 0
#endif

extern int jrogue_lang_is_japanese;

void jrogue_init_locale(void);
void jrogue_detect_language(void);
void jrogue_apply_language(void);
void jrogue_apply_options_language(void);
void jrogue_apply_rip_language(void);
void jrogue_apply_potions_language(void);
void jrogue_apply_fight_language(void);
void jrogue_apply_init_language(void);
const char *jrogue_str(const char *ja, const char *en);
int jrogue_chr(int ja, int en);
const char *jrogue_language_code(void);
int jrogue_set_language(const char *lang);

/* Use these in normal code. */
#define JSTR(ja, en) jrogue_str((ja), (en))
#define JCHR(ja, en) jrogue_chr((ja), (en))

/*
 * Use these only in static initializers, where C requires compile-time
 * constant expressions.
 */
#if DEFAULT_JAPANESE
# define JSTR_STATIC(ja, en) ja
# define JCHR_STATIC(ja, en) ja
#else
# define JSTR_STATIC(ja, en) en
# define JCHR_STATIC(ja, en) en
#endif

#endif /* JROGUE_LANGUAGE_H */
