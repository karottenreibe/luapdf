/*
 * document.c - Poppler document class
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

#include "clib/document.h"
#include "clib/page.h"
#include "luah.h"

#include <glib.h>
#include <poppler.h>

typedef struct {
    LUA_OBJECT_HEADER
    PopplerDocument *document;
    const gchar *uri;
    const gchar *password;
} ldocument_t;

static lua_class_t document_class;
LUA_OBJECT_FUNCS(document_class, ldocument_t, document)

#define luaH_checkdocument(L, idx) luaH_checkudata(L, idx, &(document_class))

static int
luaH_document_gc(lua_State *L) {
    ldocument_t *document = luaH_checkdocument(L, -1);
    g_free(document->document);
    return 0;
}

static int
luaH_document_new(lua_State *L)
{
    luaH_class_new(L, &document_class);
    ldocument_t *document = luaH_checkdocument(L, -1);
    GError *error = NULL;
    document->document = poppler_document_new_from_file(document->uri, document->password, &error);
    if (error) {
        lua_pushstring(L, error->message);
        lua_error(L);
    }
    return 1;
}

static int
luaH_document_set_uri(lua_State *L, ldocument_t *document)
{
    document->uri = luaL_checkstring(L, -1);
    return 0;
}

static int
luaH_document_get_uri(lua_State *L, ldocument_t *document)
{
    lua_pushstring(L, document->uri);
    return 1;
}

static int
luaH_document_set_password(lua_State *L, ldocument_t *document)
{
    if (lua_isnil(L, -1))
        document->password = NULL;
    else
        document->password = luaL_checkstring(L, -1);
    return 0;
}

static int
luaH_document_get_password(lua_State *L, ldocument_t *document)
{
    if (document->password)
        lua_pushstring(L, document->password);
    else
        lua_pushnil(L);
    return 1;
}

static int
luaH_document_get_pages(lua_State *L, ldocument_t *document)
{
    lua_newtable(L);
    PopplerDocument *doc = document->document;
    int n = poppler_document_get_n_pages(doc);
    for (int i = 0; i < n; ++i) {
        luaH_page_new(L, doc, i);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

void
document_class_setup(lua_State *L)
{
    static const struct luaL_reg document_methods[] =
    {
        LUA_CLASS_METHODS(document)
        { "__call", luaH_document_new },
        { NULL, NULL }
    };

    static const struct luaL_reg document_meta[] =
    {
        LUA_OBJECT_META(document)
        LUA_CLASS_META
        { "__gc", luaH_document_gc },
        { NULL, NULL },
    };

    luaH_class_setup(L, &document_class, "document",
            (lua_class_allocator_t) document_new,
            luaH_class_index_miss_property, luaH_class_newindex_miss_property,
            document_methods, document_meta);

    luaH_class_add_property(&document_class, L_TK_URI,
            (lua_class_propfunc_t) luaH_document_set_uri,
            (lua_class_propfunc_t) luaH_document_get_uri,
            (lua_class_propfunc_t) luaH_document_set_uri);

    luaH_class_add_property(&document_class, L_TK_PAGES,
            NULL,
            (lua_class_propfunc_t) luaH_document_get_pages,
            NULL);

    luaH_class_add_property(&document_class, L_TK_PASSWORD,
            (lua_class_propfunc_t) luaH_document_set_password,
            (lua_class_propfunc_t) luaH_document_get_password,
            (lua_class_propfunc_t) luaH_document_set_password);
}

#undef luaH_checkdocument

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
