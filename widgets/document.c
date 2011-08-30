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
} document_data_t;

typedef struct {
    PopplerPage *page;
    gdouble x;
    gdouble y;
    gdouble w;
    gdouble h;
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
            g_slice_free(page_info_t, p);
        }
        g_ptr_array_free(d->pages, TRUE);
    }
    g_object_unref(d->hadjust);
    g_object_unref(d->vadjust);
    g_slice_free(document_data_t, d);
}

static gint
luaH_document_scroll_newindex(lua_State *L)
{
    /* get document widget upvalue */
    document_data_t *d = luaH_checkdocument_data(L, lua_upvalueindex(1));
    const gchar *prop = luaL_checkstring(L, 2);
    luapdf_token_t t = l_tokenize(prop);

    GtkAdjustment *a;
    if (t == L_TK_X)      a = d->hadjust;
    else if (t == L_TK_Y) a = d->vadjust;
    else return 0;

    gdouble value = luaL_checknumber(L, 3);
    gdouble max = gtk_adjustment_get_upper(a) -
            gtk_adjustment_get_page_size(a);
    gtk_adjustment_set_value(a, ((value < 0 ? 0 : value) > max ? max : value));
    return 0;
}

static gint
luaH_document_scroll_index(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, lua_upvalueindex(1));
    const gchar *prop = luaL_checkstring(L, 2);
    luapdf_token_t t = l_tokenize(prop);

    GtkAdjustment *a = (*prop == 'x') ? d->hadjust : d->vadjust;

    if (t == L_TK_X || t == L_TK_Y) {
        lua_pushnumber(L, gtk_adjustment_get_value(a));
        return 1;

    } else if (t == L_TK_XMAX || t == L_TK_YMAX) {
        lua_pushnumber(L, gtk_adjustment_get_upper(a) -
                gtk_adjustment_get_page_size(a));
        return 1;

    } else if (t == L_TK_XPAGE_SIZE || t == L_TK_YPAGE_SIZE) {
        lua_pushnumber(L, gtk_adjustment_get_page_size(a));
        return 1;
    }
    return 0;
}

static gint
luaH_document_page_newindex(lua_State *L)
{
    page_info_t *p = lua_touserdata(L, lua_upvalueindex(1));
    const gchar *prop = luaL_checkstring(L, 2);
    luapdf_token_t t = l_tokenize(prop);

    gdouble value = luaL_checknumber(L, 3);
    if (t == L_TK_X) p->x = value;
    else if (t == L_TK_Y) p->y = value;

    return 0;
}

static gint
luaH_document_page_index(lua_State *L)
{
    page_info_t *p = lua_touserdata(L, lua_upvalueindex(1));
    const gchar *prop = luaL_checkstring(L, 2);
    luapdf_token_t t = l_tokenize(prop);

    switch(t)
    {
      PN_CASE(X, p->x)
      PN_CASE(Y, p->y)
      PN_CASE(WIDTH, p->w)
      PN_CASE(HEIGHT, p->h)

      default:
        break;
    }

    return 0;
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

static gint
luaH_document_push_pages(lua_State *L, document_data_t *d)
{
    lua_createtable(L, 0, d->pages->len);
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        lua_pushlightuserdata(L, p);
        luaH_document_push_indexed_table(L, luaH_document_page_index, luaH_document_page_newindex, lua_gettop(L));
        lua_remove(L, -2);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static void
page_render(cairo_t *c, page_info_t *i)
{
    PopplerPage *p = i->page;
    /* render background */
    cairo_rectangle(c, 0, 0, i->w, i->h);
    cairo_set_source_rgb(c, 1, 1, 1);
    cairo_fill(c);
    /* render page */
    poppler_page_render(p, c);
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
        page_info_t *p = g_slice_new0(page_info_t);
        p->page = poppler_document_get_page(d->document, i);
        poppler_page_get_size(p->page, &p->w, &p->h);
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
document_render(document_data_t *d)
{
    GtkWidget *w = d->widget;

    /* calculate visible region */
    gdouble width = w->allocation.width;
    gdouble height = w->allocation.height;
    cairo_rectangle_int_t rect = {
        d->hadjust->value * d->zoom,
        d->vadjust->value * d->zoom,
        width,
        height,
    };
    cairo_region_t *visible_r = cairo_region_create_rectangle(&rect);

    /* render recorded data directly to widget */
    cairo_t *c = gdk_cairo_create(gtk_widget_get_window(w));
    cairo_set_source_rgb(c, 1.0/256*220, 1.0/256*218, 1.0/256*213);
    cairo_paint(c);

    /* render pages with scroll and zoom */
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        cairo_rectangle_int_t page_rect = {
            p->x * d->zoom,
            p->y * d->zoom,
            p->w * d->zoom,
            p->h * d->zoom,
        };
        if (cairo_region_contains_rectangle(visible_r, &page_rect) != CAIRO_REGION_OVERLAP_OUT) {
            cairo_scale(c, d->zoom, d->zoom);
            cairo_translate(c, p->x - d->hadjust->value, p->y - d->vadjust->value);
            page_render(c, p);
            cairo_identity_matrix(c);
        }
    }
    cairo_region_destroy(visible_r);
    cairo_destroy(c);
}

static gint
luaH_document_index(lua_State *L, luapdf_token_t token)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);

    switch(token)
    {
      LUAPDF_WIDGET_INDEX_COMMON

      /* functions */
      PF_CASE(LOAD,     luaH_document_load)

      /* strings */
      PS_CASE(PATH,     d->path);
      PS_CASE(PASSWORD, d->password);

      /* integers */
      PI_CASE(CURRENT,  d->current + 1);

      /* numbers */
      PN_CASE(ZOOM,     d->zoom);

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
    document_data_t *d = g_slice_new0(document_data_t);
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
