#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef enum {
    STATUS_WATCHING = 0,
    STATUS_COMPLETED,
    STATUS_DROPPED,
    STATUS_PLAN_TO_WATCH,
    STATUS_ON_HOLD,
    STATUS_COUNT
} AnimeStatus;

typedef enum {
    GENRE_ACTION = 0,
    GENRE_ROMANCE,
    GENRE_HORROR,
    GENRE_COMEDY,
    GENRE_FANTASY,
    GENRE_SCIFI,
    GENRE_MYSTERY,
    GENRE_SPORTS,
    GENRE_MECHA,
    GENRE_SLICE_OF_LIFE,
    GENRE_COUNT
} AnimeGenre;

#define MAX_EPISODES 1000
#define MAX_ANIME 512

typedef struct {
    char        title[256];
    char        studio[128];
    int         episodes_total;
    int         episodes_watched;
    AnimeStatus status;
    AnimeGenre  genre;
    float       episode_ratings[MAX_EPISODES]; /* Array to hold individual ratings per episode */
    int         year;
    char        notes[512];
} Anime;

static Anime g_anime[MAX_ANIME];
static int   g_count = 0;

/* --- Fixed UI Global Declarations --- */
static GtkWidget *g_window = NULL;
static GtkWidget *g_list_box = NULL;
static GtkWidget *g_stats_label = NULL;
static GtkWidget *g_filter_btns[STATUS_COUNT + 1];
static int        g_filter_btn_count = 0;
static int        g_filter = -1;
static char       g_search[256] = "";

static const char *STATUS_LABELS[STATUS_COUNT] = {
    "⚔ Watching", "✓ Completed", "✗ Dropped", "◎ Plan to Watch", "⏸ On Hold"
};
static const char *STATUS_COLORS[STATUS_COUNT] = {
    "#00f0ff", "#2dfc13", "#ff4545", "#ffdf22", "#ffaa00"
};
static const char *GENRE_LABELS[GENRE_COUNT] = {
    "Action","Romance","Horror","Comedy","Fantasy",
    "Sci-Fi","Mystery","Sports","MeCHA","Slice of Life"
};

/* ----------------------------------------------------------
   Lighter, Cyberpunk CSS UI Theme
-----------------------------------------------------------*/

static const char *APP_CSS =
"* { font-family: 'Noto Sans', 'DejaVu Sans', sans-serif; }"
"window { background-color: #1a1a24; }"

"#sidebar {"
"  background-color: #222232;"
"  border-right: 1px solid #33334c;"
"  min-width: 220px;"
"}"
".sidebar-logo {"
"  color: #ff557e;"
"  font-size: 14px;"
"  font-weight: 900;"
"  letter-spacing: 3px;"
"  margin: 20px 16px 4px 16px;"
"}"
".sidebar-section {"
"  color: #ff557e;"
"  font-size: 10px;"
"  font-weight: 700;"
"  letter-spacing: 3px;"
"  margin: 12px 16px 4px 16px;"
"}"
".sidebar-ver {"
"  color: #55557f;"
"  font-size: 10px;"
"  letter-spacing: 2px;"
"  margin: 0 16px 16px 16px;"
"}"

".filter-btn {"
"  background: transparent;"
"  border: none;"
"  border-radius: 0;"
"  box-shadow: none;"
"  color: #9999cc;"
"  font-size: 13px;"
"  padding: 8px 16px;"
"}"
".filter-btn:hover {"
"  background-color: #2b2b3d;"
"  color: #ffffff;"
"}"
".filter-active {"
"  background-color: #33334c;"
"  color: #ffffff;"
"  font-weight: 700;"
"  border-left: 3px solid #ff557e;"
"}"

"#header {"
"  background-color: #222232;"
"  border-bottom: 2px solid #ff557e;"
"  padding: 12px 20px;"
"}"
"#logo-label {"
"  color: #ff557e;"
"  font-size: 20px;"
"  font-weight: 900;"
"  letter-spacing: 3px;"
"}"
"#tagline-label {"
"  color: #666699;"
"  font-size: 11px;"
"  letter-spacing: 2px;"
"}"

"searchentry {"
"  background-color: #2b2b3d;"
"  border: 1px solid #444466;"
"  border-radius: 6px;"
"  color: #ffffff;"
"  font-size: 13px;"
"  min-width: 240px;"
"}"
"searchentry:focus {"
"  border-color: #ff557e;"
"}"

"#add-btn {"
"  background: linear-gradient(135deg,#ff557e,#db0040);"
"  border: none;"
"  border-radius: 6px;"
"  color: #fff;"
"  font-size: 13px;"
"  font-weight: 700;"
"  letter-spacing: 1px;"
"  padding: 8px 18px;"
"}"
"#add-btn:hover {"
"  background: linear-gradient(135deg,#ff7595,#f00048);"
"}"

"#stats-bar {"
"  background-color: #1e1e2b;"
"  border-bottom: 1px solid #2b2b3d;"
"  padding: 8px 16px;"
"}"
"#stats-label {"
"  color: #8888b3;"
"  font-size: 12px;"
"  letter-spacing: 1px;"
"}"

"#main-area { background-color: #1a1a24; }"
"#list-scroll { background-color: #1a1a24; }"

".anime-card {"
"  background-color: #242435;"
"  border: 1px solid #33334c;"
"  border-radius: 8px;"
"  margin: 6px 12px;"
"  padding: 14px 18px;"
"}"
".anime-card:hover {"
"  background-color: #2b2b3f;"
"  border-color: #ff557e;"
"}"
".anime-title {"
"  color: #ffffff;"
"  font-size: 16px;"
"  font-weight: 700;"
"}"
".anime-meta {"
"  color: #9999cc;"
"  font-size: 12px;"
"}"
".rating-lbl {"
"  color: #ffdf22;"
"  font-size: 13px;"
"  font-weight: 700;"
"}"
".card-btn {"
"  background-color: #33334c;"
"  border: 1px solid #444466;"
"  border-radius: 5px;"
"  color: #ccccff;"
"  font-size: 11px;"
"  padding: 4px 12px;"
"  min-height: 0;"
"}"
".card-btn:hover {"
"  background-color: #444466;"
"  border-color: #ff557e;"
"  color: #ff557e;"
"}"
".next-btn {"
"  background: linear-gradient(135deg, #00f0ff, #0077ff); "
"  border: none;"
"  border-radius: 5px;"
"  color: #fff;"
"  font-size: 11px;"
"  font-weight: 700;"
"  padding: 4px 12px;"
"  min-height: 0;"
"}"
".next-btn:hover {"
"  background: linear-gradient(135deg, #70f5ff, #0055cc);"
"}"
".del-btn {"
"  background-color: #441a2a;"
"  border: 1px solid #772244;"
"  border-radius: 5px;"
"  color: #ff6699;"
"  font-size: 11px;"
"  padding: 4px 10px;"
"  min-height: 0;"
"}"
".del-btn:hover {"
"  background-color: #551a30;"
"  border-color: #ff4545;"
"  color: #ff4545;"
"}"
".progress-bar trough {"
"  background-color: #33334c;"
"  border-radius: 3px;"
"  min-height: 8px;"
"}"
".progress-bar progress {"
"  background: linear-gradient(90deg,#ff557e,#ffaa00);"
"  border-radius: 3px;"
"}"

".empty-lbl {"
"  color: #444466;"
"  font-size: 16px;"
"  font-weight: 700;"
"  letter-spacing: 2px;"
"  margin-top: 80px;"
"}"

"dialog { background-color: #222232; border: 1px solid #444466; border-radius: 8px; }"
"dialog .dialog-title {"
"  color: #ff557e;"
"  font-size: 17px;"
"  font-weight: 900;"
"  letter-spacing: 2px;"
"  margin-bottom: 8px;"
"}"
"dialog .form-lbl {"
"  color: #ccccff;"
"  font-size: 11px;"
"  font-weight: 700;"
"  letter-spacing: 1px;"
"}"
"dialog entry, dialog spinbutton {"
"  background-color: #2b2b3d;"
"  border: 1px solid #444466;"
"  border-radius: 5px;"
"  color: #ffffff;"
"  font-size: 13px;"
"  padding: 5px 10px;"
"}"
"dialog entry:focus, dialog spinbutton:focus {"
"  border-color: #ff557e;"
"}"
"dialog dropDown button {"
"  background-color: #2b2b3d;"
"  border: 1px solid #444466;"
"  color: #ffffff;"
"  font-size: 13px;"
"  border-radius: 5px;"
"}"
;

/* ----------------------------------------------------------
   Helpers to compute structural episodic averages
   ------------------------------------------------------- */

static float get_anime_average_rating(const Anime *a) {
    if (a->episodes_watched <= 0) return 0.0f;
    float sum = 0.0f;
    int count = 0;
    for (int i = 0; i < a->episodes_watched; i++) {
        if (a->episode_ratings[i] >= 0.0f) {
            sum += a->episode_ratings[i];
            count++;
        }
    }
    return count > 0 ? (sum / (float)count) : 0.0f;
}

/* ----------------------------------------------------------
   Persistence Helpers
   ------------------------------------------------------- */

static void ensure_dir_exists(const char *path) {
    char temp[512];
    snprintf(temp, sizeof(temp), "%s", path);
    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0755);
            *p = '/';
        }
    }
}

static void get_csv_path(char *buf, size_t len) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buf, len, "%s/.local/share/shinigami_scroll/data.csv", home);
}

static void save_to_csv(void) {
    char path[512];
    get_csv_path(path, sizeof(path));
    ensure_dir_exists(path);

    FILE *f = fopen(path, "w");
    if (!f) {
        g_printerr("Error: Could not save data to %s\n", path);
        return;
    }
    fprintf(f, "title,studio,episodes_total,episodes_watched,status,genre,year,notes,ratings\n");
    for (int i = 0; i < g_count; i++) {
        Anime *a = &g_anime[i];
        fprintf(f, "\"%s\",\"%s\",%d,%d,%d,%d,%d,\"%s\",\"",
                a->title, a->studio, a->episodes_total, a->episodes_watched,
                a->status, a->genre, a->year, a->notes);
        
        // Serialize array fields sequentially using semi-colons
        for(int j = 0; j < a->episodes_total && j < MAX_EPISODES; j++) {
            fprintf(f, "%.2f%s", a->episode_ratings[j], (j == a->episodes_total - 1) ? "" : ";");
        }
        fprintf(f, "\"\n");
    }
    fclose(f);
}

static void load_from_csv(void) {
    char path[512];
    get_csv_path(path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[4096];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return;
    }

    g_count = 0;
    while (fgets(line, sizeof(line), f) && g_count < MAX_ANIME) {
        Anime *a = &g_anime[g_count];
        // Initialize all episode elements to unrated fallback values first
        for(int i=0; i<MAX_EPISODES; i++) a->episode_ratings[i] = 0.0f;
        
        int st, ge;
        char *p = line;
        
        if (*p == '"') { 
            p++; char *t = strchr(p, '"'); 
            if (t) { *t = '\0'; g_strlcpy(a->title, p, sizeof(a->title)); p = t + 2; } 
        } else { 
            char *c = strchr(p, ','); 
            if (c) { *c = '\0'; g_strlcpy(a->title, p, sizeof(a->title)); p = c + 1; } 
        }
        
        if (*p == '"') { 
            p++; char *t = strchr(p, '"'); 
            if (t) { *t = '\0'; g_strlcpy(a->studio, p, sizeof(a->studio)); p = t + 2; } 
        } else { 
            char *c = strchr(p, ','); 
            if (c) { *c = '\0'; g_strlcpy(a->studio, p, sizeof(a->studio)); p = c + 1; } 
        }
        
        if (sscanf(p, "%d,%d,%d,%d,%d", &a->episodes_total, &a->episodes_watched, &st, &ge, &a->year) == 5) {
            a->status = (AnimeStatus)st;
            a->genre = (AnimeGenre)ge;
            
            char *comma_scan = p;
            for (int k = 0; k < 5 && comma_scan; k++) {
                comma_scan = strchr(comma_scan, ',');
                if (comma_scan) comma_scan++;
            }
            
            if (comma_scan) {
                if (*comma_scan == '"') {
                    comma_scan++;
                    char *t = strchr(comma_scan, '"');
                    if (t) { *t = '\0'; g_strlcpy(a->notes, comma_scan, sizeof(a->notes)); comma_scan = t + 2; }
                } else {
                    char *c = strchr(comma_scan, ',');
                    if (c) { *c = '\0'; g_strlcpy(a->notes, comma_scan, sizeof(a->notes)); comma_scan = c + 1; }
                }
            }

            // Read the serial episode chunk sequence string representation
            if (comma_scan && *comma_scan == '"') {
                comma_scan++;
                char *end_quote = strchr(comma_scan, '"');
                if (end_quote) *end_quote = '\0';
                
                char *token = strtok(comma_scan, ";");
                int ep_idx = 0;
                while (token && ep_idx < MAX_EPISODES) {
                    a->episode_ratings[ep_idx++] = (float)atof(token);
                    token = strtok(NULL, ";");
                }
            }
            g_count++;
        }
    }
    fclose(f);
}

/* ----------------------------------------------------------
   Filter Configuration Helpers
   ------------------------------------------------------- */

static void set_active_filter(int which) {
    for (int i = 0; i < g_filter_btn_count; i++) {
        if (i == which) {
            gtk_widget_add_css_class(g_filter_btns[i], "filter-active");
        } else {
            gtk_widget_remove_css_class(g_filter_btns[i], "filter-active");
        }
    }
}

/* ----------------------------------------------------------
   Global Statistics Update
   ------------------------------------------------------- */

static void update_stats(void) {
    int comp = 0, watching = 0, total_ep = 0, rated_animes = 0;
    float global_sum = 0.0f;
    
    for (int i = 0; i < g_count; i++) {
        if (g_anime[i].status == STATUS_COMPLETED) comp++;
        if (g_anime[i].status == STATUS_WATCHING)  watching++;
        total_ep += g_anime[i].episodes_watched;
        
        float avg = get_anime_average_rating(&g_anime[i]);
        if (avg > 0.0f) {
            global_sum += avg;
            rated_animes++;
        }
    }
    char buf[512];
    snprintf(buf, sizeof(buf),
        "📚 %d anime   ✓ %d completed   ⚔ %d watching   🎬 %d episodes   ⭐ avg %.2f",
        g_count, comp, watching, total_ep, rated_animes ? (global_sum / (float)rated_animes) : 0.0f);
    gtk_label_set_text(GTK_LABEL(g_stats_label), buf);
}

static void rebuild_list(void);

/* -------------------------------------------------------------
   Dialog form configuration
   ---------------------------------------------------------- */

typedef struct {
    GtkWidget *title_e, *studio_e, *notes_e;
    GtkWidget *ep_tot, *year_s;
    GtkWidget *status_dd, *genre_dd;
} Form;

static Form g_form;

static GtkWidget *flbl(const char *t) {
    GtkWidget *l = gtk_label_new(t);
    gtk_widget_add_css_class(l,"form-lbl");
    gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
    return l;
}
static GtkWidget *fentry(const char *ph) {
    GtkWidget *e = gtk_entry_new();
    if (ph) gtk_entry_set_placeholder_text(GTK_ENTRY(e), ph);
    return e;
}
static GtkWidget *fspin(double lo, double hi, double step, double val, int digits) {
    GtkWidget *s = gtk_spin_button_new_with_range(lo,hi,step);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s),val);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s),digits);
    return s;
}

static GtkStringList *make_string_list(const char **items, int n) {
    GtkStringList *sl = gtk_string_list_new(NULL);
    for (int i=0; i<n; i++) gtk_string_list_append(sl, items[i]);
    return sl;
}

typedef struct {
    GtkWidget *win;   
    int        idx;   
} DlgCtx;

static void form_to_anime(Anime *a, int is_edit) {
    snprintf(a->title,  sizeof(a->title),  "%s", gtk_editable_get_text(GTK_EDITABLE(g_form.title_e)));
    snprintf(a->studio, sizeof(a->studio), "%s", gtk_editable_get_text(GTK_EDITABLE(g_form.studio_e)));
    snprintf(a->notes,  sizeof(a->notes),  "%s", gtk_editable_get_text(GTK_EDITABLE(g_form.notes_e)));
    a->episodes_total   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_form.ep_tot));
    a->year    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_form.year_s));
    a->status  = (AnimeStatus)gtk_drop_down_get_selected(GTK_DROP_DOWN(g_form.status_dd));
    a->genre   = (AnimeGenre) gtk_drop_down_get_selected(GTK_DROP_DOWN(g_form.genre_dd));
    
    if (strlen(a->studio) == 0) {
        snprintf(a->studio, sizeof(a->studio), "Unknown");
    }

    if (!is_edit) {
        a->episodes_watched = 0;
        memset(a->episode_ratings, 0, sizeof(a->episode_ratings));
    }
    
    if (a->episodes_total > 0 && a->episodes_watched > a->episodes_total)
        a->episodes_watched = a->episodes_total;
}

static void on_dlg_save(GtkButton *btn, gpointer user_data) {
    (void)btn;
    DlgCtx *ctx = (DlgCtx *)user_data;
    if (ctx->idx < 0) {
        if (g_count < MAX_ANIME) {
            Anime a = {0};
            form_to_anime(&a, 0);
            if (strlen(a.title) > 0) {
                g_anime[g_count++] = a;
                save_to_csv();
                rebuild_list();
                update_stats();
            }
        }
    } else {
        if (ctx->idx < g_count) {
            form_to_anime(&g_anime[ctx->idx], 1);
            save_to_csv();
            rebuild_list();
            update_stats();
        }
    }
    gtk_window_destroy(GTK_WINDOW(ctx->win));
    g_free(ctx);
}

static void on_dlg_cancel(GtkButton *btn, gpointer user_data) {
    (void)btn;
    DlgCtx *ctx = (DlgCtx *)user_data;
    gtk_window_destroy(GTK_WINDOW(ctx->win));
    g_free(ctx);
}

static void build_and_show_dialog(const char *heading, Anime *existing, int edit_idx) {
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), heading);
    gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(g_window));
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 520, -1);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    DlgCtx *ctx = g_new0(DlgCtx, 1);
    ctx->win = win;
    ctx->idx = edit_idx;

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(win), outer);

    GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(inner, 20);
    gtk_widget_set_margin_end(inner, 20);
    gtk_widget_set_margin_top(inner, 16);
    gtk_widget_set_margin_bottom(inner, 8);
    gtk_box_append(GTK_BOX(outer), inner);

    GtkWidget *title_lbl = gtk_label_new(heading);
    gtk_widget_add_css_class(title_lbl, "dialog-title");
    gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.0f);
    gtk_box_append(GTK_BOX(inner), title_lbl);

    gtk_box_append(GTK_BOX(inner), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_box_append(GTK_BOX(inner), grid);

    int row = 0;
#define GL(txt)  gtk_grid_attach(GTK_GRID(grid), flbl(txt), 0, row,   1, 1)
#define GW(w)    gtk_grid_attach(GTK_GRID(grid), (w),       1, row++, 1, 1); \
                 gtk_widget_set_hexpand((w), TRUE)

    GL("TITLE");
    g_form.title_e = fentry("e.g. Attack on Titan");
    if (existing) gtk_editable_set_text(GTK_EDITABLE(g_form.title_e), existing->title);
    GW(g_form.title_e);

    GL("STUDIO");
    g_form.studio_e = fentry("e.g. WIT Studio / MAPPA");
    if (existing) gtk_editable_set_text(GTK_EDITABLE(g_form.studio_e), existing->studio);
    GW(g_form.studio_e);

    GL("YEAR");
    g_form.year_s = fspin(1960, 2035, 1, existing ? existing->year : 2026, 0);
    GW(g_form.year_s);

    GL("TOTAL EPISODES");
    g_form.ep_tot = fspin(0, 9999, 1, existing ? existing->episodes_total : 12, 0);
    GW(g_form.ep_tot);

    GL("STATUS");
    g_form.status_dd = gtk_drop_down_new(
        G_LIST_MODEL(make_string_list(STATUS_LABELS, STATUS_COUNT)), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(g_form.status_dd),
        existing ? (guint)existing->status : (guint)STATUS_PLAN_TO_WATCH);
    GW(g_form.status_dd);

    GL("GENRE");
    g_form.genre_dd = gtk_drop_down_new(
        G_LIST_MODEL(make_string_list(GENRE_LABELS, GENRE_COUNT)), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(g_form.genre_dd),
        existing ? (guint)existing->genre : (guint)GENRE_ACTION);
    GW(g_form.genre_dd);

    GL("NOTES");
    g_form.notes_e = fentry("Any thoughts...");
    if (existing) gtk_editable_set_text(GTK_EDITABLE(g_form.notes_e), existing->notes);
    GW(g_form.notes_e);

#undef GL
#undef GW

    GtkWidget *btn_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(btn_sep, 8);
    gtk_box_append(GTK_BOX(outer), btn_sep);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(btn_row, 20);
    gtk_widget_set_margin_end(btn_row, 20);
    gtk_widget_set_margin_top(btn_row, 10);
    gtk_widget_set_margin_bottom(btn_row, 16);
    gtk_box_append(GTK_BOX(outer), btn_row);

    GtkWidget *sp = gtk_label_new("");
    gtk_widget_set_hexpand(sp, TRUE);
    gtk_box_append(GTK_BOX(btn_row), sp);

    GtkWidget *cancel_btn = gtk_button_new_with_mnemonic("_Cancel");
    gtk_widget_add_css_class(cancel_btn, "card-btn");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_dlg_cancel), ctx);
    gtk_box_append(GTK_BOX(btn_row), cancel_btn);

    GtkWidget *save_btn = gtk_button_new_with_mnemonic("_Save");
    gtk_widget_set_name(save_btn, "add-btn");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_dlg_save), ctx);
    gtk_box_append(GTK_BOX(btn_row), save_btn);

    gtk_window_present(GTK_WINDOW(win));
}

/* -------------------------------------------------------------
   Episodic Rating Prompt Dialog Window Hook
   ---------------------------------------------------------- */

typedef struct {
    GtkWidget *win;
    GtkWidget *spin;
    int        idx;
    int        target_episode;
} RatingCtx;

static void on_rating_submit(GtkButton *btn, gpointer user_data) {
    (void)btn;
    RatingCtx *ctx = (RatingCtx *)user_data;
    if (ctx->idx >= 0 && ctx->idx < g_count) {
        int ep_pos = ctx->target_episode - 1;
        if (ep_pos >= 0 && ep_pos < MAX_EPISODES) {
            g_anime[ctx->idx].episode_ratings[ep_pos] = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->spin));
        }
        save_to_csv();
        rebuild_list();
        update_stats();
    }
    gtk_window_destroy(GTK_WINDOW(ctx->win));
    g_free(ctx);
}

static void prompt_for_episode_rating(int idx, int episode_num) {
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Rate Episode");
    gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(g_window));
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 340, -1);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    RatingCtx *ctx = g_new0(RatingCtx, 1);
    ctx->win = win;
    ctx->idx = idx;
    ctx->target_episode = episode_num;

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 16);
    gtk_widget_set_margin_end(vbox, 16);
    gtk_widget_set_margin_top(vbox, 16);
    gtk_widget_set_margin_bottom(vbox, 16);
    gtk_window_set_child(GTK_WINDOW(win), vbox);

    char label_text[512];
    snprintf(label_text, sizeof(label_text), 
             "You finished episode %d of:\n<b>%s</b>\n\nHow would you rate this episode?", 
             episode_num, g_anime[idx].title);
    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl), label_text);
    gtk_box_append(GTK_BOX(vbox), lbl);

    ctx->spin = fspin(0.0, 10.0, 0.5, 7.0, 1);
    gtk_box_append(GTK_BOX(vbox), ctx->spin);

    GtkWidget *btn = gtk_button_new_with_label("Submit Episode Rating");
    gtk_widget_add_css_class(btn, "next-btn");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_rating_submit), ctx);
    gtk_box_append(GTK_BOX(vbox), btn);

    gtk_window_present(GTK_WINDOW(win));
}

/* ----------------------------------------------------------
   Button Callbacks
   ------------------------------------------------------- */

static void on_edit_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    build_and_show_dialog("EDIT ENTRY", &g_anime[idx], idx);
}

static void on_next_episode_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    Anime *a = &g_anime[idx];
    if (a->episodes_total > 0 && a->episodes_watched < a->episodes_total) {
        a->episodes_watched++;
        int tracking_current_ep = a->episodes_watched;
        
        save_to_csv();
        rebuild_list();
        update_stats();
        
        // Triggers the contextual prompt for this specific episode rating
        prompt_for_episode_rating(idx, tracking_current_ep);
    }
}

/* Permanent deletion feature callback */
static void on_permanent_delete_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    // Shift array to remove the item entirely
    for (int i = idx; i < g_count - 1; i++) {
        g_anime[i] = g_anime[i + 1];
    }
    g_count--;

    save_to_csv();
    rebuild_list();
    update_stats();
}

/* Revive dropped anime feature callback */
static void on_revive_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    g_anime[idx].status = STATUS_ON_HOLD;

    save_to_csv();
    rebuild_list();
    update_stats();
}

/* Start watching plan/hold anime feature callback */
static void on_start_watching_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    g_anime[idx].status = STATUS_WATCHING;

    save_to_csv();
    rebuild_list();
    update_stats();
}

/* Move Watching to Dropped callback */
static void on_drop_watching_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    g_anime[idx].status = STATUS_DROPPED;

    save_to_csv();
    rebuild_list();
    update_stats();
}

/* Move Watching to Completed callback */
static void on_done_watching_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    g_anime[idx].status = STATUS_COMPLETED;

    save_to_csv();
    rebuild_list();
    update_stats();
}

/* Watch Again callback */
static void on_watch_again_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(user_data);
    if (idx < 0 || idx >= g_count) return;

    g_anime[idx].status = STATUS_WATCHING;
    g_anime[idx].episodes_watched = 0;
    memset(g_anime[idx].episode_ratings, 0, sizeof(g_anime[idx].episode_ratings));

    save_to_csv();
    rebuild_list();
    update_stats();
}

static void on_add_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    if (g_count >= MAX_ANIME) return;
    build_and_show_dialog("ADD ANIME", NULL, -1);
}

static void on_search_changed(GtkSearchEntry *e, gpointer ud) {
    (void)ud;
    snprintf(g_search, sizeof(g_search), "%s", gtk_editable_get_text(GTK_EDITABLE(e)));
    rebuild_list();
}

static void on_filter_all(GtkButton *b, gpointer d) {
    (void)b;(void)d;
    g_filter = -1;
    set_active_filter(0);
    rebuild_list();
}

static void on_filter_status(GtkButton *b, gpointer user_data) {
    (void)b;
    g_filter = GPOINTER_TO_INT(user_data);
    set_active_filter(g_filter + 1);
    rebuild_list();
}

/* -------------------------------------------------------------
   Card Widget Renderer
   ---------------------------------------------------------- */

static GtkWidget *build_card(int idx) {
    Anime *a = &g_anime[idx];

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(card, "anime-card");

    GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(card), top);

    GtkWidget *title_lbl = gtk_label_new(a->title);
    gtk_widget_add_css_class(title_lbl, "anime-title");
    gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(title_lbl), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(title_lbl, TRUE);
    gtk_box_append(GTK_BOX(top), title_lbl);

    /* Computation calculation to map average score of all episodes watched so far */
    float score_avg = get_anime_average_rating(a);
    if (score_avg > 0.0f) {
        char rb[32]; 
        snprintf(rb, sizeof(rb), "★ %.2f", score_avg);
        GtkWidget *rl = gtk_label_new(rb);
        gtk_widget_add_css_class(rl, "rating-lbl");
        gtk_box_append(GTK_BOX(top), rl);
    }

    char mk[256];
    snprintf(mk, sizeof(mk),
        "<span foreground='%s' weight='bold' size='small'>%s</span>",
        STATUS_COLORS[a->status], STATUS_LABELS[a->status]);
    GtkWidget *st_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(st_lbl), mk);
    gtk_box_append(GTK_BOX(top), st_lbl);

    char meta[256];
    snprintf(meta, sizeof(meta), "%s  ·  %d  ·  %s", a->studio, a->year, GENRE_LABELS[a->genre]);
    GtkWidget *meta_lbl = gtk_label_new(meta);
    gtk_widget_add_css_class(meta_lbl, "anime-meta");
    gtk_label_set_xalign(GTK_LABEL(meta_lbl), 0.0f);
    gtk_box_append(GTK_BOX(card), meta_lbl);

    if (a->episodes_total > 0) {
        double frac = (double)a->episodes_watched / a->episodes_total;
        if (frac > 1.0) frac = 1.0;
        GtkWidget *pb = gtk_progress_bar_new();
        gtk_widget_add_css_class(pb, "progress-bar");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), frac);
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(pb), TRUE);
        char pt[64]; snprintf(pt, sizeof(pt), "%d / %d ep", a->episodes_watched, a->episodes_total);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pb), pt);
        gtk_box_append(GTK_BOX(card), pb);
    }

    GtkWidget *bot = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(card), bot);

    GtkWidget *spacer = gtk_label_new(strlen(a->notes) > 0 ? a->notes : "");
    gtk_widget_add_css_class(spacer, "anime-meta");
    gtk_label_set_xalign(GTK_LABEL(spacer), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(spacer), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(bot), spacer);

    /* --- Status Context-Specific Buttons --- */
    if (a->status == STATUS_WATCHING) {
        // 1. +1 ep button
        GtkWidget *next_btn = gtk_button_new_with_label("+1 Ep");
        gtk_widget_add_css_class(next_btn, "next-btn");
        g_signal_connect(next_btn, "clicked", G_CALLBACK(on_next_episode_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), next_btn);

        // 2. Edit button
        GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
        gtk_widget_add_css_class(edit_btn, "card-btn");
        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), edit_btn);

        // 3. Drop button (The 'x' button to drop it)
        GtkWidget *drop_btn = gtk_button_new_with_label("✗ Drop");
        gtk_widget_add_css_class(drop_btn, "del-btn");
        g_signal_connect(drop_btn, "clicked", G_CALLBACK(on_drop_watching_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), drop_btn);

        // 4. Done button (Only sensitive if the progress bar/episodes are full)
        GtkWidget *done_btn = gtk_button_new_with_label("✓ Done");
        gtk_widget_add_css_class(done_btn, "next-btn");
        g_signal_connect(done_btn, "clicked", G_CALLBACK(on_done_watching_clicked), GINT_TO_POINTER(idx));
        
        if (a->episodes_total > 0 && a->episodes_watched >= a->episodes_total) {
            gtk_widget_set_sensitive(done_btn, TRUE);
        } else {
            gtk_widget_set_sensitive(done_btn, FALSE);
        }
        gtk_box_append(GTK_BOX(bot), done_btn);
    }
    else if (a->status == STATUS_DROPPED) {
        // 1. Revive button
        GtkWidget *rev_btn = gtk_button_new_with_label("Revive");
        gtk_widget_add_css_class(rev_btn, "next-btn");
        g_signal_connect(rev_btn, "clicked", G_CALLBACK(on_revive_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), rev_btn);

        // 2. Permanent Delete button
        GtkWidget *p_del_btn = gtk_button_new_with_label("Delete");
        gtk_widget_add_css_class(p_del_btn, "del-btn");
        g_signal_connect(p_del_btn, "clicked", G_CALLBACK(on_permanent_delete_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), p_del_btn);
    }
    else if (a->status == STATUS_PLAN_TO_WATCH) {
        // 1. Start Watching button
        GtkWidget *start_btn = gtk_button_new_with_label("⚔ Start Watching");
        gtk_widget_add_css_class(start_btn, "next-btn");
        g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_watching_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), start_btn);

        // 2. Delete button
        GtkWidget *p_del_btn = gtk_button_new_with_label("Delete");
        gtk_widget_add_css_class(p_del_btn, "del-btn");
        g_signal_connect(p_del_btn, "clicked", G_CALLBACK(on_permanent_delete_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), p_del_btn);
    }
    else if (a->status == STATUS_ON_HOLD) {
        // 1. Start Watching button
        GtkWidget *start_btn = gtk_button_new_with_label("⚔ Start Watching");
        gtk_widget_add_css_class(start_btn, "next-btn");
        g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_watching_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), start_btn);

        // 2. Delete button
        GtkWidget *p_del_btn = gtk_button_new_with_label("Delete");
        gtk_widget_add_css_class(p_del_btn, "del-btn");
        g_signal_connect(p_del_btn, "clicked", G_CALLBACK(on_permanent_delete_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), p_del_btn);
    }
    else if (a->status == STATUS_COMPLETED) {
        // 1. Watch Again button
        GtkWidget *again_btn = gtk_button_new_with_label("🔄 Watch Again");
        gtk_widget_add_css_class(again_btn, "next-btn");
        g_signal_connect(again_btn, "clicked", G_CALLBACK(on_watch_again_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), again_btn);

        // 2. Delete button
        GtkWidget *p_del_btn = gtk_button_new_with_label("Delete");
        gtk_widget_add_css_class(p_del_btn, "del-btn");
        g_signal_connect(p_del_btn, "clicked", G_CALLBACK(on_permanent_delete_clicked), GINT_TO_POINTER(idx));
        gtk_box_append(GTK_BOX(bot), p_del_btn);
    }

    return card;
}

/* -------------------------------------------------------------
   Rebuild list
   ---------------------------------------------------------- */

static void rebuild_list(void) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(g_list_box)) != NULL)
        gtk_box_remove(GTK_BOX(g_list_box), child);

    int shown = 0;
    for (int i = 0; i < g_count; i++) {
        if (g_filter >= 0 && g_anime[i].status != g_filter) continue;
        if (strlen(g_search) > 0) {
            char hay[512], lo_hay[512], lo_needle[256];
            snprintf(hay, sizeof(hay), "%s %s", g_anime[i].title, g_anime[i].studio);
            for (int k = 0; hay[k]; k++)      lo_hay[k] = (char)tolower((unsigned char)hay[k]);
            lo_hay[strlen(hay)] = '\0';
            for (int k = 0; g_search[k]; k++) lo_needle[k] = (char)tolower((unsigned char)g_search[k]);
            lo_needle[strlen(g_search)] = '\0';
            if (!strstr(lo_hay, lo_needle)) continue;
        }
        gtk_box_append(GTK_BOX(g_list_box), build_card(i));
        shown++;
    }

    if (shown == 0) {
        GtkWidget *empty = gtk_label_new("— NO ENTRIES FOUND —");
        gtk_widget_add_css_class(empty, "empty-lbl");
        gtk_box_append(GTK_BOX(g_list_box), empty);
    }
}

/* -------------------------------------------------------------
   Sidebar
   ---------------------------------------------------------- */

static GtkWidget *build_sidebar(void) {
    GtkWidget *sb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(sb, "sidebar");

    GtkWidget *logo = gtk_label_new("SHINIGAMI\nSCROLL");
    gtk_widget_add_css_class(logo, "sidebar-logo");
    gtk_label_set_justify(GTK_LABEL(logo), GTK_JUSTIFY_LEFT);
    gtk_label_set_xalign(GTK_LABEL(logo), 0.0f);
    gtk_box_append(GTK_BOX(sb), logo);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_start(sep, 16); gtk_widget_set_margin_end(sep, 16);
    gtk_widget_set_margin_top(sep, 8);   gtk_widget_set_margin_bottom(sep, 8);
    gtk_box_append(GTK_BOX(sb), sep);

    GtkWidget *flt = gtk_label_new("FILTER");
    gtk_widget_add_css_class(flt, "sidebar-section");
    gtk_label_set_xalign(GTK_LABEL(flt), 0.0f);
    gtk_box_append(GTK_BOX(sb), flt);

    g_filter_btn_count = 0;

    GtkWidget *btn_all = gtk_button_new_with_label("◈  All Anime");
    gtk_widget_add_css_class(btn_all, "filter-btn");
    gtk_widget_add_css_class(btn_all, "filter-active");
    g_signal_connect(btn_all, "clicked", G_CALLBACK(on_filter_all), NULL);
    gtk_box_append(GTK_BOX(sb), btn_all);
    g_filter_btns[g_filter_btn_count++] = btn_all;

    for (int i = 0; i < STATUS_COUNT; i++) {
        GtkWidget *btn = gtk_button_new_with_label(STATUS_LABELS[i]);
        gtk_widget_add_css_class(btn, "filter-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_filter_status), GINT_TO_POINTER(i));
        gtk_box_append(GTK_BOX(sb), btn);
        g_filter_btns[g_filter_btn_count++] = btn;
    }

    GtkWidget *sp = gtk_label_new("");
    gtk_widget_set_vexpand(sp, TRUE);
    gtk_box_append(GTK_BOX(sb), sp);

    GtkWidget *ver = gtk_label_new("Happy Weeb v2.5");
    gtk_widget_add_css_class(ver, "sidebar-ver");
    gtk_label_set_xalign(GTK_LABEL(ver), 0.0f);
    gtk_box_append(GTK_BOX(sb), ver);

    return sb;
}

/* -------------------------------------------------------------
   Activate
   --------------------------------------------------------- */

static void activate(GtkApplication *app, gpointer ud) {
    (void)ud;

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, APP_CSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    g_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(g_window), "Shinigami Scroll — Anime Watchlist");
    gtk_window_set_default_size(GTK_WINDOW(g_window), 1100, 720);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(g_window), root);

    gtk_box_append(GTK_BOX(root), build_sidebar());

    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(right, "main-area");
    gtk_widget_set_hexpand(right, TRUE);
    gtk_box_append(GTK_BOX(root), right);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_name(header, "header");
    gtk_widget_set_margin_start(header, 20);
    gtk_widget_set_margin_end(header, 20);
    gtk_widget_set_margin_top(header, 12);
    gtk_widget_set_margin_bottom(header, 12);
    gtk_box_append(GTK_BOX(right), header);

    GtkWidget *logo_lbl = gtk_label_new("SHINIGAMI SCROLL");
    gtk_widget_set_name(logo_lbl, "logo-label");
    gtk_box_append(GTK_BOX(header), logo_lbl);

    GtkWidget *tagline = gtk_label_new("YOUR ANIME COMPENDIUM");
    gtk_widget_set_name(tagline, "tagline-label");
    gtk_widget_set_hexpand(tagline, TRUE);
    gtk_label_set_xalign(GTK_LABEL(tagline), 0.0f);
    gtk_widget_set_margin_start(tagline, 10);
    gtk_box_append(GTK_BOX(header), tagline);

    GtkWidget *search = gtk_search_entry_new();
    gtk_widget_set_name(search, "search-entry");
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search), "Search title...");
    g_signal_connect(search, "search-changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_append(GTK_BOX(header), search);

    GtkWidget *add_btn = gtk_button_new_with_label("＋ ADD ANIME");
    gtk_widget_set_name(add_btn, "add-btn");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), NULL);
    gtk_box_append(GTK_BOX(header), add_btn);

    GtkWidget *stats_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_name(stats_bar, "stats-bar");
    gtk_widget_set_margin_start(stats_bar, 8);
    gtk_widget_set_margin_end(stats_bar, 8);
    gtk_widget_set_margin_top(stats_bar, 4);
    gtk_widget_set_margin_bottom(stats_bar, 4);

    g_stats_label = gtk_label_new("");
    gtk_widget_set_name(g_stats_label, "stats-label");
    gtk_label_set_xalign(GTK_LABEL(g_stats_label), 0.0f);
    gtk_box_append(GTK_BOX(stats_bar), g_stats_label);
    gtk_box_append(GTK_BOX(right), stats_bar);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_name(scroll, "list-scroll");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(right), scroll);

    g_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(g_list_box, 8);
    gtk_widget_set_margin_bottom(g_list_box, 8);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), g_list_box);

    load_from_csv();
    rebuild_list();
    update_stats();

    gtk_window_present(GTK_WINDOW(g_window));
}

/* -------------------------------------------------------------
   Main Entry Point
   ---------------------------------------------------------- */

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new(
        "io.shinigami.scroll", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int rc = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return rc;
}
