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
#include "widgets/page.h"

#include <gtk/gtk.h>
#include <poppler.h>

typedef struct {
    /* document */
    PopplerDocument *document;
    const gchar *path;
    const gchar *password;
    /* pages */
    gint current;
    gpointer pages_ref;
    GPtrArray *pages;
    /* drawing data */
    cairo_t *context;
    gint spacing;
    /* widgets */
    GtkWidget *image;
    GtkWidget *win;
} document_data_t;

typedef struct {
    PopplerPage *page;
    gdouble x;
    gdouble y;
    gdouble w;
    gdouble h;
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
    gtk_widget_destroy(GTK_WIDGET(d->image));
    gtk_widget_destroy(GTK_WIDGET(d->win));
    /* release our reference on the document. Poppler handles freeing it */
    if (d->document)
        g_object_unref(G_OBJECT(d->document));
    /* release our references on the pages. Poppler handles freeing it */
    if (d->pages) {
        for (guint i = 0; i < d->pages->len; ++i) {
            page_info_t *p = g_ptr_array_index(d->pages, i);
            g_object_unref(G_OBJECT(p->page));
            g_slice_free(page_info_t, p);
        }
        g_ptr_array_free(d->pages, TRUE);
    }
    g_slice_free(document_data_t, d);
}

static int
luaH_document_load(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, -1);
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
        g_ptr_array_add(d->pages, p);
    }

    /* create cairo context */
    // TODO: push layouting into lua by emitting a signal on the doc here
    gdouble width = 0;
    gdouble height = 0;
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        p->y = height;
        poppler_page_get_size(p->page, &p->w, &p->h);
        if (p->w > width)
            width = p->w;
        height += p->h + d->spacing;
    }
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        p->x = (width - p->w) / 2;
    }
    if (height > 0)
        height -= d->spacing;
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    d->context = cairo_create(s);
    return 0;
}

static int
luaH_document_goto(lua_State *L)
{
    widget_t *w = luaH_checkdocument(L, 1);
    widget_t *p = luaH_checkwidget(L, 2);
    /* remove old child */
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(w->widget));
    if (child) {
        g_object_ref(G_OBJECT(child));
        gtk_container_remove(GTK_CONTAINER(w->widget), GTK_WIDGET(child));
    }
    gtk_container_add(GTK_CONTAINER(w->widget), p->widget);
    document_data_t *d = w->data;
    d->current = page_get_index(p);
    return 0;
}

static gint
luaH_document_index(lua_State *L, luapdf_token_t token)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);

    switch(token)
    {
      /* functions */
      PF_CASE(LOAD,     luaH_document_load)
      PF_CASE(GOTO,     luaH_document_goto)

      /* strings */
      PS_CASE(PATH,     d->path);
      PS_CASE(PASSWORD, d->password);

      /* integers */
      PI_CASE(CURRENT,  d->current + 1);

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
      LUAKIT_WIDGET_INDEX_COMMON

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

      default:
        warn("unknown property: %s", luaL_checkstring(L, 2));
        return 0;
    }

    return luaH_object_emit_property_signal(L, 1);
}

widget_t *
widget_document(widget_t *w, luapdf_token_t token)
{
    /* super constructor */
    widget_eventbox(w, token);

    document_data_t *d = g_slice_new0(document_data_t);
    d->spacing = 10;
    d->image = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
    d->win = gtk_scrolled_window_new(NULL, NULL);
    w->data = d;

    w->widget = d->win;

    /* set gobject property to give other widgets a pointer to our webview */
    g_object_set_data(G_OBJECT(w->widget), "lua_widget", w);

    /* add image to scrolled window */
    gtk_container_add(GTK_CONTAINER(d->win), d->image);

    w->index = luaH_document_index;
    w->newindex = luaH_document_newindex;
    w->destructor = luaH_document_destructor;

    /* show widgets */
    gtk_widget_show(d->image);
    gtk_widget_show(d->win);

    return w;
}

#undef luaH_checkdocument

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
