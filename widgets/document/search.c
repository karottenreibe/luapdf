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

static gint
luaH_push_search_matches_table(lua_State *L, page_info_t *p)
{
    lua_newtable(L);
    GList *m = p->search_matches;
    gint i = 1;
    while (m) {
        lua_pushlightuserdata(L, m);
        lua_rawseti(L, -2, i);
        i += 1;
        m = g_list_next(m);
    }
    return 1;
}

static gint
luaH_page_search(lua_State *L)
{
    page_info_t *p = lua_touserdata(L, lua_upvalueindex(1));
    const gchar *text = luaL_checkstring(L, 2);
    p->search_matches = poppler_page_find_text(p->page, text);
    return 0;
}

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
    document_render(d);
}

static gint
luaH_document_highlight_match(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    luaH_checktable(L, 2);

    // TODO: handle this without __data and remove __data again
    lua_pushstring(L, "page");
    lua_gettable(L, -2);
    lua_pushstring(L, "__data");
    lua_rawget(L, -2);
    page_info_t *p = lua_touserdata(L, -1);
    lua_pop(L, 2);

    lua_pushstring(L, "match");
    lua_gettable(L, -2);
    GList *match = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!match || !p)
        luaL_typerror(L, 2, "search match");

    d->current_match = match;

    /* scroll to match */
    PopplerRectangle *r = (PopplerRectangle*) match->data;
    cairo_rectangle_t *pc = page_coordinates_from_pdf_coordinates(r, p);
    cairo_rectangle_t *dc = document_coordinates_from_page_coordinates(pc, p);
    gint clamp_margin = 10;
    gtk_adjustment_clamp_page(d->hadjust, dc->x - clamp_margin, dc->x + dc->width + clamp_margin);
    gtk_adjustment_clamp_page(d->vadjust, dc->y - clamp_margin, dc->y + dc->height + clamp_margin);
    g_free(dc);
    g_free(pc);

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
