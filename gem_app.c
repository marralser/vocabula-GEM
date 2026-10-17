#ifdef __WATCOMC__
#include "ppdgem.h"
#include <dos.h>
#else
#include "gem_compat.h"
#define FP_OFF(pointer) ((void)(pointer), 0)
#define FP_SEG(pointer) ((void)(pointer), 0)
#endif
#include "lesson.h"
#include "stats.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WIN_KIND (NAME | CLOSER | MOVER | FULLER | SIZER)
#define MAX_INPUT 80

#define MENU_DESK 3
#define MENU_DESK_ABOUT 6
#define MENU_OBJECTS 14

typedef struct { WORD x, y, w, h; } Rect;

static WORD vdi_handle, window_handle;
static WORD work_x, work_y, work_w, work_h;
static WORD desk_x, desk_y, desk_w, desk_h;
static WORD char_w, char_h, box_w, box_h;
static Lesson lesson;
static unsigned order[MAX_CARDS], mistakes[MAX_CARDS];
static unsigned card_pos, card_total, mistake_count;
static unsigned hints, factual_tries;
static int practice_mode, direction, revealed, card_failed, repeating;
static int synonym_available;
static SessionStats stats;
static char lesson_name[128], sum_name[140], car_name[140], err_name[140];
static char answer[MAX_INPUT], proposal[MAX_TEXT], last_wrong[MAX_TEXT];
static char message[160] = "Choose File / Open lesson to begin.";
static char message_second[160] = "";
static int message_bold = 0;
static WORD message_color = 1;
static char student_name[21] = "Student";
static char program_name[] = "Vocabula GEM 0.1";
static Rect enter_button, hint_button, reveal_button, synonym_button, options_button, stop_button;
static int running = 1, lesson_loaded = 0, entering_name = 0, exit_after_name = 0;
static int sound_enabled = 1, case_sensitive = 0;
static OBJECT menu_tree[MENU_OBJECTS];
static char menu_desk[] = " Desk ";
static char menu_desk_about[] = "  About Vocabula   ";
static char menu_separator[] = "--------------------";
static char menu_empty[] = "                    ";

static int select_lesson(void);
static int require_lesson(void);

static int has_voc_extension(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot || strlen(dot) != 4) return 0;
    return dot[1] == 'v' || dot[1] == 'V' ?
        (dot[2] == 'o' || dot[2] == 'O') &&
        (dot[3] == 'c' || dot[3] == 'C') : 0;
}

static void menu_object(WORD number, WORD next, WORD head, WORD tail,
    UWORD type, UWORD flags, LPVOID spec, WORD x, WORD y, WORD w, WORD h)
{
    OBJECT *object = &menu_tree[number];
    object->ob_next = next; object->ob_head = head; object->ob_tail = tail;
    object->ob_type = type; object->ob_flags = flags; object->ob_state = NORMAL;
    object->ob_spec = spec; object->ob_x = x; object->ob_y = y;
    object->ob_width = w; object->ob_height = h;
}

static void create_menu(WORD screen_w, WORD screen_h, WORD menu_w, WORD menu_h)
{
    WORD i;
    LPVOID box_spec = (LPVOID)0x00ff1100L;
    (void)menu_w; (void)menu_h;

    /* These are Resource Construction Set character coordinates, not pixels.
       The high byte is the signed pixel adjustment and the low byte is the
       character-cell count.  rsrc_obfix() converts them for the active mode. */
    menu_object(0, -1, 1, 4, G_IBOX, NONE, 0L, 0, 0, 80, 25);
    menu_object(1, 4, 2, 2, G_BOX, NONE, (LPVOID)0x00001100L,
        0, 0, 80, 0x0201);
    menu_object(2, 1, 3, 3, G_IBOX, NONE, 0L,
        2, 0, 15, 0x0301);
    menu_object(3, 2, -1, -1, G_TITLE, NONE, menu_desk,
        0, 0, 6, 0x0301);
    menu_object(4, 0, 5, 5, G_IBOX, NONE, 0L,
        0, 0x0301, 80, 23);
    menu_object(5, 4, 6, 13, G_BOX, NONE, box_spec,
        2, 0, 20, 8);
    menu_object(6, 7, -1, -1, G_STRING, NONE, menu_desk_about,
        0, 0, 20, 1);
    menu_object(7, 8, -1, -1, G_STRING, NONE, menu_separator,
        0, 1, 20, 1);
    menu_object(8, 9, -1, -1, G_STRING, NONE, menu_empty,
        0, 2, 20, 1);
    menu_object(9, 10, -1, -1, G_STRING, NONE, menu_empty,
        0, 3, 20, 1);
    menu_object(10, 11, -1, -1, G_STRING, NONE, menu_empty,
        0, 4, 20, 1);
    menu_object(11, 12, -1, -1, G_STRING, NONE, menu_empty,
        0, 5, 20, 1);
    menu_object(12, 13, -1, -1, G_STRING, NONE, menu_empty,
        0, 6, 20, 1);
    menu_object(13, 5, -1, -1, G_STRING, LASTOB, menu_empty,
        0, 7, 20, 1);
    menu_tree[7].ob_state = DISABLED;
    menu_tree[13].ob_state = DISABLED;

    for (i = 0; i < MENU_OBJECTS; ++i) rsrc_obfix(menu_tree, i);
    menu_tree[0].ob_width = screen_w; menu_tree[0].ob_height = screen_h;
    menu_tree[1].ob_width = screen_w;
    menu_tree[2].ob_width = screen_w - menu_tree[2].ob_x * 2;
    menu_tree[4].ob_width = screen_w;
    menu_tree[4].ob_height = screen_h - menu_tree[4].ob_y;
}

static void show_about(void)
{
    form_alert(1,
        "[1][Vocabula 0.1 for GEM.|Derived from Vocabula for DOS.|Open source and free to use.|https://github.com/marralser/|vocabula-GEM][OK]");
}

static void show_options(void)
{
    char alert[180]; int choice;
    do {
        sprintf(alert,
            "[1][Vocabula options|Sound: %s|Case sensitive: %s|Choose a setting to change it.][Sound|Case|Done]",
            sound_enabled ? "On" : "Off", case_sensitive ? "On" : "Off");
        choice = form_alert(3, alert);
        if (choice == 1) sound_enabled = !sound_enabled;
        else if (choice == 2) case_sensitive = !case_sensitive;
    } while (choice == 1 || choice == 2);
}

#ifdef __WATCOMC__
static unsigned char port_read(unsigned port);
#pragma aux port_read = "in al,dx" parm [dx] value [al] modify exact [al];
static void port_write(unsigned port, unsigned char value);
#pragma aux port_write = "out dx,al" parm [dx] [al];
#endif

static int inside(WORD x, WORD y, const Rect *r)
{ return x >= r->x && y >= r->y && x < r->x + r->w && y < r->y + r->h; }

static int intersect(Rect *a, const Rect *b)
{
    WORD x1 = a->x > b->x ? a->x : b->x;
    WORD y1 = a->y > b->y ? a->y : b->y;
    WORD x2 = a->x + a->w < b->x + b->w ? a->x + a->w : b->x + b->w;
    WORD y2 = a->y + a->h < b->y + b->h ? a->y + a->h : b->y + b->h;
    if (x2 <= x1 || y2 <= y1) return 0;
    a->x = x1; a->y = y1; a->w = x2 - x1; a->h = y2 - y1; return 1;
}

static void fill(WORD x, WORD y, WORD w, WORD h, WORD color)
{
    WORD p[4];
    p[0] = x; p[1] = y; p[2] = x + w - 1; p[3] = y + h - 1;
    vsf_interior(vdi_handle, 1); vsf_color(vdi_handle, color); v_bar(vdi_handle, p);
}

static void line(WORD x1, WORD y1, WORD x2, WORD y2, WORD color)
{
    WORD p[4]; p[0] = x1; p[1] = y1; p[2] = x2; p[3] = y2;
    vsl_color(vdi_handle, color); v_pline(vdi_handle, 2, p);
}

static void text(WORD x, WORD y, const char *s, WORD color)
{ vst_color(vdi_handle, color); v_gtext(vdi_handle, x, y, (char *)s); }

static void centered_text(WORD y, const char *s, WORD color)
{
    WORD width = (WORD)(strlen(s) * char_w);
    WORD x = width < work_w - 16 ? work_x + (work_w - width) / 2 : work_x + 8;
    text(x, y, s, color);
}

static void frame(WORD x, WORD y, WORD w, WORD h)
{
    line(x, y, x + w - 1, y, 1);
    line(x, y, x, y + h - 1, 1);
    line(x, y + h - 1, x + w - 1, y + h - 1, 1);
    line(x + w - 1, y, x + w - 1, y + h - 1, 1);
}

static void button(Rect *r, WORD x, WORD y, WORD w, const char *label, int is_default)
{
    WORD label_width = (WORD)(strlen(label) * char_w);
    r->x = x; r->y = y; r->w = w; r->h = char_h + 12;
    fill(x, y, w, r->h, 0); frame(x, y, w, r->h); frame(x + 2, y + 2, w - 4, r->h - 4);
    if (is_default) frame(x + 4, y + 4, w - 8, r->h - 8);
    vst_effects(vdi_handle, is_default ? 1 : 0);
    text(x + (w - label_width) / 2, y + char_h + 3, label, 1);
    vst_effects(vdi_handle, 0);
}

static void pause_milliseconds(unsigned milliseconds)
{
    clock_t until = clock() + (clock_t)((unsigned long)CLOCKS_PER_SEC * milliseconds / 1000UL);
    while (clock() < until) { }
}

static void tone(unsigned frequency, unsigned milliseconds)
{
#ifdef __WATCOMC__
    unsigned divisor = (unsigned)(1193180UL / frequency);
    unsigned char speaker = port_read(0x61);
    port_write(0x43, 0xb6);
    port_write(0x42, (unsigned char)(divisor & 0xff));
    port_write(0x42, (unsigned char)(divisor >> 8));
    port_write(0x61, (unsigned char)(speaker | 3));
    pause_milliseconds(milliseconds); port_write(0x61, speaker);
#else
    (void)frequency; fputc('\a', stdout); fflush(stdout); pause_milliseconds(milliseconds);
#endif
}

static void sound_correct(void) { if (sound_enabled) tone(880, 90); }
static void sound_wrong(void) { if (sound_enabled) tone(220, 220); }
static void sound_hint(void) { if (sound_enabled) tone(660, 55); }

static void side_text(const Side *side, char *out, unsigned size)
{
    unsigned i; out[0] = '\0';
    for (i = 0; i < side->count; ++i) {
        if (strlen(out) + strlen(side->text[i]) + 3 >= size) break;
        if (i) strcat(out, ", ");
        strcat(out, side->text[i]);
    }
}

static void current_sides(const Side **question, Side **expected)
{
    Card *card = lesson.cards[order[card_pos]];
    *question = direction ? &card->right : &card->left;
    *expected = direction ? &card->left : &card->right;
}

static void draw_screen(void)
{
    WORD y, x, bar_w, progress_w, button_w, gap, group_w;
    char temp[180], number[16]; const Side *question; Side *expected;
    fill(work_x, work_y, work_w, work_h, 0);
    if (!lesson_loaded) {
        centered_text(work_y + char_h * 4, "Please select a .VOC lesson to practise with Vocabula.", 1);
        centered_text(work_y + char_h * 6, message, 1); return;
    }
    if (entering_name) {
        centered_text(work_y + char_h * 3, "High Score Board", 4);
        sprintf(temp, "Answered: %u    Mistakes: %u    Score: %d", stats.asked, stats.wrong, stats.score);
        centered_text(work_y + char_h * 6, temp, 1);
        centered_text(work_y + char_h * 9, "Please enter your name for the high score board:", 1);
        x = work_x + 60; y = work_y + char_h * 12;
        fill(x, y, work_w - 120, char_h * 3, 0); frame(x, y, work_w - 120, char_h * 3);
        text(x + 10, y + char_h * 2, answer, 1);
        button(&enter_button, work_x + (work_w - 90) / 2, work_y + char_h * 16, 90, "Enter", 1);
        hint_button.w = reveal_button.w = synonym_button.w = options_button.w = stop_button.w = 0;
        return;
    }
    current_sides(&question, &expected); (void)expected;
    sprintf(temp, "%s -> %s", direction ? lesson.right_name : lesson.left_name,
            direction ? lesson.left_name : lesson.right_name);
    centered_text(work_y + char_h * 2, temp, 4);

    bar_w = work_w > 180 ? work_w - 140 : work_w - 60;
    x = work_x + (work_w - bar_w) / 2;
    y = work_y + char_h * 3;
    sprintf(number, "%u", card_pos + 1); text(x - (WORD)(strlen(number) * char_w) - 8, y + char_h, number, 1);
    sprintf(number, "%u", card_total); text(x + bar_w + 8, y + char_h, number, 1);
    fill(x, y + 4, bar_w, 10, 0); frame(x, y + 4, bar_w, 10);
    progress_w = card_total ? (WORD)((long)(bar_w - 4) * (card_pos + 1) / card_total) : 0;
    if (progress_w) fill(x + 2, y + 6, progress_w, 6, 4);

    sprintf(temp, "Score: %d", stats.score);
    centered_text(work_y + char_h * 5, temp, 1);

    side_text(question, temp, sizeof(temp));
    x = work_x + 20; y = work_y + char_h * 7;
    fill(x, y, work_w - 40, char_h * 4, 0); frame(x, y, work_w - 40, char_h * 4);
    text(x + 10, y + char_h + 2, "Translate:", 1);
    vst_effects(vdi_handle, 1);
    centered_text(y + char_h * 3, temp, 4);
    vst_effects(vdi_handle, 0);

    y = work_y + char_h * 12;
    fill(x, y, work_w - 40, char_h * 3, 0); frame(x, y, work_w - 40, char_h * 3);
    text(x + 10, y + char_h + 2, "Answer:", 1);
    text(x + 10, y + char_h * 2 + 4, answer, 1);

    y = work_y + char_h * 16;
    gap = 8; button_w = 72; group_w = button_w * 6 + gap * 5;
    x = work_x + (work_w - group_w) / 2;
    button(&enter_button, x, y, button_w, "Enter", 1);
    button(&hint_button, x + button_w + gap, y, button_w, "Hint", 0);
    button(&reveal_button, x + (button_w + gap) * 2, y, button_w, "Reveal", 0);
    button(&synonym_button, x + (button_w + gap) * 3, y, button_w, "Synonym", 0);
    button(&options_button, x + (button_w + gap) * 4, y, button_w, "Options", 0);
    button(&stop_button, x + (button_w + gap) * 5, y, button_w, "Stop", 0);
    vst_effects(vdi_handle, message_bold ? 1 : 0);
    centered_text(y + char_h * 3, message, message_color);
    vst_effects(vdi_handle, 0);
    if (*message_second) centered_text(y + char_h * 4 + 2, message_second, 1);
}

static void redraw(WORD x, WORD y, WORD w, WORD h)
{
    Rect wanted, visible; WORD clip[4];
    wanted.x = x; wanted.y = y; wanted.w = w; wanted.h = h;
    wind_get(window_handle, WF_FIRSTXYWH, &visible.x, &visible.y, &visible.w, &visible.h);
    while (visible.w && visible.h) {
        Rect part = visible;
        if (intersect(&part, &wanted)) {
            clip[0] = part.x; clip[1] = part.y; clip[2] = part.x + part.w - 1; clip[3] = part.y + part.h - 1;
            vs_clip(vdi_handle, 1, clip); draw_screen();
        }
        wind_get(window_handle, WF_NEXTXYWH, &visible.x, &visible.y, &visible.w, &visible.h);
    }
    vs_clip(vdi_handle, 0, clip);
}

/* Typing changes only the answer field.  Redrawing the complete GEM window
   for every character is conspicuously slow on period hardware. */
static void redraw_answer(void)
{
    WORD x, y, w, clip[4];
    if (!lesson_loaded) return;
    y = work_y + char_h * 12;
    x = entering_name ? work_x + 60 : work_x + 20;
    w = entering_name ? work_w - 120 : work_w - 40;
    clip[0] = x + 2; clip[1] = y + char_h + 3;
    clip[2] = x + w - 3; clip[3] = y + char_h * 3 - 2;
    wind_update(1); vs_clip(vdi_handle, 1, clip);
    fill(clip[0], clip[1], clip[2] - clip[0] + 1, clip[3] - clip[1] + 1, 0);
    text(x + 10, y + char_h * 2 + (entering_name ? 0 : 4), answer, 1);
    vs_clip(vdi_handle, 0, clip); wind_update(0);
}

static void shuffle(unsigned *items, unsigned count)
{
    unsigned i, j, t; for (i = 0; i < count; ++i) items[i] = i;
    while (count > 1) { j = (unsigned)(rand() % count); --count;
        t = items[count]; items[count] = items[j]; items[j] = t; }
}

static void start_card(void)
{
    answer[0] = proposal[0] = last_wrong[0] = '\0'; hints = factual_tries = 0;
    revealed = card_failed = synonym_available = 0;
    message_bold = 0; message_color = 1; message_second[0] = '\0';
    strcpy(message, "Type an answer, then press Enter.");
    direction = practice_mode == 2 || (practice_mode == 3 && (rand() & 1));
}

static void show_high_scores(void)
{
    HighScore scores[10]; unsigned count, page, first, last, i, pages;
    char alert[360], line_text[64], button_text[16];
    count = stats_high_scores(sum_name, scores, 10);
    if (!count) { form_alert(1, "[1][Hall of Fame|No scores saved yet.][Continue]"); return; }
    pages = (count + 3) / 4;
    for (page = 0; page < pages; ++page) {
        first = page * 4; last = first + 4; if (last > count) last = count;
        sprintf(alert, "[1][Hall of Fame %u/%u", page + 1, pages);
        for (i = first; i < last; ++i) {
            sprintf(line_text, "|%u %.16s %.9s %d", i + 1, scores[i].date, scores[i].name, scores[i].score);
            if (strlen(alert) + strlen(line_text) < sizeof(alert) - 16) strcat(alert, line_text);
        }
        strcpy(button_text, page + 1 < pages ? "Next" : "Continue");
        strcat(alert, "]["); strcat(alert, button_text); strcat(alert, "]"); form_alert(1, alert);
    }
}

static void begin_score_entry(int exit_after)
{
    entering_name = 1; exit_after_name = exit_after; answer[0] = '\0';
    message_bold = 0; message_color = 1; message_second[0] = '\0';
    strcpy(message, "Enter your name for the high score board.");
}

static void complete_score_entry(void)
{
    int choice;
    if (*answer) {
        strncpy(student_name, answer, sizeof(student_name) - 1);
        student_name[sizeof(student_name) - 1] = '\0';
    } else strcpy(student_name, "Student");
    stats_save_session(sum_name, student_name, &stats);
    show_high_scores(); lesson_free(&lesson); lesson_loaded = 0; entering_name = 0; answer[0] = '\0';
    choice = form_alert(exit_after_name ? 2 : 1,
        "[1][Thanks for practising with|Vocabula for GEM.|Press New lesson to start over,|or Exit to close the program.][New lesson|Exit]");
    if (choice == 1) {
        if (!select_lesson()) require_lesson();
    } else running = 0;
}

static void finish_session(void)
{
    char alert[220]; int repeat = 0;
    sprintf(alert, "[1][Lesson complete!|Answered: %u  Mistakes: %u|Score: %d][OK]",
            stats.asked, stats.wrong, stats.score);
    form_alert(1, alert);
    if (mistake_count)
        repeat = form_alert(1, "[2][Would you like to repeat|the mistakes now?][Yes|No]") == 1;
    if (repeat) {
        unsigned i; card_total = mistake_count;
        for (i = 0; i < card_total; ++i) order[i] = mistakes[i];
        mistake_count = 0; card_pos = 0; repeating = 1; start_card(); return;
    }
    begin_score_entry(0);
}

static void next_card(void)
{
    ++card_pos;
    if (card_pos >= card_total) finish_session(); else start_card();
}

static void record_correct(const Side *question, Side *expected, AnswerKind kind, unsigned match)
{
    static const char *praise[] = {
        "Excellent - that is correct!",
        "Well done!",
        "Yes! You got it.",
        "Perfect!",
        "Super - keep going!",
        "You win this card!"
    };
    unsigned number = order[card_pos];
    char alert[220];
    if (card_failed || revealed) {
        ++stats.asked; ++stats.wrong; stats.score -= 2;
        stats_record_card(car_name, number, 0);
        stats_record_error(err_name, number, question->text[0], last_wrong, expected);
        if (!repeating && mistake_count < MAX_CARDS) mistakes[mistake_count++] = number;
        strcpy(message, "Correct now. This card will be practised again.");
    } else {
        ++stats.asked; ++stats.right; stats.score += hints ? 1 : 2;
        stats_record_card(car_name, number, 1);
        if (kind == ANSWER_CAPITALIZATION) sprintf(message, "Correct. Please note: %s", expected->text[match]);
        else if (kind == ANSWER_APOSTROPHE) sprintf(message, "Correct. Use this apostrophe: %s", expected->text[match]);
        else if (kind == ANSWER_TYPO) sprintf(message, "Typo! Correct spelling: %s", expected->text[match]);
        else strcpy(message, praise[rand() % (sizeof(praise) / sizeof(praise[0]))]);
    }
    message_color = 1; message_bold = 0; message_second[0] = '\0';
    sound_correct();
    sprintf(alert, "[1][%.70s][Continue]", message);
    form_alert(1, alert); next_card();
}

static void check_answer(void)
{
    AnswerResult result; const Side *question; Side *expected;
    if (entering_name) { complete_score_entry(); return; }
    if (!lesson_loaded || !*answer) return;
    current_sides(&question, &expected); answer_assess(answer, expected, &result);
    if (case_sensitive && result.kind == ANSWER_CAPITALIZATION) result.kind = ANSWER_SPELLING;
    if (result.kind != ANSWER_WRONG && result.kind != ANSWER_SPELLING) {
        record_correct(question, expected, result.kind, result.match); answer[0] = '\0'; return;
    }
    strncpy(proposal, answer, sizeof(proposal) - 1); proposal[sizeof(proposal) - 1] = '\0';
    if (result.kind == ANSWER_SPELLING) {
        synonym_available = 0;
        sound_wrong();
        message_bold = 0; message_color = 1; message_second[0] = '\0';
        sprintf(message, "Please retype the correctly spelled answer.");
        {
            char spelling_alert[220];
            sprintf(spelling_alert, "[1][That looks like a spelling mistake.|The correct spelling is:|%.60s|Please type it again.][OK]", expected->text[result.match]);
            form_alert(1, spelling_alert);
        }
    } else {
        sound_wrong();
        ++factual_tries; card_failed = 1;
        synonym_available = 1;
        strncpy(last_wrong, answer, sizeof(last_wrong) - 1); last_wrong[sizeof(last_wrong) - 1] = '\0';
        if (factual_tries >= 3) {
            message_bold = 0; message_color = 1; message_second[0] = '\0';
            revealed = 1; synonym_available = 0;
            sprintf(message, "The answer is %s. Please type it now.", expected->text[0]);
            form_alert(1, "[3][That was wrong three times.|The correct answer is now shown.|Please type it yourself.][OK]");
        } else {
            message_bold = 0; message_color = 1; message_second[0] = '\0';
            strcpy(message, "Please type another answer, or use Synonym if yours is valid.");
            form_alert(1, "[3][Oh no, this seems wrong.|Please try again.|If it is right after all,|press Synonym to save it.][OK]");
        }
    }
    answer[0] = '\0';
}

static void show_hint(void)
{
    const Side *question; Side *expected; char hint[100]; unsigned i, n, len;
    if (!lesson_loaded) return;
    current_sides(&question, &expected); (void)question;
    ++hints; ++stats.hints; --stats.score; len = (unsigned)strlen(expected->text[0]); n = hints * 2;
    if (n > len) n = len;
    strcpy(hint, "Hint: ");
    for (i = 0; i < len && i + 7 < sizeof(hint); ++i)
        hint[i + 6] = i < n || isspace((unsigned char)expected->text[0][i]) ? expected->text[0][i] : '_';
    hint[i + 6] = '\0';
    message_bold = 0; message_color = 4; message_second[0] = '\0'; strcpy(message, hint);
    sound_hint();
}

static void reveal_answer(void)
{
    const Side *question; Side *expected; char alert[180];
    if (!lesson_loaded) return;
    current_sides(&question, &expected); (void)question;
    revealed = card_failed = 1; synonym_available = 0;
    strcpy(last_wrong, "[revealed]"); proposal[0] = '\0';
    sprintf(alert, "[1][The answer is:|%.60s|Please type it yourself.][Continue]", expected->text[0]);
    form_alert(1, alert);
    message_bold = 0; message_color = 1; message_second[0] = '\0';
    strcpy(message, "Now type the revealed answer to help remember it.");
}

static void save_proposal(void)
{
    const Side *question; Side *expected; char error[160], alert[320];
    if (!lesson_loaded) return;
    if (!synonym_available || !*proposal) {
        form_alert(1,
            "[1][A synonym can be added after|a factually wrong answer.|It is not available for a typo|or spelling mistake.][OK]");
        return;
    }
    current_sides(&question, &expected);
    sprintf(alert,
        "[2][The lesson file suggests:|%.22s = %.22s|You entered: %.30s|Save this as a synonym and|count the answer as correct?][Save|Cancel]",
        question->text[0], expected->text[0], proposal);
    if (form_alert(2, alert) != 1) return;
    if (expected->count >= MAX_SYNONYMS || strlen(proposal) >= MAX_TEXT) {
        form_alert(1, "[3][This card cannot hold|another synonym.][OK]"); return;
    }
    strcpy(expected->text[expected->count++], proposal);
    if (!lesson_save(lesson_name, &lesson, error, sizeof(error))) {
        --expected->count; form_alert(1, "[3][The synonym could not be saved.][OK]"); return;
    }
    strcpy(answer, proposal); proposal[0] = '\0'; synonym_available = 0;
    card_failed = revealed = 0; factual_tries = 0;
    record_correct(question, expected, ANSWER_EXACT, expected->count - 1); answer[0] = '\0';
}

static int select_lesson(void)
{
    char path[128] = "VOC\\*.VOC", file[14] = ""; char *slash; WORD button;
    char error[160], alert[220], direction_alert[220]; int mode;
    form_alert(1, "[1][Please choose a lesson file.][Continue]");
    if (!fsel_input(path, file, &button) || !button || !*file) return 0;
    if (!has_voc_extension(file)) {
        form_alert(1,
            "[3][This is not a .VOC lesson file.|Please choose a filename|ending in .VOC.][OK]");
        return 0;
    }
    slash = strrchr(path, '\\'); if (slash) slash[1] = '\0'; else path[0] = '\0';
    snprintf(lesson_name, sizeof(lesson_name), "%s%s", path, file);
    if (lesson_loaded) lesson_free(&lesson);
    if (!lesson_load(lesson_name, &lesson, error, sizeof(error))) {
        sprintf(alert, "[3][Could not load lesson.|%s][OK]", error); form_alert(1, alert); return 0;
    }
    sprintf(direction_alert, "[1][How should we practise?][%.12s -> %.12s|%.12s -> %.12s|Mixed]",
            lesson.left_name, lesson.right_name, lesson.right_name, lesson.left_name);
    mode = form_alert(1, direction_alert);
    practice_mode = mode < 1 || mode > 3 ? 1 : mode;
    memset(&stats, 0, sizeof(stats)); card_total = lesson.count; card_pos = mistake_count = 0; repeating = 0;
    shuffle(order, card_total); stats_names(lesson_name, sum_name, car_name, err_name);
    lesson_loaded = 1; start_card(); return 1;
}

static int require_lesson(void)
{
    int choice;
    while (running && !lesson_loaded) {
        choice = form_alert(1,
            "[1][You need to select a .VOC lesson|to practise with Vocabula.][Choose lesson|Exit]");
        if (choice != 1) { running = 0; return 0; }
        if (select_lesson()) return 1;
    }
    return lesson_loaded;
}

static void stop_session(void)
{
    char alert[220];
    if (!lesson_loaded) { running = 0; return; }
    sprintf(alert, "[2][Answered: %u  Mistakes: %u|Score: %d|Stop practising?][Stop|Continue]",
            stats.asked, stats.wrong, stats.score);
    if (form_alert(2, alert) == 1) {
        begin_score_entry(1);
    }
}

static void close_program(void)
{
    char alert[220];
    if (!lesson_loaded) { running = 0; return; }
    sprintf(alert,
        "[2][Answered: %u  Mistakes: %u|Score: %d|Stop practising and exit Vocabula?][Stop|Continue]",
        stats.asked, stats.wrong, stats.score);
    if (form_alert(2, alert) == 1) running = 0;
}

static void key_event(WORD key)
{
    unsigned ascii = key & 0xff, scan = (key >> 8) & 0xff, len = (unsigned)strlen(answer);
    if (ascii == 15) { show_options(); return; }
    if (!entering_name && scan == 59) { show_hint(); redraw(work_x, work_y, work_w, work_h); return; }
    if (!entering_name && scan == 60) { reveal_answer(); redraw(work_x, work_y, work_w, work_h); return; }
    if (!entering_name && scan == 61) { save_proposal(); if (running) redraw(work_x, work_y, work_w, work_h); return; }
    if (ascii == 27) { stop_session(); if (running) redraw(work_x, work_y, work_w, work_h); return; }
    if (ascii == 13) { check_answer(); if (running) redraw(work_x, work_y, work_w, work_h); return; }
    if (!lesson_loaded) return;
    if (ascii == 8 && len) { answer[len - 1] = '\0'; redraw_answer(); }
    else if (ascii >= 32 && ascii <= 255 && len + 1 < sizeof(answer) && (!entering_name || len < 20)) {
        answer[len] = (char)ascii; answer[len + 1] = '\0'; redraw_answer();
    }
}

WORD GEMAIN(WORD argc, char *argv[])
{
    WORD work_in[11], work_out[57], i, event, event_flags, msg[8];
    WORD text_width, text_height, cell_width, cell_height;
    WORD full_x, full_y, full_w, full_h;
    WORD menu_char_w, menu_char_h;
    WORD mouse_in_work = 0;
    UWORD mx, my, buttons, shifts, key, clicks;
    (void)argc; (void)argv;
    if (appl_init(NULL) < 0) return 1;
    vdi_handle = graf_handle(&char_w, &char_h, &box_w, &box_h);
    menu_char_w = char_w; menu_char_h = char_h;
    for (i = 0; i < 10; ++i) work_in[i] = 1;
    work_in[10] = 2;
    v_opnvwk(work_in, &vdi_handle, work_out);
    create_menu(work_out[0] + 1, work_out[1] + 1, menu_char_w, menu_char_h);
    menu_bar(menu_tree, TRUE);
    vst_point(vdi_handle, 12, &text_width, &text_height, &cell_width, &cell_height);
    char_w = cell_width; char_h = cell_height;
    wind_get(0, WF_WORKXYWH, &desk_x, &desk_y, &desk_w, &desk_h);
    window_handle = wind_create(WIN_KIND, desk_x, desk_y, desk_w, desk_h);
    wind_set(window_handle, WF_NAME, FP_OFF(program_name), FP_SEG(program_name), 0, 0);
    wind_open(window_handle, desk_x + 12, desk_y + 12, desk_w - 24, desk_h - 24);
    wind_get(window_handle, WF_WORKXYWH, &work_x, &work_y, &work_w, &work_h);
    graf_mouse(ARROW, 0L); srand((unsigned)time(NULL));
    if (!select_lesson()) require_lesson();
    while (running) {
        /* Do not request button events while the pointer is outside the
           window work area.  GEM must own menu-bar and window-decoration
           clicks so that its normal press/track/release handling works. */
        event_flags = MU_MESAG | MU_KEYBD | MU_M1;
        if (mouse_in_work) event_flags |= MU_BUTTON;
        event = evnt_multi(event_flags,
            1, 1, 1,
            mouse_in_work ? 1 : 0, work_x, work_y, work_w, work_h,
            0, 0, 0, 0, 0, msg, 0, 0,
            &mx, &my, &buttons, &shifts, &key, &clicks);
        if (event & MU_M1)
            mouse_in_work = (WORD)mx >= work_x && (WORD)my >= work_y &&
                (WORD)mx < work_x + work_w && (WORD)my < work_y + work_h;
        if (event & MU_KEYBD) key_event(key);
        if ((event & MU_BUTTON) && mouse_in_work) {
            if (inside(mx, my, &enter_button)) check_answer();
            else if (inside(mx, my, &hint_button)) show_hint();
            else if (inside(mx, my, &reveal_button)) reveal_answer();
            else if (inside(mx, my, &synonym_button)) save_proposal();
            else if (inside(mx, my, &options_button)) show_options();
            else if (inside(mx, my, &stop_button)) stop_session();
            if (running) redraw(work_x, work_y, work_w, work_h);
        }
        if (event & MU_MESAG) switch (msg[0]) {
        case MN_SELECTED:
            if (msg[3] == MENU_DESK && (msg[4] == 1 || msg[4] == MENU_DESK_ABOUT))
                show_about();
            menu_tnormal(menu_tree, msg[3], TRUE);
            break;
        case WM_REDRAW: redraw(msg[4], msg[5], msg[6], msg[7]); break;
        case WM_TOPPED: wind_set(window_handle, WF_TOP, 0, 0, 0, 0); break;
        case WM_CLOSED: close_program(); break;
        case WM_MOVED: case WM_SIZED:
            wind_set(window_handle, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7]);
            wind_get(window_handle, WF_WORKXYWH, &work_x, &work_y, &work_w, &work_h);
            redraw(work_x, work_y, work_w, work_h); break;
        case WM_FULLED:
            wind_get(0, WF_WORKXYWH, &full_x, &full_y, &full_w, &full_h);
            wind_set(window_handle, WF_CURRXYWH, full_x, full_y, full_w, full_h);
            wind_get(window_handle, WF_WORKXYWH, &work_x, &work_y, &work_w, &work_h);
            redraw(work_x, work_y, work_w, work_h); break;
        }
    }
    if (lesson_loaded) lesson_free(&lesson);
    menu_bar(menu_tree, FALSE);
    wind_close(window_handle); wind_delete(window_handle); v_clsvwk(vdi_handle); appl_exit(); return 0;
}
