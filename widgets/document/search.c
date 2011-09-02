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
    g_free(d->current_search);
    d->current_search = NULL;
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        if (p->search_matches)
            g_list_free(p->search_matches);
    }
}

static gboolean
document_search(document_data_t *d, const gchar *text, gboolean forward, gboolean wrap, guint start_page)
{
    search_data_t *s = d->current_search;
    if (!s) {
        /* start searching from the start page again */
        s = g_new(search_data_t, 1);
        s->text = text;
        s->forward = forward;
        s->wrap = wrap;
        s->current_match = NULL;
        d->current_search = s;

        /* get all matches */
        for (guint i = 0; i < d->pages->len; ++i) {
            page_info_t *p = g_ptr_array_index(d->pages, i);
            if (p->search_matches)
                g_list_free(p->search_matches);
            p->search_matches = poppler_page_find_text(p->page, text);
            if (p->search_matches && !s->current_match && i >= start_page) {
                s->current_page = i;
                s->current_match = p->search_matches;
            }
        }

        if (!s->current_match && wrap) {
            for (guint i = 0; i < d->pages->len; ++i) {
                page_info_t *p = g_ptr_array_index(d->pages, i);
                if (p->search_matches) {
                    s->current_page = i;
                    s->current_match = p->search_matches;
                    break;
                }
            }
        }

        return s->current_match != NULL;
    } else {
        /* get the next/prev match */
        if (forward) {
            GList *m = g_list_next(s->current_match);
            if (m) {
                s->current_match = m;
                return TRUE;
            }
            for (guint i = s->current_page + 1; i < d->pages->len; ++i) {
                page_info_t *p = g_ptr_array_index(d->pages, i);
                if (p->search_matches) {
                    s->current_page = i;
                    s->current_match = p->search_matches;
                    return TRUE;
                }
            }
            if (!wrap) return FALSE;

            for (guint i = 0; i <= s->current_page; ++i) {
                page_info_t *p = g_ptr_array_index(d->pages, i);
                if (p->search_matches) {
                    s->current_page = i;
                    s->current_match = p->search_matches;
                    return TRUE;
                }
            }
            return FALSE;
        } else {
            GList *m = g_list_previous(s->current_match);
            if (m) {
                s->current_match = m;
                return TRUE;
            }
            for (gint i = (gint) s->current_page - 1; i >= 0; --i) {
                page_info_t *p = g_ptr_array_index(d->pages, i);
                if (p->search_matches) {
                    s->current_page = i;
                    s->current_match = p->search_matches;
                    return TRUE;
                }
            }
            if (!wrap) return FALSE;

            for (guint i = d->pages->len - 1; i >= s->current_page; --i) {
                page_info_t *p = g_ptr_array_index(d->pages, i);
                if (p->search_matches) {
                    s->current_page = i;
                    s->current_match = p->search_matches;
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
}

static gint
luaH_document_search(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    const gchar *text = luaL_checkstring(L, 2);
    // TODO: case sensitivity is ignored by poppler
    gboolean forward = luaH_checkboolean(L, 4);
    gboolean wrap = luaH_checkboolean(L, 5);

    search_data_t *s = d->current_search;

    /* init search data */
    guint start_page = 0;
    if (s) start_page = s->current_page;
    /* clear old search if search term or case sensitivity changed */
    if (s && g_strcmp0(s->text, text) != 0) {
        document_clear_search(d);
        s = NULL;
    }

    gboolean success = document_search(d, text, forward, wrap, start_page);
    document_render(d);

    lua_pushboolean(L, success);
    return 1;
}

static gint
luaH_document_highlight_match(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    GList *match = lua_touserdata(L, 2);
    if (!match)
        luaL_typerror(L, 2, "search match");
    d->current_search->current_match = match->data;
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
