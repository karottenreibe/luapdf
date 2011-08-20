/*
 * page.c - Poppler page class
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

#include "clib/page.h"
#include "luah.h"
#include "clib/widget.h"
#include "widgets/image.h"

#include <glib.h>

typedef struct {
    LUA_OBJECT_HEADER
    PopplerPage *page;
    gpointer widget_ref;
} lpage_t;

static lua_class_t page_class;
LUA_OBJECT_FUNCS(page_class, lpage_t, page)

#define luaH_checkpage(L, idx) luaH_checkudata(L, idx, &(page_class))

static int
luaH_page_gc(lua_State *L) {
    lpage_t *page = luaH_checkpage(L, -1);
    if (page->widget_ref)
        luaH_object_unref(L, page->widget_ref);
    g_free(page->page);
    return 0;
}

int
luaH_page_new(lua_State *L, PopplerDocument *document, int index)
{
    lua_newtable(L);
    lua_insert(L, 2);
    luaH_class_new(L, &page_class);
    lua_remove(L, 2);
    lpage_t *page = luaH_checkpage(L, -1);
    page->page = poppler_document_get_page(document, index);
    page->widget_ref = NULL;
    return 1;
}

static int
luaH_page_get_widget(lua_State *L, lpage_t *page)
{
    if (!page->widget_ref) {
        /* create image widget */
        lua_newtable(L);
        lua_pushstring(L, "type");
        lua_pushstring(L, "image");
        lua_rawset(L, -3);
        lua_insert(L, 2);
        luaH_widget_new(L);
        lua_remove(L, 2);
        /* set contents */
        PopplerPage *p = page->page;
        double w, h;
        poppler_page_get_size(p, &w, &h);
        cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        cairo_t *c = cairo_create(s);
        poppler_page_render(p, c);
        int stride = cairo_image_surface_get_stride(s);
        guchar *data = cairo_image_surface_get_data(s);
        GdkPixbuf *buf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, TRUE, 8, w, h, stride, NULL, NULL);
        luaH_image_set_pixbuf(L, buf);
        /* ref it */
        page->widget_ref = luaH_object_ref(L, -1);
    }
    luaH_object_push(L, page->widget_ref);
    return 1;
}

void
page_class_setup(lua_State *L)
{
    static const struct luaL_reg page_methods[] =
    {
        LUA_CLASS_METHODS(page)
        { NULL, NULL }
    };

    static const struct luaL_reg page_meta[] =
    {
        LUA_OBJECT_META(page)
        LUA_CLASS_META
        { "__gc", luaH_page_gc },
        { NULL, NULL },
    };

    luaH_class_setup(L, &page_class, "page",
            (lua_class_allocator_t) page_new,
            luaH_class_index_miss_property, luaH_class_newindex_miss_property,
            page_methods, page_meta);

    luaH_class_add_property(&page_class, L_TK_WIDGET,
            NULL,
            (lua_class_propfunc_t) luaH_page_get_widget,
            NULL);
}

#undef luaH_checkpage

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
