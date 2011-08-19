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

#include <glib.h>

typedef struct {
    LUA_OBJECT_HEADER
    PopplerPage *page;
} lpage_t;

static lua_class_t page_class;
LUA_OBJECT_FUNCS(page_class, lpage_t, page)

#define luaH_checkpage(L, idx) luaH_checkudata(L, idx, &(page_class))

static int
luaH_page_gc(lua_State *L) {
    lpage_t *page = luaH_checkpage(L, -1);
    g_free(page->page);
    return 0;
}

int
luaH_page_new(lua_State *L, PopplerDocument *document, int index)
{
    luaH_class_new(L, &page_class);
    lpage_t *page = luaH_checkpage(L, -1);
    page->page = poppler_document_get_page(document, index);
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
}

#undef luaH_checkpage

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
