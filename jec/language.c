#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "language.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

int jrogue_lang_is_japanese = DEFAULT_JAPANESE;

/* Only ja/en/auto are valid option values; do not accept arbitrary prefixes. */
static int
lang_equals(const char *value, const char *expected)
{
    if (value == NULL)
        return FALSE;
    while (*value && *expected)
    {
        if (tolower((unsigned char)*value) != tolower((unsigned char)*expected))
            return FALSE;
        value++;
        expected++;
    }
    return *value == '\0' && *expected == '\0';
}

#ifndef _WIN32
/* Locale names include a region/encoding suffix (e.g. ja_JP.UTF-8). */
static int
locale_is_ja(const char *locale)
{
    if (locale == NULL ||
        tolower((unsigned char)locale[0]) != 'j' ||
        tolower((unsigned char)locale[1]) != 'a')
        return FALSE;
    return locale[2] == '\0' || locale[2] == '_' ||
           locale[2] == '-' || locale[2] == '.' || locale[2] == '@';
}
#endif

/*
 * Windows uses a UTF-8 C locale for text processing, independent of the
 * Windows user-interface language used for initial language selection.
 */
void
jrogue_init_locale(void)
{
#ifdef _WIN32
    if (setlocale(LC_ALL, ".UTF8") == NULL)
        (void) setlocale(LC_ALL, "");
#else
    (void) setlocale(LC_ALL, "");
#endif
}

static int
jrogue_os_language_is_japanese(void)
{
#ifdef _WIN32
    /* Avoid the CRT locale and narrow environment's Windows code page. */
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_JAPANESE;
#else
    const char *locale = getenv("LC_ALL");
    if (locale == NULL || *locale == '\0')
        locale = getenv("LC_MESSAGES");
    if (locale == NULL || *locale == '\0')
        locale = getenv("LANG");
    return locale_is_ja(locale);
#endif
}

/* Called once before ROGUEOPTS: the build default only affects startup. */
void
jrogue_detect_language(void)
{
#if DEFAULT_LANGUAGE == 1
    jrogue_lang_is_japanese = TRUE;
#elif DEFAULT_LANGUAGE == 2
    jrogue_lang_is_japanese = FALSE;
#else
    jrogue_lang_is_japanese = jrogue_os_language_is_japanese();
#endif
}

const char *
jrogue_str(const char *ja, const char *en)
{
    return jrogue_lang_is_japanese ? ja : en;
}

int
jrogue_chr(int ja, int en)
{
    return jrogue_lang_is_japanese ? ja : en;
}

const char *
jrogue_language_code(void)
{
    return jrogue_lang_is_japanese ? "ja" : "en";
}

/* ROGUEOPTS is the only environment variable that selects game language.
 * auto requests OS detection even if the build has a fixed default.
 * The interactive options screen deliberately offers only ja/en.
 */
int
jrogue_set_language(const char *lang)
{
    if (lang_equals(lang, "ja"))
    {
        jrogue_lang_is_japanese = TRUE;
        return TRUE;
    }
    if (lang_equals(lang, "en"))
    {
        jrogue_lang_is_japanese = FALSE;
        return TRUE;
    }
    if (lang_equals(lang, "auto"))
    {
        jrogue_lang_is_japanese = jrogue_os_language_is_japanese();
        return TRUE;
    }
    return FALSE;
}
