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
luaH_document_page_newindex(lua_State *L)
{
    page_info_t *p = lua_touserdata(L, lua_upvalueindex(1));
    const gchar *prop = luaL_checkstring(L, 2);
    luapdf_token_t t = l_tokenize(prop);

    gdouble value = luaL_checknumber(L, 3);
    if (t == L_TK_X) p->x = value;
    else if (t == L_TK_Y) p->y = value;

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
      PN_CASE(X, p->x)
      PN_CASE(Y, p->y)
      PN_CASE(WIDTH, p->w)
      PN_CASE(HEIGHT, p->h)

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