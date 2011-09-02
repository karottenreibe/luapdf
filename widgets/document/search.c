/*
 * widgets/document/search.c - Poppler document search functions
 *
 * Copyright © 2010 Fabian Streitel <luapdf@rottenrei.be>
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

static void
document_clear_search(document_data_t *d)
{
    d->current_match = NULL;
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        if (p->search_matches)
            g_list_free(p->search_matches);
        p->search_matches = NULL;
    }
}

static gint
luaH_document_highlight_match(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    GList *match = lua_touserdata(L, 2);
    if (!match)
        luaL_typerror(L, 2, "search match");
    d->current_match = match;
    document_render(d);
    return 0;
}

static gint
luaH_document_clear_search(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    document_clear_search(d);
    return 0;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
