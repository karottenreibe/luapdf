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
document_search_match_destroy(gpointer p)
{
    search_match_t *sm = (search_match_t*) p;
    g_free(sm->rectangle);
    g_free(sm);
}

static void
document_clear_search(document_data_t *d)
{
    search_data_t *s = d->last_search;
    if (s) {
        g_list_free_full(s->matches, document_search_match_destroy);
        g_free(s);
    }
    d->last_search = NULL;
}

static gint
luaH_document_search(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    const gchar *text = luaL_checkstring(L, 2);
    gboolean case_sensitive = luaH_checkboolean(L, 3);
    gboolean forward = luaH_checkboolean(L, 4);
    gboolean wrap = luaH_checkboolean(L, 5);

    // TODO: case sensitivity is ignored by poppler
    case_sensitive = FALSE;

    search_data_t *s = d->last_search;

    /* init search data */
    gint start_page = 0;
    if (s) start_page = s->last_page;
    /* clear old search if search term or case sensitivity changed */
    if (s && (g_strcmp0(s->text, text) != 0 || s->case_sensitive != case_sensitive)) {
        document_clear_search(d);
        s = NULL;
    }
    if (!s) {
        s = g_new(search_data_t, 1);
        s->text = text;
        s->case_sensitive = case_sensitive;
        s->forward = forward;
        s->wrap = wrap;
        s->last_page = start_page;
        s->last_match = NULL;

        /* get all matches */
        GList *matches = NULL;
        for (guint i = 0; i < d->pages->len; ++i) {
            page_info_t *p = g_ptr_array_index(d->pages, i);
            GList *poppler_matches = poppler_page_find_text(p->page, text);
            GList *m = poppler_matches;
            while (m) {
                search_match_t *sm = g_new(search_match_t, 1);
                sm->page = i;
                sm->rectangle = (PopplerRectangle*) m->data;
                matches = g_list_prepend(matches, sm);
                m = g_list_next(m);
            }
            g_list_free(poppler_matches);
        }
        s->matches = g_list_reverse(matches);

        d->last_search = s;
    }

    /* get the next finding */
    if (s->matches) {
        GList *match = s->last_match;
        GList *start_point = forward ? s->matches : g_list_last(s->matches);
        if (!match) {
            /* start from the starting page */
            GList *m = start_point;
            while (m) {
                search_match_t *sm = (search_match_t*) m->data;
                if ((forward && sm->page >= start_page) || (!forward && sm->page <= start_page)) {
                    match = m;
                    break;
                }
                m = forward ? g_list_next(m) : g_list_previous(m);
            }
        }
        if (match) match = forward ? g_list_next(match) : g_list_previous(match);
        if (!match && wrap) match = start_point;
    }
    /* highlight it */
    document_render(d);

    gboolean success = (s->last_match != NULL);
    lua_pushboolean(L, success);
    return 1;
}

static gint
luaH_document_clear_search(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    document_clear_search(d);
    return 0;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
