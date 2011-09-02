/*
 * widgets/document/pages.c - Poppler document page functions
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

static gint
document_current_page(document_data_t *d)
{
    GtkWidget *w = d->widget;
    gdouble midx = d->hadjust->value + (w->allocation.width / d->zoom / 2);
    gdouble midy = d->vadjust->value + (w->allocation.height / d->zoom / 2);
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        cairo_rectangle_int_t rect = {
            p->rectangle->x,
            p->rectangle->y,
            p->rectangle->width,
            p->rectangle->height,
        };
        cairo_region_t *r = cairo_region_create_rectangle(&rect);
        if (cairo_region_contains_point(r, midx, midy))
            return i;
    }
    return 0;
}

static gint
luaH_push_search_matches_table(lua_State *L, page_info_t *p)
{
    lua_newtable(L);
    GList *m = p->search_matches;
    gint i = 1;
    while(m) {
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
    const gchar *text = luaL_checkstring(L, 1);
    p->search_matches = poppler_page_find_text(p->page, text);
    return 0;
}

static gint
luaH_document_page_newindex(lua_State *L)
{
    page_info_t *p = lua_touserdata(L, lua_upvalueindex(1));
    const gchar *prop = luaL_checkstring(L, 2);
    luapdf_token_t t = l_tokenize(prop);

    gdouble value = luaL_checknumber(L, 3);
    if (t == L_TK_X) p->rectangle->x = value;
    else if (t == L_TK_Y) p->rectangle->y = value;

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
      /* closures */
      case L_TK_SEARCH:
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushcclosure(L, luaH_page_search, 1);
        return 1;

      /* numbers */
      PN_CASE(X, p->rectangle->x)
      PN_CASE(Y, p->rectangle->y)
      PN_CASE(WIDTH, p->rectangle->width)
      PN_CASE(HEIGHT, p->rectangle->height)

      case L_TK_SEARCH_MATCHES:
        luaH_push_search_matches_table(L, p);
        return 1;

      default:
        break;
    }

    return 0;
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

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
