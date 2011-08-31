/*
 * widgets/document/printing.c - Poppler document printing
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

static void
draw_page_cb(GtkPrintOperation *UNUSED(op), GtkPrintContext *cx, int index, document_data_t *d)
{
    cairo_t *c = gtk_print_context_get_cairo_context(cx);
    page_info_t* p = g_ptr_array_index(d->pages, index);
    if (p) poppler_page_render(p->page, c);
}

static void
begin_print_cb(GtkPrintOperation *op, GtkPrintContext *UNUSED(cx), document_data_t *d)
{
    GtkPrintSettings *settings = gtk_print_operation_get_print_settings(op);
    gint n = 0;
    GtkPageRange *ranges;
    gint nranges;
    switch(gtk_print_settings_get_print_pages(settings))
    {
      case GTK_PRINT_PAGES_ALL:
        n = d->pages->len;
        break;
      case GTK_PRINT_PAGES_RANGES:
        ranges = gtk_print_settings_get_page_ranges(settings, &nranges);
        for (int i = 0; i < nranges; ++i)
            n += ranges[i].end - ranges[i].start + 1;
      default:
        n = 1; /* selection and current page */
        break;
    }
    gtk_print_operation_set_n_pages(op, n);
}

static const gchar *
document_print(document_data_t *d, page_info_t *p, GtkWindow *w)
{
    GtkPrintOperation *op = gtk_print_operation_new();

    GtkPrintSettings *settings = gtk_print_settings_new();
    if (p) {
        gint index = poppler_page_get_index(p->page) - 1;
        GtkPageRange range = { index, index };
        gtk_print_settings_set_page_ranges(settings, &range, 1);
        gtk_print_settings_set_print_pages(settings, GTK_PRINT_PAGES_RANGES);
    }
    gtk_print_operation_set_print_settings(op, settings);

    g_object_connect(G_OBJECT(op),
      "signal::draw-page",            G_CALLBACK(draw_page_cb),      d,
      "signal::begin-print",          G_CALLBACK(begin_print_cb),    d,
      NULL);

    GError *error = NULL;
    gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, w, &error);
    if (error)
        return error->message;
    return NULL;
}

static gint
luaH_document_print(lua_State *L)
{
    document_data_t *d = luaH_checkdocument_data(L, 1);
    page_info_t *p = NULL;
    if (!lua_isnil(L, 2)) {
        gint index = luaL_checkinteger(L, 2);
        p = g_ptr_array_index(d->pages, index);
    }
    GtkWindow *w = GTK_WINDOW(gtk_widget_get_ancestor(d->widget, GTK_TYPE_WINDOW));
    const gchar *error = document_print(d, p, w);
    if (error) luaL_error(L, error);
    return 0;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
