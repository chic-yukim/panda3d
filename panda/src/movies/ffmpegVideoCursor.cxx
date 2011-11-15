// Filename: ffmpegVideoCursor.cxx
// Created by: jyelon (01Aug2007)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#ifdef HAVE_FFMPEG

#include "ffmpegVideoCursor.h"
#include "config_movies.h"
extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
#ifdef HAVE_SWSCALE
  #include "libswscale/swscale.h"
#endif
}
#include "pStatCollector.h"
#include "pStatTimer.h"
#include "mutexHolder.h"
#include "reMutexHolder.h"

ReMutex FfmpegVideoCursor::_av_lock;
TypeHandle FfmpegVideoCursor::_type_handle;


#if LIBAVFORMAT_VERSION_MAJOR < 53
  #define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#endif

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::Default Constructor
//       Access: Private
//  Description: This constructor is only used when reading from a bam
//               file.
////////////////////////////////////////////////////////////////////
FfmpegVideoCursor::
FfmpegVideoCursor() :
  _max_readahead_frames(0),
  _thread_priority(ffmpeg_thread_priority),
  _lock("FfmpegVideoCursor::_lock"),
  _action_cvar(_lock),
  _thread_status(TS_stopped),
  _seek_frame(0),
  _packet0(NULL),
  _packet1(NULL),
  _format_ctx(NULL),
  _video_ctx(NULL),
  _convert_ctx(NULL),
  _video_index(-1),
  _frame(NULL),
  _frame_out(NULL),
  _eof_known(false)
{
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::init_from
//       Access: Private
//  Description: Specifies the source of the video cursor.  This is
//               normally called only by the constructor or when
//               reading from a bam file.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
init_from(FfmpegVideo *source) {
  nassertv(_thread == NULL && _thread_status == TS_stopped);
  nassertv(source != NULL);
  _source = source;
  _filename = _source->get_filename();

  if (!open_stream()) {
    cleanup();
    return;
  }

  ReMutexHolder av_holder(_av_lock);
  
#ifdef HAVE_SWSCALE
  nassertv(_convert_ctx == NULL);
  _convert_ctx = sws_getContext(_size_x, _size_y,
                                _video_ctx->pix_fmt, _size_x, _size_y,
                                PIX_FMT_BGR24, SWS_BILINEAR | SWS_PRINT_INFO, NULL, NULL, NULL);
#endif  // HAVE_SWSCALE

  _frame = avcodec_alloc_frame();
  _frame_out = avcodec_alloc_frame();

  if ((_frame == 0)||(_frame_out == 0)) {
    cleanup();
    return;
  }

  _packet0 = new AVPacket;
  _packet1 = new AVPacket;
  memset(_packet0, 0, sizeof(AVPacket));
  memset(_packet1, 0, sizeof(AVPacket));
  
  fetch_packet(0);
  _initial_dts = _packet0->dts;
  fetch_frame(-1);

  _current_frame = -1;
  _eof_known = false;
  _eof_frame = 0;

#ifdef HAVE_THREADS
  set_max_readahead_frames(ffmpeg_max_readahead_frames);
#endif  // HAVE_THREADS
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
FfmpegVideoCursor::
FfmpegVideoCursor(FfmpegVideo *src) : 
  _max_readahead_frames(0),
  _thread_priority(ffmpeg_thread_priority),
  _lock("FfmpegVideoCursor::_lock"),
  _action_cvar(_lock),
  _thread_status(TS_stopped),
  _seek_frame(0),
  _packet0(NULL),
  _packet1(NULL),
  _format_ctx(NULL),
  _video_ctx(NULL),
  _convert_ctx(NULL),
  _video_index(-1),
  _frame(NULL),
  _frame_out(NULL),
  _eof_known(false)
{
  init_from(src);
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::Destructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
FfmpegVideoCursor::
~FfmpegVideoCursor() {
  cleanup();
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::set_max_readahead_frames
//       Access: Published
//  Description: Specifies the maximum number of frames that a
//               sub-thread will attempt to read ahead of the current
//               frame.  Setting this to a nonzero allows the video
//               decoding to take place in a sub-thread, which
//               smoothes out the video decoding time by spreading it
//               evenly over several frames.  Set this number larger
//               to increase the buffer between the currently visible
//               frame and the first undecoded frame; set it smaller
//               to reduce memory consumption.
//
//               Setting this to zero forces the video to be decoded
//               in the main thread.  If threading is not available in
//               the Panda build, this value is always zero.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
set_max_readahead_frames(int max_readahead_frames) {
#ifndef HAVE_THREADS
  if (max_readahead_frames > 0) {
    ffmpeg_cat.warning()
      << "Couldn't set max_readahead_frames to " << max_readahead_frames
      << ": threading not available.\n";
    max_readahead_frames = 0;
  }
#endif  // HAVE_THREADS

  _max_readahead_frames = max_readahead_frames;
  if (_max_readahead_frames > 0) {
    if (_thread_status == TS_stopped) {
      start_thread();
    }
  } else {
    if (_thread_status != TS_stopped) {
      stop_thread();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::get_max_readahead_frames
//       Access: Published
//  Description: Returns the maximum number of frames that a
//               sub-thread will attempt to read ahead of the current
//               frame.  See set_max_readahead_frames().
////////////////////////////////////////////////////////////////////
int FfmpegVideoCursor::
get_max_readahead_frames() const {
  return _max_readahead_frames;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::set_thread_priority
//       Access: Published
//  Description: Changes the thread priority of the thread that
//               decodes the ffmpeg video stream (if
//               max_readahead_frames is nonzero).  Normally you
//               shouldn't mess with this, but there may be special
//               cases where a precise balance of CPU utilization
//               between the main thread and the various ffmpeg
//               service threads may be needed.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
set_thread_priority(ThreadPriority thread_priority) {
  if (_thread_priority != thread_priority) {
    _thread_priority = thread_priority;
    if (is_thread_started()) {
      stop_thread();
      start_thread();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::get_thread_priority
//       Access: Published
//  Description: Returns the current thread priority of the thread that
//               decodes the ffmpeg video stream (if
//               max_readahead_frames is nonzero).  See
//               set_thread_priority().
////////////////////////////////////////////////////////////////////
ThreadPriority FfmpegVideoCursor::
get_thread_priority() const {
  return _thread_priority;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::start_thread
//       Access: Published
//  Description: Explicitly starts the ffmpeg decoding thread after it
//               has been stopped by a call to stop_thread().  The
//               thread is normally started automatically, so there is
//               no need to call this method unless you have
//               previously called stop_thread() for some reason.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
start_thread() {
  MutexHolder holder(_lock);

  if (_thread_status == TS_stopped && _max_readahead_frames > 0) {
    // Get a unique name for the thread's sync name.
    ostringstream strm;
    strm << (void *)this;
    _sync_name = strm.str();

    // Create and start the thread object.
    _thread_status = TS_wait;
    _thread = new GenericThread(_filename.get_basename(), _sync_name, st_thread_main, this);
    if (!_thread->start(_thread_priority, true)) {
      // Couldn't start the thread.
      _thread = NULL;
      _thread_status = TS_stopped;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::stop_thread
//       Access: Published
//  Description: Explicitly stops the ffmpeg decoding thread.  There
//               is normally no reason to do this unless you want to
//               maintain precise control over what threads are
//               consuming CPU resources.  Calling this method will
//               make the video update in the main thread, regardless
//               of the setting of max_readahead_frames, until you
//               call start_thread() again.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
stop_thread() {
  if (_thread_status != TS_stopped) {
    PT(GenericThread) thread = _thread;
    {
      MutexHolder holder(_lock);
      if (_thread_status != TS_stopped) {
        _thread_status = TS_shutdown;
      }
      _action_cvar.notify();
      _thread = NULL;
    }

    // Now that we've released the lock, we can join the thread.
    thread->join();
  }

  // This is a good time to clean up all of the allocated frame
  // objects.  It's not really necessary to be holding the lock, since
  // the thread is gone, but we'll grab it anyway just in case someone
  // else starts the thread up again.
  MutexHolder holder(_lock);

  _readahead_frames.clear();
  _recycled_frames.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::is_thread_started
//       Access: Published
//  Description: Returns true if the thread has been started, false if
//               not.  This will always return false if
//               max_readahead_frames is 0.
////////////////////////////////////////////////////////////////////
bool FfmpegVideoCursor::
is_thread_started() const {
  return (_thread_status != TS_stopped);
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::set_time
//       Access: Published, Virtual
//  Description: See MovieVideoCursor::set_time().
////////////////////////////////////////////////////////////////////
bool FfmpegVideoCursor::
set_time(double time, int loop_count) {
  int frame = (int)(time / _video_timebase + 0.5);

  if (_eof_known) {
    if (loop_count == 0) {
      frame = frame % _eof_frame;
    } else {
      int last_frame = _eof_frame * loop_count;
      if (frame < last_frame) {
        frame = frame % _eof_frame;
      } else {
        frame = _eof_frame - 1;
      }
    }
  }

  if (ffmpeg_cat.is_spam() /* && frame != _current_frame*/) {
    ffmpeg_cat.spam()
      << "set_time(" << time << "): " << frame << ", loop_count = " << loop_count << "\n";
  }

  _current_frame = frame;
  if (_current_frame_buffer != NULL) {
    // If we've previously returned a frame, don't bother asking for a
    // next one if that frame is still valid.
    return (_current_frame >= _current_frame_buffer->_end_frame || 
            _current_frame < _current_frame_buffer->_begin_frame);
  }

  // If our last request didn't return a frame, try again.
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::fetch_buffer
//       Access: Public, Virtual
//  Description: See MovieVideoCursor::fetch_buffer.
////////////////////////////////////////////////////////////////////
PT(MovieVideoCursor::Buffer) FfmpegVideoCursor::
fetch_buffer() {
  MutexHolder holder(_lock);
  
  // If there was an error at any point, just return NULL.
  if (_format_ctx == (AVFormatContext *)NULL) {
    return NULL;
  }

  PT(FfmpegBuffer) frame;
  if (_thread_status == TS_stopped) {
    // Non-threaded case.  Just get the next frame directly.
    advance_to_frame(_current_frame);
    if (_frame_ready) {
      frame = do_alloc_frame();
      export_frame(frame);
    }

  } else {
    // Threaded case.  Wait for the thread to serve up the required
    // frames.
    if (!_readahead_frames.empty()) {
      frame = _readahead_frames.front();
      _readahead_frames.pop_front();
      _action_cvar.notify();
      while (frame->_end_frame < _current_frame && !_readahead_frames.empty()) {
        // This frame is too old.  Discard it.
        if (ffmpeg_cat.is_debug()) {
          ffmpeg_cat.debug()
            << "ffmpeg for " << _filename.get_basename()
            << " at frame " << _current_frame << ", discarding frame at "
            << frame->_begin_frame << "\n";
        }
        do_recycle_frame(frame);
        frame = _readahead_frames.front();
        _readahead_frames.pop_front();
      }
      if (frame->_begin_frame > _current_frame) {
        // This frame is too new.  Empty all remaining frames and seek
        // backwards.
        if (ffmpeg_cat.is_debug()) {
          ffmpeg_cat.debug()
            << "ffmpeg for " << _filename.get_basename()
            << " at frame " << _current_frame << ", encountered too-new frame at "
            << frame->_begin_frame << "\n";
        }
        do_recycle_all_frames();
        if (_thread_status == TS_wait || _thread_status == TS_seek || _thread_status == TS_readahead) {
          _thread_status = TS_seek;
          _seek_frame = _current_frame;
          _action_cvar.notify();
        }
      }
    }
    if (frame == NULL || frame->_end_frame < _current_frame) {
      // No frame available, or the frame is too old.  Seek.
      if (_thread_status == TS_wait || _thread_status == TS_seek || _thread_status == TS_readahead) {
        _thread_status = TS_seek;
        _seek_frame = _current_frame;
        _action_cvar.notify();
      }
    }
  }

  if (frame != NULL && (frame->_end_frame < _current_frame || frame->_begin_frame > _current_frame)) {
    // The frame is too old or too new.  Just recycle it.
    do_recycle_frame(frame);
    frame = NULL;
  }

  if (frame != NULL) {
    if (_current_frame_buffer != NULL) {
      do_recycle_frame(_current_frame_buffer);
    }
    _current_frame_buffer = frame;
    if (ffmpeg_cat.is_debug()) {
      ffmpeg_cat.debug()
        << "ffmpeg for " << _filename.get_basename()
        << " at frame " << _current_frame << ", returning frame at "
        << frame->_begin_frame << "\n";
    }
  } else {
    if (ffmpeg_cat.is_debug()) {
      ffmpeg_cat.debug()
        << "ffmpeg for " << _filename.get_basename()
        << " at frame " << _current_frame << ", returning NULL\n";
    }
  }
  return frame.p();
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::release_buffer
//       Access: Public, Virtual
//  Description: Should be called after processing the Buffer object
//               returned by fetch_buffer(), this releases the Buffer
//               for future use again.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
release_buffer(Buffer *buffer) {
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::make_new_buffer
//       Access: Protected, Virtual
//  Description: May be called by a derived class to allocate a new
//               Buffer object.
////////////////////////////////////////////////////////////////////
PT(MovieVideoCursor::Buffer) FfmpegVideoCursor::
make_new_buffer() {
  PT(FfmpegBuffer) frame = new FfmpegBuffer(size_x() * size_y() * get_num_components());
  return frame.p();
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::open_stream
//       Access: Private
//  Description: Opens the stream for the first time, or when needed
//               internally.
////////////////////////////////////////////////////////////////////
bool FfmpegVideoCursor::
open_stream() {
  nassertr(!_ffvfile.is_open(), false);

  // Hold the global lock while we open the file and create avcodec
  // objects.
  ReMutexHolder av_holder(_av_lock);

  if (!_source->get_subfile_info().is_empty()) {
    // Read a subfile.
    if (!_ffvfile.open_subfile(_source->get_subfile_info())) {
      ffmpeg_cat.info() 
        << "Couldn't open " << _source->get_subfile_info() << "\n";
      close_stream();
      return false;
    }

  } else {
    // Read a filename.
    if (!_ffvfile.open_vfs(_filename)) {
      ffmpeg_cat.info() 
        << "Couldn't open " << _filename << "\n";
      close_stream();
      return false;
    }
  }

  nassertr(_format_ctx == NULL, false);
  _format_ctx = _ffvfile.get_format_context();
  nassertr(_format_ctx != NULL, false);

  if (av_find_stream_info(_format_ctx) < 0) {
    ffmpeg_cat.info() 
      << "Couldn't find stream info\n";
    close_stream();
    return false;
  }
  
  // Find the video stream
  nassertr(_video_ctx == NULL, false);
  for (int i = 0; i < (int)_format_ctx->nb_streams; ++i) {
    if (_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      _video_index = i;
      _video_ctx = _format_ctx->streams[i]->codec;
      _video_timebase = av_q2d(_format_ctx->streams[i]->time_base);
      _min_fseek = (int)(3.0 / _video_timebase);
    }
  }
  
  if (_video_ctx == NULL) {
    ffmpeg_cat.info() 
      << "Couldn't find video_ctx\n";
    close_stream();
    return false;
  }

  AVCodec *pVideoCodec = avcodec_find_decoder(_video_ctx->codec_id);
  if (pVideoCodec == NULL) {
    ffmpeg_cat.info() 
      << "Couldn't find codec\n";
    close_stream();
    return false;
  }
  if (avcodec_open(_video_ctx, pVideoCodec) < 0) {
    ffmpeg_cat.info() 
      << "Couldn't open codec\n";
    close_stream();
    return false;
  }
  
  _size_x = _video_ctx->width;
  _size_y = _video_ctx->height;
  _num_components = 3; // Don't know how to implement RGBA movies yet.
  _length = (double)_format_ctx->duration / (double)AV_TIME_BASE;
  _can_seek = true;
  _can_seek_fast = true;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::close_stream
//       Access: Private
//  Description: Closes the stream, during cleanup or when needed
//               internally.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
close_stream() {
  // Hold the global lock while we free avcodec objects.
  ReMutexHolder av_holder(_av_lock);
  
  if ((_video_ctx)&&(_video_ctx->codec)) {
    avcodec_close(_video_ctx);
  }
  _video_ctx = NULL;
  
  _ffvfile.close();
  _format_ctx = NULL;

  _video_index = -1;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::cleanup
//       Access: Private
//  Description: Reset to a standard inactive state.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
cleanup() {
  stop_thread();
  close_stream();

  ReMutexHolder av_holder(_av_lock);

#ifdef HAVE_SWSCALE
  if (_convert_ctx != NULL) {
    sws_freeContext(_convert_ctx);
  }
  _convert_ctx = NULL;
#endif  // HAVE_SWSCALE

  if (_frame) {
    av_free(_frame);
    _frame = NULL;
  }

  if (_frame_out) {
    _frame_out->data[0] = 0;
    av_free(_frame_out);
    _frame_out = NULL;
  }

  if (_packet0) {
    if (_packet0->data) {
      av_free_packet(_packet0);
    }
    delete _packet0;
    _packet0 = NULL;
  }

  if (_packet1) {
    if (_packet1->data) {
      av_free_packet(_packet1);
    }
    delete _packet1;
    _packet1 = NULL;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::st_thread_main
//       Access: Private, Static
//  Description: The thread main function, static version (for passing
//               to GenericThread).
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
st_thread_main(void *self) {
  ((FfmpegVideoCursor *)self)->thread_main();
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::thread_main
//       Access: Private
//  Description: The thread main function.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
thread_main() {
  MutexHolder holder(_lock);
  if (ffmpeg_cat.is_spam()) {
    ffmpeg_cat.spam()
      << "ffmpeg thread for " << _filename.get_basename() << " starting.\n";
  }
  
  // Repeatedly wait for something interesting to do, until we're told
  // to shut down.
  while (_thread_status != TS_shutdown) {
    nassertv(_thread_status != TS_stopped);
    _action_cvar.wait();

    while (do_poll()) {
      // Keep doing stuff as long as there's something to do.
      PStatClient::thread_tick(_sync_name);
      Thread::consider_yield();
    }
  }

  _thread_status = TS_stopped;
  if (ffmpeg_cat.is_spam()) {
    ffmpeg_cat.spam()
      << "ffmpeg thread for " << _filename.get_basename() << " stopped.\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::do_poll
//       Access: Private
//  Description: Called within the sub-thread.  Assumes the lock is
//               already held.  If there is something for the thread
//               to do, does it and returns true.  If there is nothing
//               for the thread to do, returns false.
////////////////////////////////////////////////////////////////////
bool FfmpegVideoCursor::
do_poll() {
  switch (_thread_status) {
  case TS_stopped:
  case TS_seeking:
    // This shouldn't be possible while the thread is running.
    nassertr(false, false);
    return false;
    
  case TS_wait:
    // The video hasn't started playing yet.
    return false;

  case TS_readahead:
    if ((int)_readahead_frames.size() < _max_readahead_frames) {
      // Time to read the next frame.
      PT(FfmpegBuffer) frame = do_alloc_frame();
      nassertr(frame != NULL, false);
      _lock.release();
      fetch_frame(-1);
      if (_frame_ready) {
        export_frame(frame);
        _lock.acquire();
        _readahead_frames.push_back(frame);
      } else {
        // No frame.
        _lock.acquire();
        do_recycle_frame(frame);
      }
      return true;
    }

    // No room for the next frame yet.  Wait for more.
    return false;

  case TS_seek:
    // Seek to a specific frame.
    {
      int seek_frame = _seek_frame;
      _thread_status = TS_seeking;
      PT(FfmpegBuffer) frame = do_alloc_frame();
      nassertr(frame != NULL, false);
      _lock.release();
      advance_to_frame(seek_frame);
      if (_frame_ready) {
        export_frame(frame);
        _lock.acquire();
        do_recycle_all_frames();
        _readahead_frames.push_back(frame);
      } else {
        _lock.acquire();
        do_recycle_all_frames();
        do_recycle_frame(frame);
      }

      if (_thread_status == TS_seeking) {
        // After seeking, we automatically transition to readahead.
        _thread_status = TS_readahead;
      }
    }
    return true;

  case TS_shutdown:
    // Time to stop the thread.
    return false;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::do_alloc_frame
//       Access: Private
//  Description: Allocates a new Buffer object, or returns a
//               previously-recycled object.  Assumes the lock is
//               held.
////////////////////////////////////////////////////////////////////
PT(FfmpegVideoCursor::FfmpegBuffer) FfmpegVideoCursor::
do_alloc_frame() {
  if (!_recycled_frames.empty()) {
    PT(FfmpegBuffer) frame = _recycled_frames.front();
    _recycled_frames.pop_front();
    return frame;
  }
  PT(Buffer) buffer = make_new_buffer();
  return (FfmpegBuffer *)buffer.p();
}
 
////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::do_recycle_frame
//       Access: Private
//  Description: Recycles a previously-allocated Buffer object for
//               future reuse.  Assumes the lock is held.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
do_recycle_frame(FfmpegBuffer *frame) {
  _recycled_frames.push_back(frame);
}
 
////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::do_recycle_all_frames
//       Access: Private
//  Description: Empties the entire readahead_frames queue into the
//               recycle bin.  Assumes the lock is held.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
do_recycle_all_frames() {
  while (!_readahead_frames.empty()) {
    PT(FfmpegBuffer) frame = _readahead_frames.front();
    _readahead_frames.pop_front();
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "ffmpeg for " << _filename.get_basename()
        << " recycling frame at " << frame->_begin_frame << "\n";
    }
    _recycled_frames.push_back(frame);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::fetch_packet
//       Access: Private
//  Description: Called within the sub-thread.  Fetches a video packet
//               and stores it in the packet0 buffer.  Sets packet_frame
//               to the packet's timestamp.  If a packet could not be
//               read, the packet is cleared and the packet_frame is
//               set to the specified default value.  Returns true on
//               failure (such as the end of the video), or false on
//               success.
////////////////////////////////////////////////////////////////////
bool FfmpegVideoCursor::
fetch_packet(int default_frame) {
  if (_packet0->data) {
    av_free_packet(_packet0);
  }
  while (av_read_frame(_format_ctx, _packet0) >= 0) {
    if (_packet0->stream_index == _video_index) {
      _packet_frame = _packet0->dts;
      return false;
    }
    av_free_packet(_packet0);
  }
  _packet0->data = 0;

  if (!_eof_known && default_frame != 0) {
    _eof_frame = _packet_frame;
    _eof_known = true;
  }

  if (ffmpeg_cat.is_spam()) {
    if (_eof_known) {
      ffmpeg_cat.spam()
        << "end of video at frame " << _eof_frame << "\n";
    } else {
      ffmpeg_cat.spam()
        << "end of video\n";
    }
  }
  _packet_frame = default_frame;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::flip_packets
//       Access: Private
//  Description: Called within the sub-thread.  Reverses _packet0 and _packet1.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
flip_packets() {
  AVPacket *t = _packet0;
  _packet0 = _packet1;
  _packet1 = t;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::fetch_frame
//       Access: Private
//  Description: Called within the sub-thread.  Slides forward until
//               the indicated frame, then fetches a frame from the
//               stream and stores it in the frame buffer.  Sets
//               _begin_frame and _end_frame to indicate the extents of
//               the frame.  Sets _frame_ready true to indicate a
//               frame is now available, or false if it is not (for
//               instance, because the end of the video was reached).
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
fetch_frame(int frame) {
  static PStatCollector fetch_buffer_pcollector("*:FFMPEG Video Decoding:Fetch");
  PStatTimer timer(fetch_buffer_pcollector);

  int finished = 0;

  if (_packet_frame <= frame) {
    _video_ctx->skip_frame = AVDISCARD_BIDIR;
    // Put the current packet aside in case we discover it's the
    // packet to keep.
    flip_packets();
    
    // Get the next packet.  The first packet beyond the frame we're
    // looking for marks the point to stop.
    _begin_frame = _packet_frame;
    if (fetch_packet(frame)) {
      _end_frame = _packet_frame;
      _frame_ready = false;
      return;
    }
    while (_packet_frame <= frame) {
      static PStatCollector seek_pcollector("*:FFMPEG Video Decoding:Seek");
      PStatTimer timer(seek_pcollector);

      // Decode and discard the previous packet.
#if LIBAVCODEC_VERSION_INT < 3414272
      avcodec_decode_video(_video_ctx, _frame,
                           &finished, _packet1->data, _packet1->size);
#else
      avcodec_decode_video2(_video_ctx, _frame, &finished, _packet1);
#endif
      flip_packets();
      _begin_frame = _packet_frame;
      if (fetch_packet(frame)) {
        _end_frame = _packet_frame;
        _frame_ready = false;
        return;
      }
    }
    _video_ctx->skip_frame = AVDISCARD_DEFAULT;

    // At this point, _packet0 contains the *next* packet to be
    // decoded next frame, and _packet1 contains the packet to decode
    // for this frame.
#if LIBAVCODEC_VERSION_INT < 3414272
    avcodec_decode_video(_video_ctx, _frame,
                         &finished, _packet1->data, _packet1->size);
#else
    avcodec_decode_video2(_video_ctx, _frame, &finished, _packet1);
#endif
    
  } else {
    // Just get the next frame.
    finished = 0;
    while (!finished && _packet0->data) {
#if LIBAVCODEC_VERSION_INT < 3414272
      avcodec_decode_video(_video_ctx, _frame,
                           &finished, _packet0->data, _packet0->size);
#else
      avcodec_decode_video2(_video_ctx, _frame, &finished, _packet0);
#endif
      _begin_frame = _packet_frame;
      fetch_packet(_begin_frame + 1);
    }
  }

  _end_frame = _packet_frame;
  _frame_ready = true;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::seek
//       Access: Private
//  Description: Called within the sub-thread. Seeks to a target
//               location.  Afterward, the packet_frame is guaranteed
//               to be less than or equal to the specified frame.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
seek(int frame, bool backward) {
  static PStatCollector seek_pcollector("*:FFMPEG Video Decoding:Seek");
  PStatTimer timer(seek_pcollector);

  // Protect the call to av_seek_frame() in a global lock, just to be
  // paranoid.
  ReMutexHolder av_holder(_av_lock);

  PN_int64 target_ts = (PN_int64)frame;
  if (target_ts < (PN_int64)(_initial_dts)) {
    // Attempts to seek before the first packet will fail.
    target_ts = _initial_dts;
  }
  int flags = 0;
  if (backward) {
    flags = AVSEEK_FLAG_BACKWARD;
    //reset_stream();
  }

  if (av_seek_frame(_format_ctx, _video_index, target_ts, flags) < 0) {
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "Seek failure.\n";
    }

    if (backward) {
      // Now try to seek forward.
      reset_stream();
      return seek(frame, false);
    }

    // Try a binary search to get a little closer.
    if (binary_seek(_initial_dts, frame, frame, 1) < 0) {
      if (ffmpeg_cat.is_spam()) {
        ffmpeg_cat.spam()
          << "Seek double failure.\n";
      }
      reset_stream();
    }
  }

  {
    // Close and re-open the codec (presumably to flush the queue).
    // Actually, this causes the stream to fail in certain video
    // files, and doesn't seem to have any useful benefit.  So screw
    // it, and don't do this.

    /*
    avcodec_close(_video_ctx);
    AVCodec *pVideoCodec = avcodec_find_decoder(_video_ctx->codec_id);
    if (pVideoCodec == 0) {
      cleanup();
      return;
    }

    if (avcodec_open(_video_ctx, pVideoCodec)<0) {
      cleanup();
      return;
    }
    */
  }

  fetch_packet(0);
  fetch_frame(-1);
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::binary_seek
//       Access: Private
//  Description: Casts about within the stream for a reasonably-close
//               frame to seek to.  We're trying to get as close as
//               possible to target_frame.
////////////////////////////////////////////////////////////////////
int FfmpegVideoCursor::
binary_seek(int min_frame, int max_frame, int target_frame, int num_iterations) {
  int try_frame = (min_frame + max_frame) / 2;
  if (num_iterations > 5 || try_frame >= max_frame) {
    // Success.
    return 0; 
  }

  if (av_seek_frame(_format_ctx, _video_index, try_frame, AVSEEK_FLAG_BACKWARD) < 0) {
    // Failure.  Try lower.
    if (binary_seek(min_frame, try_frame - 1, target_frame, num_iterations + 1) < 0) {
      return -1;
    }
  } else {
    // Success.  Try higher.
    if (binary_seek(try_frame + 1, max_frame, target_frame, num_iterations + 1) < 0) {
      return -1;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::reset_stream
//       Access: Private
//  Description: Resets the stream to its initial, first-opened state
//               by closing and re-opening it.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
reset_stream() {
  if (ffmpeg_cat.is_spam()) {
    ffmpeg_cat.spam()
      << "Resetting ffmpeg stream.\n";
  }

  close_stream();
  if (!open_stream()) {
    ffmpeg_cat.error()
      << "Stream error, invalidating movie.\n";
    cleanup();
    return;
  }

  fetch_packet(0);
  _initial_dts = _packet0->dts;
  fetch_frame(-1);
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::advance_to_frame
//       Access: Private 
//  Description: Called within the sub-thread.  Advance until the
//               specified frame is in the export buffer.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
advance_to_frame(int frame) {
  static PStatCollector fetch_buffer_pcollector("*:FFMPEG Video Decoding:Fetch");
  PStatTimer timer(fetch_buffer_pcollector);

  if (frame < _begin_frame) {
    // Frame is in the past.
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "Seeking backward to " << frame << " from " << _begin_frame << "\n";
    }
    seek(frame, true);
    if (_begin_frame > frame) {
      if (ffmpeg_cat.is_spam()) {
        ffmpeg_cat.spam()
          << "Ended up at " << _begin_frame << ", not far enough back!\n";
      }
      reset_stream();
      if (ffmpeg_cat.is_spam()) {
        ffmpeg_cat.spam()
          << "Reseek to 0, got " << _begin_frame << "\n";
      }
    }
    if (frame > _end_frame) {
      if (ffmpeg_cat.is_spam()) {
        ffmpeg_cat.spam()
          << "Now sliding forward to " << frame << " from " << _begin_frame << "\n";
      }
      fetch_frame(frame);
    }

  } else if (frame < _end_frame) {
    // Frame is in the present: already have the frame.
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "Currently have " << frame << " within " << _begin_frame << " .. " << _end_frame << "\n";
    }

  } else if (frame < _end_frame + _min_fseek) {
    // Frame is in the near future.
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "Sliding forward to " << frame << " from " << _begin_frame << "\n";
    }
    fetch_frame(frame);

  } else {
    // Frame is in the far future.  Seek forward, then read.
    // There's a danger here: because keyframes are spaced
    // unpredictably, trying to seek forward could actually
    // move us backward in the stream!  This must be avoided.
    // So the rule is, try the seek.  If it hurts us by moving
    // us backward, we increase the minimum threshold distance
    // for forward-seeking in the future.
    
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "Jumping forward to " << frame << " from " << _begin_frame << "\n";
    }
    int base = _begin_frame;
    seek(frame, false);
    if (_begin_frame < base) {
      _min_fseek += (base - _begin_frame);
      if (ffmpeg_cat.is_spam()) {
        ffmpeg_cat.spam()
          << "Wrong way!  Increasing _min_fseek to " << _min_fseek << "\n";
      }
    }
    if (frame > _end_frame) {
      if (ffmpeg_cat.is_spam()) {
        ffmpeg_cat.spam()
          << "Correcting, sliding forward to " << frame << " from " << _begin_frame << "\n";
      }
      fetch_frame(frame);
    }
  }

  if (ffmpeg_cat.is_spam()) {
    ffmpeg_cat.spam()
      << "Wanted " << frame << ", got " << _begin_frame << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::export_frame
//       Access: Private
//  Description: Called within the sub-thread.  Exports the contents
//               of the frame buffer into the indicated target buffer.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
export_frame(FfmpegBuffer *buffer) {
  static PStatCollector export_frame_collector("*:FFMPEG Convert Video to BGR");
  PStatTimer timer(export_frame_collector);

  if (!_frame_ready) {
    // No frame data ready, just fill with black.
    if (ffmpeg_cat.is_spam()) {
      ffmpeg_cat.spam()
        << "ffmpeg for " << _filename.get_basename()
        << ", no frame available.\n";
    }
    memset(buffer->_block, 0, buffer->_block_size);
    return;
  }

  _frame_out->data[0] = buffer->_block + ((_size_y - 1) * _size_x * 3);
  _frame_out->linesize[0] = _size_x * -3;
  buffer->_begin_frame = _begin_frame;
  buffer->_end_frame = _end_frame;
#ifdef HAVE_SWSCALE
  nassertv(_convert_ctx != NULL && _frame != NULL && _frame_out != NULL);
  sws_scale(_convert_ctx, _frame->data, _frame->linesize, 0, _size_y, _frame_out->data, _frame_out->linesize);
#else
  img_convert((AVPicture *)_frame_out, PIX_FMT_BGR24, 
              (AVPicture *)_frame, _video_ctx->pix_fmt, _size_x, _size_y);
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               FfmpegVideo.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
write_datagram(BamWriter *manager, Datagram &dg) {
  MovieVideoCursor::write_datagram(manager, dg);

  // No need to write any additional data here--all of it comes
  // implicitly from the underlying MovieVideo, which we process in
  // finalize().
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::finalize
//       Access: Public, Virtual
//  Description: Called by the BamReader to perform any final actions
//               needed for setting up the object after all objects
//               have been read and all pointers have been completed.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
finalize(BamReader *) {
  if (_source != (MovieVideo *)NULL) {
    FfmpegVideo *video;
    DCAST_INTO_V(video, _source);
    init_from(video);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::make_from_bam
//       Access: Private, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type FfmpegVideo is encountered
//               in the Bam file.  It should create the FfmpegVideo
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *FfmpegVideoCursor::
make_from_bam(const FactoryParams &params) {
  FfmpegVideoCursor *video = new FfmpegVideoCursor;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  video->fillin(scan, manager);

  return video;
}

////////////////////////////////////////////////////////////////////
//     Function: FfmpegVideoCursor::fillin
//       Access: Private
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new FfmpegVideo.
////////////////////////////////////////////////////////////////////
void FfmpegVideoCursor::
fillin(DatagramIterator &scan, BamReader *manager) {
  MovieVideoCursor::fillin(scan, manager);
  
  // The MovieVideoCursor gets the underlying MovieVideo pointer.  We
  // need a finalize callback so we can initialize ourselves once that
  // has been read completely.
  manager->register_finalize(this);
}

#endif // HAVE_FFMPEG
