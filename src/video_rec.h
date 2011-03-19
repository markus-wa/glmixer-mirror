/*
 *  GLW Recoder - Record video of display output
 *  Copyright (C) 2010 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FFVHUFF_REC_H__
#define FFVHUFF_REC_H__

typedef struct video_rec video_rec_t;


video_rec_t *mpeg_rec_init(const char *filename, int width, int height, int fps);
video_rec_t *ffvhuff_rec_init(const char *filename, int width, int height, int fps);

void mpeg_rec_deliver_vframe(video_rec_t *gr, void *data);
void ffvhuff_rec_deliver_vframe(video_rec_t *gr, void *data);

void mpeg_rec_stop(video_rec_t *);
void ffvhuff_rec_stop(video_rec_t *);

#endif // FFVHUFF_REC_H__
