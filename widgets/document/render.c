/*
 * widgets/document/render.c - Poppler document rendering
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

static gboolean
document_page_is_visible(document_data_t *d, page_info_t *p)
{
    cairo_rectangle_int_t *page_rect = viewport_coordinates_from_document_coordinates(p->rectangle, d);
    gboolean visible = viewport_coordinates_get_visible(page_rect, d);
    g_free(page_rect);
    return visible;
}

static void
page_render(cairo_t *c, page_info_t *i)
{
    PopplerPage *p = i->page;
    /* render background */
    cairo_rectangle(c, 0, 0, i->rectangle->width, i->rectangle->height);
    cairo_set_source_rgb(c, 1, 1, 1);
    cairo_fill(c);
    /* render page */
    poppler_page_render(p, c);
}

static void
document_render(document_data_t *d)
{
    GtkWidget *w = d->widget;

    /* render recorded data directly to widget */
    cairo_t *c = gdk_cairo_create(gtk_widget_get_window(w));
    cairo_set_source_rgb(c, 1.0/256*220, 1.0/256*218, 1.0/256*213);
    cairo_paint(c);

    /* render pages with scroll and zoom */
    for (guint i = 0; i < d->pages->len; ++i) {
        page_info_t *p = g_ptr_array_index(d->pages, i);
        if (document_page_is_visible(d, p)) {
            /* render page */
            cairo_scale(c, d->zoom, d->zoom);
            cairo_translate(c, p->rectangle->x - d->hadjust->value, p->rectangle->y - d->vadjust->value);
            page_render(c, p);
            cairo_identity_matrix(c);

            /* render search matches */
            GList *m = p->search_matches;
            while (m) {
                PopplerRectangle *pr = (PopplerRectangle*) m->data;
                cairo_rectangle_t *pc = page_coordinates_from_pdf_coordinates(pr, p);
                cairo_rectangle_t *dc = document_coordinates_from_page_coordinates(pc, p);
                cairo_scale(c, d->zoom, d->zoom);
                cairo_translate(c, dc->x - d->hadjust->value, dc->y - d->vadjust->value);
                cairo_rectangle(c, 0, 0, dc->width, dc->height);
                if (d->current_match == m)
                    cairo_set_source_rgba(c, 0.5, 1, 0, 0.5);
                else
                    cairo_set_source_rgba(c, 1, 1, 0, 0.5);
                cairo_fill(c);
                cairo_identity_matrix(c);
                g_free(dc);
                g_free(pc);
                m = g_list_next(m);
            }
        }
    }
    cairo_destroy(c);
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
