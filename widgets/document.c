/*
 * widgets/document.c - Poppler document widget
 *
 * Copyright © 2010 Fabian Streitel <karottenreibe@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "luah.h"
#include "clib/widget.h"
#include "widgets/common.h"

#include <gtk/gtk.h>
#include <poppler.h>

typedef struct {
    gint page;
    PopplerRectangle *rectangle;
} search_match_t;

typedef struct {
    const gchar *text;
    gboolean case_sensitive;
    gboolean forward;
    gboolean wrap;
    GList *matches;    /* of search_match_t */
    GList *last_match; /* of search_match_t */
    gint last_page;
} search_data_t;

typedef struct {
    GtkWidget *widget;
    /* document */
    PopplerDocument *document;
    const gchar *path;
    const gchar *password;
    /* pages */
    gint current;
    gpointer pages_ref;
    GPtrArray *pages;
    /* drawing data */
    gint spacing;
    gdouble zoom;
    GtkAdjustment *hadjust;
    GtkAdjustment *vadjust;
    gdouble width;
    gdouble height;
    /* searching */
    search_data_t *last_search;
} document_data_t;

typedef struct {
    PopplerPage *page;
    cairo_rectangle_t *rectangle;
    cairo_surface_t *surface;
} page_info_t;

static widget_t*
luaH_checkdocument(lua_State *L, gint udx)
{
    widget_t *w = luaH_checkwidget(L, udx);
    if (w->info->tok != L_TK_DOCUMENT)
        luaL_argerror(L, udx, "incorrect widget type (expected document)");
    return w;
}

static document_data_t*
luaH_checkdocument_data(lua_State *L, gint udx)
{
    widget_t *w = luaH_checkdocument(L, udx);
    document_data_t *d = w->data;
    return d;
}

static gint
luaH_document_push_indexed_table(lua_State *L, lua_CFunction index, lua_CFunction newindex, gint idx)
{
    /* create table */
    lua_newtable(L);
    /* setup metatable */
    lua_createtable(L, 0, 2);
    /* push __index metafunction */
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, idx); /* copy userdata */
    lua_pushcclosure(L, index, 1);
    lua_rawset(L, -3);
    /* push __newindex metafunction */
    lua_pushliteral(L, "__newindex");
    lua_pushvalue(L, idx); /* copy userdata */
    lua_pushcclosure(L, newindex, 1);
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    return 1;
}

#include "widgets/document/render.c"
#include "widgets/document/scroll.c"
#include "widgets/document/pages.c"
#include "widgets/document/printing.c"

static void
luaH_document_destructor(widget_t *w) {
    document_data_t *d = w->data;
    gtk_widget_destroy(GTK_WIDGET(d->widget));
    /* release our reference on the document. Poppler handles freeing it */
    if (d->document)
        g_object_unref(G_OBJECT(d->document));
    /* release our references on the pages. Poppler handles freeing it */
    if (d->pages) {
        for (guint i = 0; i < d->pages->len; ++i) {
            page_info_t *p = g_ptr_array_index(d->pages, i);
            g_object_unref(G_OBJECT(p->page));
            cairo_surface_destroy(p->surface);
            g_free(p->rectangle);
            g_free(p);
        }
        g_ptr_array_free(d->pages, TRUE);
    }
    g_object_unref(d->hadjust);
    g_object_unref(d->vadjust);
    g_free(d);
}

static void
document_update_adjustments(document_data_t *d)
{
    d->hadjust->page_size = d->widget->allocation.width / d->zoom;
    d->vadjust->page_size = d->widget->allocation.height / d->zoom;
}

static int
luaH_document_load(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    if (!d->path)
        luaL_error(L, "no path given to document class");
    GError *error = NULL;
    const gchar *uri = g_filename_to_uri(d->path, NULL, &error);
    if (error)
        luaL_error(L, error->message);
    error = NULL;
    d->document = poppler_document_new_from_file(uri, d->password, &error);
    if (error)
        luaL_error(L, error->message);

    /* extract pages */
    const gint size = poppler_document_get_n_pages(d->document);
    d->pages = g_ptr_array_sized_new(size);
    for (int i = 0; i < size; ++i) {
        page_info_t *p = g_new0(page_info_t, 1);
        p->page = poppler_document_get_page(d->document, i);
        p->rectangle = g_new0(cairo_rectangle_t, 1);
        poppler_page_get_size(p->page, &p->rectangle->width, &p->rectangle->height);
        g_ptr_array_add(d->pages, p);
    }

    /* calculate page geometry and positioning */
    gint ret = luaH_object_emit_signal(L, 1, "layout", 0, 2);
    if (!ret)
        luaL_error(L, "no layout was definied");

    gdouble height = luaL_checknumber(L, -1);
    gdouble width = luaL_checknumber(L, -2);

    d->width = width;
    d->height = height;

    /* configure adjustments */
    d->hadjust->upper = width;
    d->vadjust->upper = height;
    document_update_adjustments(d);
    return 0;
}

static void
document_search_match_destroy(gpointer p)
{
    search_match_t *sm = (search_match_t*) p;
    g_free(sm->rectangle);
    g_free(sm);
}

static void
document_clear_search(document_data_t *d)
{
    search_data_t *s = d->last_search;
    if (s) {
        g_list_free_full(s->matches, document_search_match_destroy);
        g_slice_free(search_data_t, s);
    }
    d->last_search = NULL;
}

static gint
luaH_document_search(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    const gchar *text = luaL_checkstring(L, 2);
    gboolean case_sensitive = luaH_checkboolean(L, 3);
    gboolean forward = luaH_checkboolean(L, 4);
    gboolean wrap = luaH_checkboolean(L, 5);

    // TODO: case sensitivity is ignored by poppler
    case_sensitive = FALSE;

    search_data_t *s = d->last_search;

    /* init search data */
    gint start_page = 0;
    if (s) start_page = s->last_page;
    /* clear old search if search term or case sensitivity changed */
    if (s && (g_strcmp0(s->text, text) != 0 || s->case_sensitive != case_sensitive)) {
        document_clear_search(d);
        s = NULL;
    }
    if (!s) {
        s = g_slice_new(search_data_t);
        s->text = text;
        s->case_sensitive = case_sensitive;
        s->forward = forward;
        s->wrap = wrap;
        s->last_page = start_page;
        s->last_match = NULL;

        /* get all matches */
        GList *matches = NULL;
        for (guint i = 0; i < d->pages->len; ++i) {
            page_info_t *p = g_ptr_array_index(d->pages, i);
            GList *poppler_matches = poppler_page_find_text(p->page, text);
            GList *m = poppler_matches;
            while (m) {
                search_match_t *sm = g_new(search_match_t, 1);
                sm->page = i;
                sm->rectangle = (PopplerRectangle*) m->data;
                matches = g_list_prepend(matches, sm);
                m = g_list_next(m);
            }
            g_list_free(poppler_matches);
        }
        s->matches = g_list_reverse(matches);

        d->last_search = s;
    }

    /* get the next finding */
    if (s->matches) {
        GList *match = s->last_match;
        GList *start_point = forward ? s->matches : g_list_last(s->matches);
        if (!match) {
            /* start from the starting page */
            GList *m = start_point;
            while (m) {
                search_match_t *sm = (search_match_t*) m->data;
                if ((forward && sm->page >= start_page) || (!forward && sm->page <= start_page)) {
                    match = m;
                    break;
                }
                m = forward ? g_list_next(m) : g_list_previous(m);
            }
        }
        if (match) match = forward ? g_list_next(match) : g_list_previous(match);
        if (!match && wrap) match = start_point;
    }
    /* highlight it */
    document_render(d);

    gboolean success = (s->last_match != NULL);
    lua_pushboolean(L, success);
    return 1;
}

static gint
luaH_document_clear_search(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    document_clear_search(d);
    return 0;
}

static gint
luaH_document_index(lua_State *L, luapdf_token_t token)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);

    switch(token)
    {
      LUAPDF_WIDGET_INDEX_COMMON

      /* functions */
      PF_CASE(LOAD,         luaH_document_load)
      PF_CASE(PRINT,        luaH_document_print)
      PF_CASE(SEARCH,       luaH_document_search)
      PF_CASE(CLEAR_SEARCH, luaH_document_clear_search)

      /* strings */
      PS_CASE(PATH,     d->path)
      PS_CASE(PASSWORD, d->password)
      PS_CASE(TITLE,    poppler_document_get_title(d->document))
      PS_CASE(AUTHOR,   poppler_document_get_author(d->document))
      PS_CASE(SUBJECT,  poppler_document_get_subject(d->document))
      PS_CASE(KEYWORDS, poppler_document_get_keywords(d->document))
      PS_CASE(CREATOR,  poppler_document_get_creator(d->document))
      PS_CASE(PRODUCER, poppler_document_get_producer(d->document))

      /* integers */
      PI_CASE(CURRENT,  d->current + 1)

      /* numbers */
      PN_CASE(ZOOM,     d->zoom)

      case L_TK_SCROLL:
        return luaH_document_push_indexed_table(L, luaH_document_scroll_index, luaH_document_scroll_newindex, 1);

      case L_TK_PAGES:
        return luaH_document_push_pages(L, d);

      default:
        break;
    }

    return 0;
}

static gint
luaH_document_newindex(lua_State *L, luapdf_token_t token)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);

    switch(token)
    {
      LUAPDF_WIDGET_INDEX_COMMON

      case L_TK_PATH:
        if (lua_isnil(L, 3))
            d->path = NULL;
        else
            d->path = luaL_checkstring(L, 3);
        break;

      case L_TK_PASSWORD:
        if (lua_isnil(L, 3))
            d->password = NULL;
        else
            d->password = luaL_checkstring(L, 3);
        break;

      case L_TK_ZOOM:
        d->zoom = luaL_checknumber(L, 3);
        document_update_adjustments(d);
        document_render(d);
        break;

      default:
        warn("unknown property: %s", luaL_checkstring(L, 2));
        return 0;
    }

    return luaH_object_emit_property_signal(L, 1);
}

void
render_cb(gpointer *UNUSED(p), document_data_t *d)
{
    document_render(d);
}

void
resize_cb(gpointer *UNUSED(p), GdkRectangle *UNUSED(r), document_data_t *d)
{
    document_update_adjustments(d);
}

void
expose_cb(GtkWidget *UNUSED(w), GdkEventExpose *UNUSED(e), document_data_t *d)
{
    document_render(d);
}

static gboolean
scroll_event_cb(GtkWidget *UNUSED(v), GdkEventScroll *ev, widget_t *w)
{
    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    luaH_modifier_table_push(L, ev->state);
    lua_pushinteger(L, ((int)ev->direction) + 4);
    gint ret = luaH_object_emit_signal(L, -3, "button-release", 2, 1);
    gboolean catch = ret && lua_toboolean(L, -1) ? TRUE : FALSE;
    lua_pop(L, ret + 1);
    return catch;
}

widget_t *
widget_document(widget_t *w, luapdf_token_t UNUSED(token))
{
    document_data_t *d = g_new0(document_data_t, 1);
    d->spacing = 10;
    d->zoom = 1.0;
    d->widget = gtk_event_box_new();
    d->hadjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 10, 1, 0));
    g_object_ref_sink(d->hadjust);
    d->vadjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 10, 1, 0));
    g_object_ref_sink(d->vadjust);
    w->data = d;

    w->widget = d->widget;
    /* set gobject property to give other widgets a pointer to our image widget */
    g_object_set_data(G_OBJECT(w->widget), "lua_widget", w);

    w->index = luaH_document_index;
    w->newindex = luaH_document_newindex;
    w->destructor = luaH_document_destructor;

    g_object_connect(G_OBJECT(d->hadjust),
      "signal::value-changed",        G_CALLBACK(render_cb),         d,
      NULL);
    g_object_connect(G_OBJECT(d->vadjust),
      "signal::value-changed",        G_CALLBACK(render_cb),         d,
      NULL);
    g_object_connect(G_OBJECT(d->widget),
      "signal::expose-event",         G_CALLBACK(expose_cb),         d,
      "signal::size-allocate",        G_CALLBACK(resize_cb),         d,
      "signal::scroll-event",         G_CALLBACK(scroll_event_cb),   w,
      NULL);

    /* show widgets */
    gtk_widget_show(d->widget);

    return w;
}

#undef luaH_checkdocument

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
