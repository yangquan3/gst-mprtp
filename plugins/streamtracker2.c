/* GStreamer Scheduling tree
 * Copyright (C) 2015 Balázs Kreith (contact: balazs.kreith@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include "streamtracker2.h"
#include <math.h>
#include <string.h>

//#define THIS_READLOCK(this) g_rw_lock_reader_lock(&this->rwmutex)
//#define THIS_READUNLOCK(this) g_rw_lock_reader_unlock(&this->rwmutex)
//#define THIS_WRITELOCK(this) g_rw_lock_writer_lock(&this->rwmutex)
//#define THIS_WRITEUNLOCK(this) g_rw_lock_writer_unlock(&this->rwmutex)

#define THIS_READLOCK(this)
#define THIS_READUNLOCK(this)
#define THIS_WRITELOCK(this)
#define THIS_WRITEUNLOCK(this)


GST_DEBUG_CATEGORY_STATIC (streamtracker2_debug_category);
#define GST_CAT_DEFAULT streamtracker2_debug_category

G_DEFINE_TYPE (StreamTracker2, streamtracker2, G_TYPE_OBJECT);

//----------------------------------------------------------------------
//-------- Private functions belongs to Scheduler tree object ----------
//----------------------------------------------------------------------

static void streamtracker2_finalize (GObject * object);
static void
_add_value(StreamTracker2 *this, guint64 value);
static void
_obsolate (StreamTracker2 * this);

//----------------------------------------------------------------------
//--------- Private functions implementations to SchTree object --------
//----------------------------------------------------------------------

void
streamtracker2_class_init (StreamTracker2Class * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  gobject_class->finalize = streamtracker2_finalize;

  GST_DEBUG_CATEGORY_INIT (streamtracker2_debug_category, "streamtracker2", 0,
      "StreamTracker2");

}

void
streamtracker2_finalize (GObject * object)
{
  StreamTracker2 *this;
  this = STREAMTRACKER2(object);
  g_object_unref(this->sysclock);
  g_free(this->items);
}

void
streamtracker2_init (StreamTracker2 * this)
{
  g_rw_lock_init (&this->rwmutex);
}

StreamTracker2 *make_streamtracker2(guint32 length)
{
  StreamTracker2 *result;
  result = g_object_new (STREAMTRACKER2_TYPE, NULL);
  THIS_WRITELOCK (result);
  result->items = g_malloc0(sizeof(StreamTracker2Item)*length);
  result->sum = 0;
  result->length = length;
  result->sysclock = gst_system_clock_obtain();
  result->treshold = GST_SECOND;
  result->counter = 0;
  THIS_WRITEUNLOCK (result);

  return result;
}

void streamtracker2_reset(StreamTracker2 *this)
{
  THIS_WRITELOCK (this);
  memset(this->items, 0, sizeof(StreamTracker2Item) * this->length);
  this->counter = this->write_index = this->read_index = 0;
  this->sum = 0;
  THIS_WRITEUNLOCK (this);
}

void streamtracker2_add(StreamTracker2 *this, guint64 value)
{
  THIS_WRITELOCK (this);
  _add_value(this, value);
  THIS_WRITEUNLOCK (this);
}

void streamtracker2_set_treshold(StreamTracker2 *this, GstClockTime treshold)
{
  THIS_WRITELOCK (this);
  this->treshold = treshold;
  THIS_WRITEUNLOCK (this);
}

guint32 streamtracker2_get_num(StreamTracker2 *this)
{
  guint32 result;
  THIS_READLOCK(this);
  result = this->counter;
  THIS_READUNLOCK(this);
  return result;
}

guint64 streamtracker2_get_last(StreamTracker2 *this)
{
  guint64 result;
  THIS_READLOCK(this);
  if(this->read_index == this->write_index) result = 0;
  else if(this->write_index == 0) result = this->items[this->length-1].value;
  else result = this->items[this->write_index-1].value;
  THIS_READUNLOCK(this);
  return result;
}


void
streamtracker2_get_stats (StreamTracker2 * this,
                         guint64 *sum,
                         guint32 *items_num)
{
  THIS_READLOCK (this);
  if(sum) *sum = this->sum;
  if(items_num) *items_num = this->counter;
  THIS_READUNLOCK (this);
}



void
streamtracker2_obsolate (StreamTracker2 * this)
{
  THIS_READLOCK (this);
  _obsolate(this);
  THIS_READUNLOCK (this);
}




void _add_value(StreamTracker2 *this, guint64 value)
{
  GstClockTime now;
  now = gst_clock_get_time(this->sysclock);
  //add new one
  this->sum += value;
  this->items[this->write_index].value = value;
  this->items[this->write_index].added = now;
  if(++this->write_index == this->length){
      this->write_index=0;
  }
  ++this->counter;
  _obsolate(this);
}

void
_obsolate (StreamTracker2 * this)
{
  GstClockTime treshold,now;
  now = gst_clock_get_time(this->sysclock);
  treshold = now - this->treshold;
again:
  if(this->write_index == this->read_index) goto elliminate;
  else if(this->items[this->read_index].added < treshold) goto elliminate;
  else goto done;
elliminate:
  this->sum -= this->items[this->read_index].value;
  this->items[this->read_index].value = 0;
  this->items[this->read_index].added = 0;
  if(++this->read_index == this->length){
      this->read_index=0;
  }
  --this->counter;
  goto again;
done:
  return;
}

#undef THIS_WRITELOCK
#undef THIS_WRITEUNLOCK
#undef THIS_READLOCK
#undef THIS_READUNLOCK
