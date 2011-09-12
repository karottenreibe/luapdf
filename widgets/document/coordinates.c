/*
 * widgets/document/coordinates.c - Document coordinate helper functions
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

static cairo_rectangle_t *
page_coordinates_from_pdf_coordinates(PopplerRectangle *r, page_info_t *p)
{
    cairo_rectangle_t *pc = g_new(cairo_rectangle_t, 1);
    gdouble x1 = r->x1;
    gdouble x2 = r->x2;
    gdouble y1 = p->rectangle->height - r->y1;
    gdouble y2 = p->rectangle->height - r->y2;
    pc->x = x1;
    pc->y = y2;
    pc->width = x2 - x1;
    pc->height = y1 - y2;
    return pc;
}

static cairo_rectangle_t *
document_coordinates_from_page_coordinates(cairo_rectangle_t *r, page_info_t *p)
{
    cairo_rectangle_t *dc = g_new(cairo_rectangle_t, 1);
    dc->x = p->rectangle->x + r->x;
    dc->y = p->rectangle->y + r->y;
    dc->width = r->width;
    dc->height = r->height;
    return dc;
}

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

static void
document_coordinates_from_widget_coordinates(gdouble x, gdouble y, gdouble *xout, gdouble *yout, document_data_t *d)
{
    *xout = (x / d->zoom) + d->hadjust->value;
    *yout = (y / d->zoom) + d->vadjust->value;
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

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
