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
#include "widgets/page.h"

#include <glib.h>
#include <poppler.h>

typedef struct {
    /* functions of the superclass */
    widget_destructor_t *super_destructor;
    gint (*super_index)(lua_State *, luapdf_token_t);
    gint (*super_newindex)(lua_State *, luapdf_token_t);
    /* document data */
    PopplerDocument *document;
    const gchar *path;
    const gchar *password;
} document_data_t;

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
    d->super_destructor(w);
    /* release our reference on the document. Poppler handles freeing it */
    if (d->document)
        g_object_unref(d->document);
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
    return 0;
}

static int
luaH_document_push_pages(lua_State *L, document_data_t *d)
{
    lua_newtable(L);
    PopplerDocument *doc = d->document;
    int n = poppler_document_get_n_pages(doc);
    for (int i = 0; i < n; ++i) {
        luaH_page_new(L, doc, i);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
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

      case L_TK_PAGES:
        return luaH_document_push_pages(L, d);

      default:
        return d->super_index(L, token);
    }
    return 0;
}

static gint
luaH_document_newindex(lua_State *L, luapdf_token_t token)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);

    switch(token)
    {
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
        return d->super_newindex(L, token);
    }

    return luaH_object_emit_property_signal(L, 1);
}

widget_t *
widget_document(widget_t *w, luapdf_token_t token)
{
    /* super constructor */
    widget_eventbox(w, token);

    document_data_t *d = g_slice_new0(document_data_t);
    w->data = d;

    d->super_index = w->index;
    d->super_newindex = w->newindex;
    d->super_destructor = w->destructor;

    w->index = luaH_document_index;
    w->newindex = luaH_document_newindex;
    w->destructor = luaH_document_destructor;

    return w;
}

#undef luaH_checkdocument

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
