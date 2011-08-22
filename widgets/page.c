/*
 * widgets/page.c - Poppler page widget
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

#include "widgets/page.h"
#include "luah.h"
#include "clib/widget.h"
#include "widgets/common.h"

#include <glib.h>

typedef struct {
    /* functions of the superclass */
    widget_destructor_t *super_destructor;
    gint (*super_index)(lua_State *, luapdf_token_t);
    gint (*super_newindex)(lua_State *, luapdf_token_t);
    /* page data */
    PopplerPage *page;
    int index;
    PopplerDocument *document;
} page_data_t;

static widget_t*
luaH_checkpage(lua_State *L, gint udx)
{
    widget_t *w = luaH_checkwidget(L, udx);
    if (w->info->tok != L_TK_PAGE)
        luaL_argerror(L, udx, "incorrect widget type (expected page)");
    return w;
}

static void
luaH_page_destructor(widget_t *w) {
    page_data_t *d = w->data;
    widget_destructor(w);
    /* release our reference on the page. Poppler handles freeing it */
    if (d->page)
        g_object_unref(d->page);
    g_slice_free(page_data_t, d);
}

int
luaH_page_new(lua_State *L, PopplerDocument *document, int index)
{
    /* push {type = "page"} to index 2 */
    lua_newtable(L);
    lua_pushstring(L, "type");
    lua_pushstring(L, "page");
    lua_rawset(L, -3);
    lua_insert(L, 2);
    /* call widget constructor and cleanup */
    luaH_widget_new(L);
    lua_remove(L, 2);
    /* get widget and set properties */
    widget_t *w = luaH_checkpage(L, -1);
    page_data_t *d = w->data;
    d->document = document;
    d->index = index;
    d->page = poppler_document_get_page(document, index);
    /* render page to pixbuf */
    PopplerPage *p = d->page;
    double width, height;
    poppler_page_get_size(p, &width, &height);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *c = cairo_create(s);
    poppler_page_render(p, c);
    int stride = cairo_image_surface_get_stride(s);
    guchar *data = cairo_image_surface_get_data(s);
    GdkPixbuf *buf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, TRUE, 8, width, height, stride, NULL, NULL);
    /* render pixbuf to image widget */
    gtk_image_set_from_pixbuf(GTK_IMAGE(w->widget), buf);
    return 1;
}

static gint
luaH_page_index(lua_State *L, luapdf_token_t token)
{
    switch(token)
    {
      LUAKIT_WIDGET_INDEX_COMMON

      default:
        break;
    }
    return 0;
}

widget_t *
widget_page(widget_t *w, luapdf_token_t UNUSED(token))
{
    w->index = luaH_page_index;
    w->newindex = NULL;
    w->destructor = luaH_page_destructor;

    /* create gtk image widget as main widget */
    w->widget = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
    g_object_set_data(G_OBJECT(w->widget), "lua_widget", (gpointer) w);

    gtk_widget_show(w->widget);

    /* create data struct */
    page_data_t *d = g_slice_new0(page_data_t);
    w->data = d;

    return w;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
