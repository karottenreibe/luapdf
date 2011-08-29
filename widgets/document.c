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
    /* widgets */
    GtkWidget *image;
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
    gtk_widget_destroy(GTK_WIDGET(d->image));
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
    g_free(d->hadjust);
    g_free(d->vadjust);
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
luaH_document_push_scroll_table(lua_State *L)
{
    /* create scroll table */
    lua_newtable(L);
    /* setup metatable */
    lua_createtable(L, 0, 2);
    /* push __index metafunction */
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, 1); /* copy document userdata */
    lua_pushcclosure(L, luaH_document_scroll_index, 1);
    lua_rawset(L, -3);
    /* push __newindex metafunction */
    lua_pushliteral(L, "__newindex");
    lua_pushvalue(L, 1); /* copy document userdata */
    lua_pushcclosure(L, luaH_document_scroll_newindex, 1);
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
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

    /* calculate page geometry and positioning */
    // TODO: push layouting into lua by emitting a signal on the doc here
    gdouble width = 0;
    gdouble height = 0;
    gdouble page_width = 0;
    gdouble page_height = 0;
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        p->y = height;
        poppler_page_get_size(p->page, &p->w, &p->h);
        if (p->w > width)
            width = p->w;
        height += p->h + d->spacing;
        if (!page_width)  page_width = p->w;
        if (!page_height) page_height = p->h;
    }
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        p->x = (width - p->w) / 2;
    }
    if (height > 0)
        height -= d->spacing;

    d->width = width;
    d->height = height;

    /* configure adjustments */
    d->hadjust->upper = width;
    d->hadjust->page_increment = page_width;
    d->vadjust->upper = height;
    d->vadjust->page_increment = page_height;
    return 0;
}

static void
document_render(document_data_t *d)
{
    GtkWidget *image = GTK_WIDGET(d->image);
    if (!gtk_widget_get_visible(image))
        return;

    /* calculate visible region */
    gdouble width = image->allocation.width;
    gdouble height = image->allocation.height;
    // TODO remove
    width = 600;
    height = 600;
    cairo_rectangle_int_t rect = {
        d->hadjust->value,
        d->vadjust->value,
        width,
        height,
    };
    cairo_region_t *visible_r = cairo_region_create_rectangle(&rect);

    /* render recorded data into image surface */
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *c = cairo_create(s);

    /* render pages with scroll and zoom */
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        cairo_rectangle_int_t page_rect = {
            (p->x - d->hadjust->value) * d->zoom,
            (p->y - d->vadjust->value) * d->zoom,
            p->w * d->zoom,
            p->h * d->zoom,
        };
        if (cairo_region_contains_rectangle(visible_r, &page_rect) != CAIRO_REGION_OVERLAP_OUT) {
            cairo_scale(c, d->zoom, d->zoom);
            cairo_translate(c, p->x, p->y - d->vadjust->value);
            page_render(c, p);
            cairo_identity_matrix(c);
        }
    }
    cairo_region_destroy(visible_r);

    /* render to image widget */
    gint stride = cairo_image_surface_get_stride(s);
    guchar *data = cairo_image_surface_get_data(s);
    GdkPixbuf *buf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, TRUE, 8, width, height, stride, NULL, NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(d->image), buf);
    cairo_destroy(c);
    // TODO: segfaults
    //cairo_surface_destroy(is);
}

static gint
luaH_document_index(lua_State *L, luapdf_token_t token)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);

    switch(token)
    {
      /* functions */
      PF_CASE(LOAD,     luaH_document_load)

      /* strings */
      PS_CASE(PATH,     d->path);
      PS_CASE(PASSWORD, d->password);

      /* integers */
      PI_CASE(CURRENT,  d->current + 1);

      case L_TK_SCROLL:
        return luaH_document_push_scroll_table(L);

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

      default:
        warn("unknown property: %s", luaL_checkstring(L, 2));
        return 0;
    }

    return luaH_object_emit_property_signal(L, 1);
}

void
render_cb(gpointer *UNUSED(p), document_data_t *d)
{
    printf("render_cb\n");
    document_render(d);
}

widget_t *
widget_document(widget_t *w, luapdf_token_t token)
{
    /* super constructor */
    widget_eventbox(w, token);

    document_data_t *d = g_slice_new0(document_data_t);
    d->spacing = 10;
    d->zoom = 1.0;
    d->image = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
    d->hadjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 10, 1, 0));
    d->vadjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 10, 1, 0));
    w->data = d;

    /* rerender view if the scroll changes */
    g_object_connect(G_OBJECT(d->hadjust),
      "signal::value-changed",        G_CALLBACK(render_cb),     d,
      NULL);
    g_object_connect(G_OBJECT(d->vadjust),
      "signal::value-changed",        G_CALLBACK(render_cb),     d,
      NULL);

    w->widget = d->image;
    /* set gobject property to give other widgets a pointer to our image widget */
    g_object_set_data(G_OBJECT(w->widget), "lua_widget", w);

    w->index = luaH_document_index;
    w->newindex = luaH_document_newindex;
    w->destructor = luaH_document_destructor;

    /* show widgets */
    gtk_widget_show(d->image);

    return w;
}

#undef luaH_checkdocument

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
