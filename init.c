/*
 * global variable initializaton
 *
 * @(#)init.c	4.31 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"

/*
 * init_player:
 *	Roll her up
 */
void
init_player(void)
{
    THING *obj;

    pstats = max_stats;
    food_left = HUNGERTIME;
    /*
     * Give him some food
     */
    obj = new_item();
    obj->o_type = FOOD;
    obj->o_count = 1;
    add_pack(obj, TRUE);
    /*
     * And his suit of armor
     */
    obj = new_item();
    obj->o_type = ARMOR;
    obj->o_which = RING_MAIL;
    obj->o_arm = a_class[RING_MAIL] - 1;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    cur_armor = obj;
    add_pack(obj, TRUE);
    /*
     * Give him his weaponry.  First a mace.
     */
    obj = new_item();
    init_weapon(obj, MACE);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    cur_weapon = obj;
    /*
     * Now a +1 bow
     */
    obj = new_item();
    init_weapon(obj, BOW);
    obj->o_hplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    /*
     * Now some arrows
     */
    obj = new_item();
    init_weapon(obj, ARROW);
    obj->o_count = rnd(15) + 25;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
}

/*
 * Contains defintions and functions for dealing with things like
 * potions and scrolls
 */

const char *rainbow[] = {
    JSTR_STATIC("こはく色", "amber"),
    JSTR_STATIC("淡い青緑色", "aquamarine"),
    JSTR_STATIC("黒色", "black"),
    JSTR_STATIC("青色", "blue"),
    JSTR_STATIC("茶色", "brown"),
    JSTR_STATIC("透明", "clear"),
    JSTR_STATIC("深紅色", "crimson"),
    JSTR_STATIC("水色", "cyan"),
    JSTR_STATIC("ベージュ色", "ecru"),
    JSTR_STATIC("金色", "gold"),
    JSTR_STATIC("緑色", "green"),
    JSTR_STATIC("灰色", "grey"),
    JSTR_STATIC("赤紫色", "magenta"),
    JSTR_STATIC("オレンジ色", "orange"),
    JSTR_STATIC("ピンク色", "pink"),
    JSTR_STATIC("格子柄", "plaid"),
    JSTR_STATIC("紫色", "purple"),
    JSTR_STATIC("赤色", "red"),
    JSTR_STATIC("銀色", "silver"),
    JSTR_STATIC("淡い茶色", "tan"),
    JSTR_STATIC("みかん色", "tangerine"),
    JSTR_STATIC("淡褐色", "topaz"),
    JSTR_STATIC("青緑色", "turquoise"),
    JSTR_STATIC("朱色", "vermilion"),
    JSTR_STATIC("すみれ色", "violet"),
    JSTR_STATIC("白色", "white"),
    JSTR_STATIC("黄色", "yellow"),
};

#define NCOLORS (sizeof rainbow / sizeof (char *))

/* Keep both spellings together.  The English spelling is used to make the
 * original Rogue 5.4.5 length decisions, regardless of the display language;
 * the Japanese spelling is used only to enforce our shared 30-cell limit. */
struct scroll_syllable {
    const char *ja;
    const char *en;
};

static const struct scroll_syllable sylls[] = {
    {"ア", "a"}, {"アブ", "ab"}, {"アグ", "ag"}, {"アクス", "aks"}, {"アラ", "ala"},
    {"アン", "an"}, {"アプ", "app"}, {"アーグ", "arg"}, {"アーゼ", "arze"}, {"アッシュ", "ash"},
    {"ベク", "bek"}, {"ビー", "bie"}, {"ビット", "bit"}, {"ビジョー", "bjor"}, {"ブル", "blu"},
    {"ボット", "bot"}, {"ブ", "bu"}, {"バイト", "byt"}, {"コンプ", "comp"}, {"コン", "con"},
    {"コス", "cos"}, {"クレ", "cre"}, {"ダルフ", "dalf"}, {"ダン", "dan"}, {"デン", "den"},
    {"ド", "do"}, {"エ", "e"}, {"イープ", "eep"}, {"エル", "el"}, {"エング", "eng"},
    {"エー", "er"}, {"エレ", "ere"}, {"イーク", "erk"}, {"エッシュ", "esh"}, {"エブス", "evs"},
    {"ファ", "fa"}, {"フィッド", "fid"}, {"フライ", "fri"}, {"フー", "fu"}, {"ガン", "gan"},
    {"ガー", "gar"}, {"グレン", "glen"}, {"ゴップ", "gop"}, {"グレ", "gre"}, {"ハー", "ha"},
    {"ヒド", "hyd"}, {"イ", "i"}, {"イング", "ing"}, {"イップ", "ip"}, {"イッシュ", "ish"},
    {"イット", "it"}, {"アイト", "ite"}, {"イブ", "iv"}, {"ジョー", "jo"}, {"コー", "kho"},
    {"クリ", "kli"}, {"クリス", "klis"}, {"ラー", "la"}, {"レック", "lech"}, {"マー", "mar"},
    {"メ", "me"}, {"マイ", "mi"}, {"マイク", "mic"}, {"ミク", "mik"}, {"モン", "mon"},
    {"ムング", "mung"}, {"マー", "mur"}, {"ネジュ", "nej"}, {"ネルグ", "nelg"}, {"ネップ", "nep"},
    {"ナー", "ner"}, {"ネス", "nes"}, {"ネス", "nes"}, {"ニー", "nih"}, {"ニン", "nin"},
    {"オ", "o"}, {"オッド", "od"}, {"オード", "ood"}, {"オーグ", "org"}, {"オルン", "orn"},
    {"オックス", "ox"}, {"オキシー", "oxy"}, {"ペイ", "pay"}, {"プル", "ple"}, {"プルー", "plu"},
    {"ポ", "po"}, {"ポット", "pot"}, {"プロック", "prok"}, {"リ", "re"}, {"レー", "rea"},
    {"ロブ", "rhov"}, {"リー", "ri"}, {"ロ", "ro"}, {"ログ", "rog"}, {"ロク", "rok"},
    {"ロル", "rol"}, {"サ", "sa"}, {"サン", "san"}, {"サット", "sat"}, {"セフ", "sef"},
    {"ゼー", "seh"}, {"シュ", "shu"}, {"スキ", "ski"}, {"スネイ", "sna"}, {"スネ", "sne"},
    {"スニック", "snik"}, {"スノ", "sno"}, {"ソ", "so"}, {"ソル", "sol"}, {"スリ", "sri"},
    {"スタ", "sta"}, {"サン", "sun"}, {"タ", "ta"}, {"タブ", "tab"}, {"テム", "tem"},
    {"ザー", "ther"}, {"ティ", "ti"}, {"トックス", "tox"}, {"トロール", "trol"}, {"チュー", "tue"},
    {"ターズ", "turs"}, {"ウ", "u"}, {"ウルク", "ulk"}, {"ウム", "um"}, {"ウン", "un"},
    {"ウニ", "uni"}, {"アー", "ur"}, {"ヴァル", "val"}, {"ヴィヴ", "viv"}, {"ヴライ", "vly"},
    {"ヴォム", "vom"}, {"ワー", "wah"}, {"ウェド", "wed"}, {"ワーグ", "werg"}, {"ウェックス", "wex"},
    {"ホン", "whon"}, {"ウン", "wun"}, {"クソー", "xo"}, {"イ", "y"}, {"ヨット", "yot"},
    {"ユ", "yu"}, {"ザント", "zant"}, {"ゼブ", "zeb"}, {"ジム", "zim"}, {"ゾック", "zok"},
    {"ゾン", "zon"}, {"ズム", "zum"},
};

#define NSYLLS (sizeof sylls / sizeof sylls[0])

STONE stones[] = {
    { JSTR_STATIC("めのう", "agate"),		 25},
    { JSTR_STATIC("アレキサンドライト", "alexandrite"),	 40},
    { JSTR_STATIC("紫水晶", "amethyst"),	 50},
    { JSTR_STATIC("カーネリアン", "carnelian"),	 40},
    { JSTR_STATIC("ダイヤ", "diamond"),	300},
    { JSTR_STATIC("エメラルド", "emerald"),	300},
    { JSTR_STATIC("ゲルマニウム", "germanium"),	225},
    { JSTR_STATIC("御影石", "granite"),	  5},
    { JSTR_STATIC("ざくろ石", "garnet"),		 50},
    { JSTR_STATIC("ひすい", "jade"),		150},
    { JSTR_STATIC("クリプトナイト", "kryptonite"),	300},
    { JSTR_STATIC("るり", "lapis lazuli"),	 50},
    { JSTR_STATIC("月長石", "moonstone"),	 50},
    { JSTR_STATIC("黒曜石", "obsidian"),	 15},
    { JSTR_STATIC("オニキス", "onyx"),		 60},
    { JSTR_STATIC("オパール", "opal"),		200},
    { JSTR_STATIC("真珠", "pearl"),		220},
    { JSTR_STATIC("ペリドット", "peridot"),	 63},
    { JSTR_STATIC("ルビー", "ruby"),		350},
    { JSTR_STATIC("サファイア", "sapphire"),	285},
    { JSTR_STATIC("安タンタル石", "stibotantalite"),	200},
    { JSTR_STATIC("虎目石", "tiger eye"),	 50},
    { JSTR_STATIC("トパーズ", "topaz"),		 60},
    { JSTR_STATIC("トルコ石", "turquoise"),	 70},
    { JSTR_STATIC("ターフェアイト", "taaffeite"),	300},
    { JSTR_STATIC("ジルコン", "zircon"),	 	 80},
};

#define NSTONES (sizeof stones / sizeof (STONE))

const char *wood[] = {
    JSTR_STATIC("アボカド", "avocado wood"),
    JSTR_STATIC("バルサ", "balsa"),
    JSTR_STATIC("竹", "bamboo"),
    JSTR_STATIC("バニヤン", "banyan"),
    JSTR_STATIC("白樺", "birch"),
    JSTR_STATIC("杉", "cedar"),
    JSTR_STATIC("桜", "cherry"),
    JSTR_STATIC("辰砂", "cinnibar"),
    JSTR_STATIC("糸杉", "cypress"),
    JSTR_STATIC("ハナミズキ", "dogwood"),
    JSTR_STATIC("流木", "driftwood"),
    JSTR_STATIC("黒檀", "ebony"),
    JSTR_STATIC("楡", "elm"),
    JSTR_STATIC("ユーカリ", "eucalyptus"),
    JSTR_STATIC("堕落", "fall"),
    JSTR_STATIC("ツガ", "hemlock"),
    JSTR_STATIC("ヒイラギ", "holly"),
    JSTR_STATIC("鉄樹", "ironwood"),
    JSTR_STATIC("ククイノキ", "kukui wood"),
    JSTR_STATIC("マホガニー", "mahogany"),
    JSTR_STATIC("マンザニータ", "manzanita"),
    JSTR_STATIC("楓", "maple"),
    JSTR_STATIC("樫", "oaken"),
    JSTR_STATIC("柿", "persimmon wood"),
    JSTR_STATIC("ピーカン", "pecan"),
    JSTR_STATIC("松", "pine"),
    JSTR_STATIC("ポプラ", "poplar"),
    JSTR_STATIC("アメリカ杉", "redwood"),
    JSTR_STATIC("紫檀", "rosewood"),
    JSTR_STATIC("トウヒ", "spruce"),
    JSTR_STATIC("チーク", "teak"),
    JSTR_STATIC("胡桃", "walnut"),
    JSTR_STATIC("ゼブラウッド", "zebrawood"),
};

#define NWOOD (sizeof wood / sizeof (char *))

const char *metal[] = {
    JSTR_STATIC("アルミニウム", "aluminum"),
    JSTR_STATIC("ベリリウム", "beryllium"),
    JSTR_STATIC("骨", "bone"),
    JSTR_STATIC("真ちゅう", "brass"),
    JSTR_STATIC("青銅", "bronze"),
    JSTR_STATIC("銅", "copper"),
    JSTR_STATIC("琥珀金", "electrum"),
    JSTR_STATIC("金", "gold"),
    JSTR_STATIC("鉄", "iron"),
    JSTR_STATIC("鉛", "lead"),
    JSTR_STATIC("マグネシウム", "magnesium"),
    JSTR_STATIC("水銀", "mercury"),
    JSTR_STATIC("ニッケル", "nickel"),
    JSTR_STATIC("ピューター", "pewter"),
    JSTR_STATIC("プラチナ", "platinum"),
    JSTR_STATIC("鋼鉄", "steel"),
    JSTR_STATIC("銀", "silver"),
    JSTR_STATIC("シリコン", "silicon"),
    JSTR_STATIC("すず", "tin"),
    JSTR_STATIC("チタン", "titanium"),
    JSTR_STATIC("タングステン", "tungsten"),
    JSTR_STATIC("亜鉛", "zinc"),
};

#define NMETAL (sizeof metal / sizeof (char *))

int cNWOOD = NWOOD;
int cNMETAL = NMETAL;
int cNSTONES = NSTONES;
int cNCOLORS = NCOLORS;

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors(void)
{
    size_t i;
    int j;
    int used[NCOLORS];

    for (i = 0; i < NCOLORS; i++)
	used[i] = FALSE;
    for (i = 0; i < MAXPOTIONS; i++)
    {
	do
	    j = rnd(NCOLORS);
	until (!used[j]);
	used[j] = TRUE;
	p_colors[i] = rainbow[j];
    }
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
#define MAXNAME	40	/* Max number of characters in a name */

static void
free_scroll_name(int which)
{
    if (s_names[which] != NULL)
    {
	free(s_names[which]);
	s_names[which] = NULL;
    }
}

void
jrogue_rebuild_scroll_names(void)
{
    int i, n, id;
    size_t len;
    char *name, *cp;
    const char *sp;

    for (i = 0; i < MAXSCROLLS; i++)
    {
	if (scroll_syll_count[i] <= 0)
	    continue;
	len = 0;
	for (n = 0; n < scroll_syll_count[i]; n++)
	{
	    id = scroll_syll_ids[i][n];
	    if (id < 0 || id >= (int) NSYLLS)
		continue;
	    len += strlen(JSTR(sylls[id].ja, sylls[id].en));
	    if (scroll_syll_breaks[i][n] && n + 1 < scroll_syll_count[i])
		len++;
	}
	name = malloc(len + 1);
	if (name == NULL)
	    continue;
	cp = name;
	for (n = 0; n < scroll_syll_count[i]; n++)
	{
	    id = scroll_syll_ids[i][n];
	    if (id < 0 || id >= (int) NSYLLS)
		continue;
	    sp = JSTR(sylls[id].ja, sylls[id].en);
	    while (*sp)
		*cp++ = *sp++;
	    if (scroll_syll_breaks[i][n] && n + 1 < scroll_syll_count[i])
		*cp++ = ' ';
	}
	*cp = '\0';
	free_scroll_name(i);
	s_names[i] = name;
    }
}

/* Only the kana used by the scroll syllable table occur here: each
 * non-ASCII UTF-8 character occupies two terminal cells.  Count codepoints,
 * not bytes, so a syllable is never shortened in the middle of a character. */
#define MAX_SCROLL_NAME_JA_CELLS 30

static int
scroll_ja_cells(const char *sp)
{
    int cells = 0;

    for (; *sp != '\0'; sp++)
    {
        unsigned char ch = (unsigned char) *sp;
        if ((ch & 0xc0) == 0x80) /* continuation byte */
            continue;
        cells += (ch < 0x80) ? 1 : 2;
    }
    return cells;
}

void
init_names(void)
{
    int nsyl, sid;
    const char *sp;
    size_t english_len, syll_len;
    int i, nwords, n, used, cells;

    for (i = 0; i < MAXSCROLLS; i++)
    {
        /* This first pass deliberately matches the RRP 5.4.5 loops:
         * English syllables determine the 40-character limit; a too-long
         * syllable ends only its WORD, not the remaining words.  No random
         * numbers are drawn while shortening the resulting title. */
        english_len = 0;
        nwords = rnd(3) + 2;
        n = 0;
        while (nwords--)
        {
            nsyl = rnd(3) + 1;
            while (nsyl--)
            {
                sid = rnd((int) NSYLLS);
                sp = sylls[sid].en;
                /* Track the original English buffer position as a length.
                 * This preserves RRP's 40-character limit and random draws
                 * without writing a temporary name into prbuf. */
                syll_len = strlen(sp);
                if (english_len + syll_len > MAXNAME)
                    break;
                english_len += syll_len;
                /* At most four words of three syllables: 12 < 16. */
                scroll_syll_ids[i][n] = sid;
                scroll_syll_breaks[i][n] = FALSE;
                n++;
            }
            english_len++; /* RRP also counts a space after each word. */
            if (n > 0)
                scroll_syll_breaks[i][n - 1] = TRUE;
        }
        /* RRP removes the final space; it is not part of the name. */

        /* Shorten the same syllable sequence for BOTH languages.  The
         * Japanese width includes the spaces between words; keep the
         * longest prefix that fits in 30 cells, without cutting a syllable.
         * All random draws have already taken place above. */
        cells = 0;
        for (used = 0; used < n; used++)
        {
            int extra = scroll_ja_cells(sylls[scroll_syll_ids[i][used]].ja);
            if (used > 0 && scroll_syll_breaks[i][used - 1])
                extra++;
            if (cells + extra > MAX_SCROLL_NAME_JA_CELLS)
                break;
            cells += extra;
        }
        scroll_syll_count[i] = used;
        if (used > 0)
            scroll_syll_breaks[i][used - 1] = FALSE;
    }
    jrogue_rebuild_scroll_names();
}

/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void
init_stones(void)
{
    int used[NSTONES];
    size_t i;
    int j;

    for (i = 0; i < NSTONES; i++)
	used[i] = FALSE;
    for (i = 0; i < MAXRINGS; i++)
    {
	do
	    j = rnd(NSTONES);
	until (!used[j]);
	used[j] = TRUE;
	r_stones[i] = stones[j].st_name;
	ring_info[i].oi_worth += stones[j].st_value;
    }
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
init_materials(void)
{
    size_t i;
    int j;
    const char *str;
    int metused[NMETAL];
    int used[NWOOD];

    for (i = 0; i < NWOOD; i++)
	used[i] = FALSE;
    for (i = 0; i < NMETAL; i++)
	metused[i] = FALSE;
    for (i = 0; i < MAXSTICKS; i++)
    {
	for (;;)
	    if (rnd(2) == 0)
	    {
		j = rnd(NMETAL);
		if (!metused[j])
		{
		    ws_type[i] = JSTR("魔法の棒", "wand");
		    str = metal[j];
		    metused[j] = TRUE;
		    break;
		}
	    }
	    else
	    {
		j = rnd(NWOOD);
		if (!used[j])
		{
		    ws_type[i] = JSTR("つえ", "staff");
		    str = wood[j];
		    used[j] = TRUE;
		    break;
		}
	    }
	ws_made[i] = str;
    }
}

#ifdef MASTER
# define	NT	NUMTHINGS, "things"
# define	MP	MAXPOTIONS, "potions"
# define	MS	MAXSCROLLS, "scrolls"
# define	MR	MAXRINGS, "rings"
# define	MWS	MAXSTICKS, "sticks"
# define	MW	MAXWEAPONS, "weapons"
# define	MA	MAXARMORS, "armor"
#else
# define	NT	NUMTHINGS
# define	MP	MAXPOTIONS
# define	MS	MAXSCROLLS
# define	MR	MAXRINGS
# define	MWS	MAXSTICKS
# define	MW	MAXWEAPONS
# define	MA	MAXARMORS
#endif

/*
 * sumprobs:
 *	Sum up the probabilities for items appearing
 */
void
sumprobs(struct obj_info *info, int bound
#ifdef MASTER
	, char *name
#endif
)
{
#ifdef MASTER
	struct obj_info *start = info;
#endif
    struct obj_info *endp;

    endp = info + bound;
    while (++info < endp)
	info->oi_prob += (info - 1)->oi_prob;
#ifdef MASTER
    badcheck(name, start, bound);
#endif
}

/*
 * init_probs:
 *	Initialize the probabilities for the various items
 */
void
init_probs(void)
{
    sumprobs(things, NT);
    sumprobs(pot_info, MP);
    sumprobs(scr_info, MS);
    sumprobs(ring_info, MR);
    sumprobs(ws_info, MWS);
    sumprobs(weap_info, MW);
    sumprobs(arm_info, MA);
}

#ifdef MASTER
/*
 * badcheck:
 *	Check to see if a series of probabilities sums to 100
 */
void
badcheck(const char *name, const struct obj_info *info, int bound)
{
    const struct obj_info *end;

    if (info[bound - 1].oi_prob == 100)
	return;
    printf("\nBad percentages for %s (bound = %d):\n", name, bound);
    for (end = &info[bound]; info < end; info++)
	printf("%3d%% %s\n", info->oi_prob, info->oi_name);
    printf("[hit RETURN to continue]");
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}
#endif

/*
 * pick_color:
 *	If he is halucinating, pick a random color name and return it,
 *	otherwise return the given color.
 */
const char *
pick_color(const char *col)
{
    return (on(player, ISHALU) ? rainbow[rnd(NCOLORS)] : col);
}


static void
jrogue_capture_color_indexes(int pot_index[], int ring_index[],
    int stick_is_staff[], int stick_index[])
{
    int i, j;

    for (i = 0; i < MAXPOTIONS; i++)
    {
	pot_index[i] = -1;
	if (p_colors[i] == NULL)
	    continue;
	for (j = 0; j < cNCOLORS; j++)
	    if (p_colors[i] == rainbow[j] || strcmp(p_colors[i], rainbow[j]) == 0)
	    {
		pot_index[i] = j;
		break;
	    }
    }

    for (i = 0; i < MAXRINGS; i++)
    {
	ring_index[i] = -1;
	if (r_stones[i] == NULL)
	    continue;
	for (j = 0; j < cNSTONES; j++)
	    if (r_stones[i] == stones[j].st_name
		|| strcmp(r_stones[i], stones[j].st_name) == 0)
	    {
		ring_index[i] = j;
		break;
	    }
    }

    for (i = 0; i < MAXSTICKS; i++)
    {
	stick_is_staff[i] = FALSE;
	stick_index[i] = -1;
	if (ws_made[i] == NULL)
	    continue;

	for (j = 0; j < cNWOOD; j++)
	    if (ws_made[i] == wood[j] || strcmp(ws_made[i], wood[j]) == 0)
	    {
		stick_is_staff[i] = TRUE;
		stick_index[i] = j;
		break;
	    }

	if (stick_index[i] >= 0)
	    continue;

	for (j = 0; j < cNMETAL; j++)
	    if (ws_made[i] == metal[j] || strcmp(ws_made[i], metal[j]) == 0)
	    {
		stick_is_staff[i] = FALSE;
		stick_index[i] = j;
		break;
	    }
    }
}

static void
jrogue_reapply_color_indexes(int pot_index[], int ring_index[],
    int stick_is_staff[], int stick_index[])
{
    int i;

    for (i = 0; i < MAXPOTIONS; i++)
	if (pot_index[i] >= 0 && pot_index[i] < cNCOLORS)
	    p_colors[i] = rainbow[pot_index[i]];

    for (i = 0; i < MAXRINGS; i++)
	if (ring_index[i] >= 0 && ring_index[i] < cNSTONES)
	    r_stones[i] = stones[ring_index[i]].st_name;

    for (i = 0; i < MAXSTICKS; i++)
    {
	if (stick_index[i] < 0)
	    continue;

	if (stick_is_staff[i])
	{
	    if (stick_index[i] < cNWOOD)
	    {
		ws_type[i] = JSTR("つえ", "staff");
		ws_made[i] = wood[stick_index[i]];
	    }
	}
	else
	{
	    if (stick_index[i] < cNMETAL)
	    {
		ws_type[i] = JSTR("魔法の棒", "wand");
		ws_made[i] = metal[stick_index[i]];
	    }
	}
    }
}


void
jrogue_apply_init_language(void)
{
    int pot_index[MAXPOTIONS], ring_index[MAXRINGS];
    int stick_is_staff[MAXSTICKS], stick_index[MAXSTICKS];

    jrogue_capture_color_indexes(pot_index, ring_index, stick_is_staff, stick_index);

    rainbow[0] = JSTR("こはく色", "amber");
    rainbow[1] = JSTR("淡い青緑色", "aquamarine");
    rainbow[2] = JSTR("黒色", "black");
    rainbow[3] = JSTR("青色", "blue");
    rainbow[4] = JSTR("茶色", "brown");
    rainbow[5] = JSTR("透明", "clear");
    rainbow[6] = JSTR("深紅色", "crimson");
    rainbow[7] = JSTR("水色", "cyan");
    rainbow[8] = JSTR("ベージュ色", "ecru");
    rainbow[9] = JSTR("金色", "gold");
    rainbow[10] = JSTR("緑色", "green");
    rainbow[11] = JSTR("灰色", "grey");
    rainbow[12] = JSTR("赤紫色", "magenta");
    rainbow[13] = JSTR("オレンジ色", "orange");
    rainbow[14] = JSTR("ピンク色", "pink");
    rainbow[15] = JSTR("格子柄", "plaid");
    rainbow[16] = JSTR("紫色", "purple");
    rainbow[17] = JSTR("赤色", "red");
    rainbow[18] = JSTR("銀色", "silver");
    rainbow[19] = JSTR("淡い茶色", "tan");
    rainbow[20] = JSTR("みかん色", "tangerine");
    rainbow[21] = JSTR("淡褐色", "topaz");
    rainbow[22] = JSTR("青緑色", "turquoise");
    rainbow[23] = JSTR("朱色", "vermilion");
    rainbow[24] = JSTR("すみれ色", "violet");
    rainbow[25] = JSTR("白色", "white");
    rainbow[26] = JSTR("黄色", "yellow");
    wood[0] = JSTR("アボカド", "avocado wood");
    wood[1] = JSTR("バルサ", "balsa");
    wood[2] = JSTR("竹", "bamboo");
    wood[3] = JSTR("バニヤン", "banyan");
    wood[4] = JSTR("白樺", "birch");
    wood[5] = JSTR("杉", "cedar");
    wood[6] = JSTR("桜", "cherry");
    wood[7] = JSTR("辰砂", "cinnibar");
    wood[8] = JSTR("糸杉", "cypress");
    wood[9] = JSTR("ハナミズキ", "dogwood");
    wood[10] = JSTR("流木", "driftwood");
    wood[11] = JSTR("黒檀", "ebony");
    wood[12] = JSTR("楡", "elm");
    wood[13] = JSTR("ユーカリ", "eucalyptus");
    wood[14] = JSTR("堕落", "fall");
    wood[15] = JSTR("ツガ", "hemlock");
    wood[16] = JSTR("ヒイラギ", "holly");
    wood[17] = JSTR("鉄樹", "ironwood");
    wood[18] = JSTR("ククイノキ", "kukui wood");
    wood[19] = JSTR("マホガニー", "mahogany");
    wood[20] = JSTR("マンザニータ", "manzanita");
    wood[21] = JSTR("楓", "maple");
    wood[22] = JSTR("樫", "oaken");
    wood[23] = JSTR("柿", "persimmon wood");
    wood[24] = JSTR("ピーカン", "pecan");
    wood[25] = JSTR("松", "pine");
    wood[26] = JSTR("ポプラ", "poplar");
    wood[27] = JSTR("アメリカ杉", "redwood");
    wood[28] = JSTR("紫檀", "rosewood");
    wood[29] = JSTR("トウヒ", "spruce");
    wood[30] = JSTR("チーク", "teak");
    wood[31] = JSTR("胡桃", "walnut");
    wood[32] = JSTR("ゼブラウッド", "zebrawood");
    metal[0] = JSTR("アルミニウム", "aluminum");
    metal[1] = JSTR("ベリリウム", "beryllium");
    metal[2] = JSTR("骨", "bone");
    metal[3] = JSTR("真ちゅう", "brass");
    metal[4] = JSTR("青銅", "bronze");
    metal[5] = JSTR("銅", "copper");
    metal[6] = JSTR("琥珀金", "electrum");
    metal[7] = JSTR("金", "gold");
    metal[8] = JSTR("鉄", "iron");
    metal[9] = JSTR("鉛", "lead");
    metal[10] = JSTR("マグネシウム", "magnesium");
    metal[11] = JSTR("水銀", "mercury");
    metal[12] = JSTR("ニッケル", "nickel");
    metal[13] = JSTR("ピューター", "pewter");
    metal[14] = JSTR("プラチナ", "platinum");
    metal[15] = JSTR("鋼鉄", "steel");
    metal[16] = JSTR("銀", "silver");
    metal[17] = JSTR("シリコン", "silicon");
    metal[18] = JSTR("すず", "tin");
    metal[19] = JSTR("チタン", "titanium");
    metal[20] = JSTR("タングステン", "tungsten");
    metal[21] = JSTR("亜鉛", "zinc");
    stones[0].st_name = JSTR("めのう", "agate");
    stones[1].st_name = JSTR("アレキサンドライト", "alexandrite");
    stones[2].st_name = JSTR("紫水晶", "amethyst");
    stones[3].st_name = JSTR("カーネリアン", "carnelian");
    stones[4].st_name = JSTR("ダイヤ", "diamond");
    stones[5].st_name = JSTR("エメラルド", "emerald");
    stones[6].st_name = JSTR("ゲルマニウム", "germanium");
    stones[7].st_name = JSTR("御影石", "granite");
    stones[8].st_name = JSTR("ざくろ石", "garnet");
    stones[9].st_name = JSTR("ひすい", "jade");
    stones[10].st_name = JSTR("クリプトナイト", "kryptonite");
    stones[11].st_name = JSTR("るり", "lapis lazuli");
    stones[12].st_name = JSTR("月長石", "moonstone");
    stones[13].st_name = JSTR("黒曜石", "obsidian");
    stones[14].st_name = JSTR("オニキス", "onyx");
    stones[15].st_name = JSTR("オパール", "opal");
    stones[16].st_name = JSTR("真珠", "pearl");
    stones[17].st_name = JSTR("ペリドット", "peridot");
    stones[18].st_name = JSTR("ルビー", "ruby");
    stones[19].st_name = JSTR("サファイア", "sapphire");
    stones[20].st_name = JSTR("安タンタル石", "stibotantalite");
    stones[21].st_name = JSTR("虎目石", "tiger eye");
    stones[22].st_name = JSTR("トパーズ", "topaz");
    stones[23].st_name = JSTR("トルコ石", "turquoise");
    stones[24].st_name = JSTR("ターフェアイト", "taaffeite");
    stones[25].st_name = JSTR("ジルコン", "zircon");

    jrogue_reapply_color_indexes(pot_index, ring_index, stick_is_staff, stick_index);
}
