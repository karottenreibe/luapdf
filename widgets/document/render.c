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

static cairo_rectangle_int_t *
viewport_coordinates_from_document_coordinates(cairo_rectangle_t *r, document_data_t *d)
{
    cairo_rectangle_int_t *vr = g_new(cairo_rectangle_int_t, 1);
    vr->x = r->x * d->zoom;
    vr->y = r->y * d->zoom;
    vr->width = r->width * d->zoom;
    vr->height = r->height * d->zoom;
    return vr;
}

static gboolean
viewport_coordinates_get_visible(cairo_rectangle_int_t *r, document_data_t *d)
{
    /* calculate visible region */
    GtkWidget *w = d->widget;
    gdouble width = w->allocation.width;
    gdouble height = w->allocation.height;
    cairo_rectangle_int_t rect = {
        d->hadjust->value * d->zoom,
        d->vadjust->value * d->zoom,
        width,
        height,
    };
    cairo_region_t *visible_r = cairo_region_create_rectangle(&rect);

    gboolean visible = (cairo_region_contains_rectangle(visible_r, r) != CAIRO_REGION_OVERLAP_OUT);
    cairo_region_destroy(visible_r);
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
        cairo_rectangle_int_t *page_rect = viewport_coordinates_from_document_coordinates(p->rectangle, d);
        if (viewport_coordinates_get_visible(page_rect, d)) {
            cairo_scale(c, d->zoom, d->zoom);
            cairo_translate(c, p->rectangle->x - d->hadjust->value, p->rectangle->y - d->vadjust->value);
            page_render(c, p);
            cairo_identity_matrix(c);
        }
        g_free(page_rect);
    }
    cairo_destroy(c);
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
