/*
 * widgets/document/scroll.c - Poppler document scrolling functions
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

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
