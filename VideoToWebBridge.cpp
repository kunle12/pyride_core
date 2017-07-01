/*
 *  VideoToWebBridge.cpp
 *  PyRIDE
 *
 *  Created by Xun Wang on 13/06/2017
 *
 */

#include <sys/time.h>

#include <boost/lexical_cast.hpp>
#include "async_web_server_cpp/http_reply.hpp"

#include "VideoToWebBridge.h"

namespace pyride {

using namespace pyride_remote;

VideoToWebBridge * VideoToWebBridge::s_pVideoToWebBridge = NULL;

static bool __verbose = false;
static const int kVideoStreamingGapTolerance = 15;

static bool web_logger( async_web_server_cpp::HttpServerRequestHandler forward,
                        const async_web_server_cpp::HttpRequest &request,
                        async_web_server_cpp::HttpConnectionPtr connection, const char* begin,
                        const char* end )
{
  if (__verbose) {
    INFO_MSG( "Handling Request: %s.\n", request.uri.c_str() );
  }
  try {
    forward( request, connection, begin, end );
    return true;
  }
  catch (std::exception &e) {
    WARNING_MSG( "Error Handling Request: %s.\n", e.what() );
    return false;
  }
  return false;
}

VideoToWebBridge::VideoToWebBridge() :
  port_( 8281 ),
  address_( "0.0.0.0" ),
  server_threads_( 1 ),
  isRunning_( false ),
  dataTS_( -1 ),
  dataStream_( NULL ),
  handler_group_(
          async_web_server_cpp::HttpReply::stock_reply( async_web_server_cpp::HttpReply::not_found ) )
{
  handler_group_.addHandlerForPath("/", boost::bind(&VideoToWebBridge::handle_list_streams, this, _1, _2, _3, _4));
  handler_group_.addHandlerForPath("/stream", boost::bind(&VideoToWebBridge::handle_stream, this, _1, _2, _3, _4));
  //handler_group_.addHandlerForPath("/stream_viewer",
  //                                 boost::bind(&VideoToWebBridge::handle_stream_viewer, this, _1, _2, _3, _4));
  //handler_group_.addHandlerForPath("/snapshot", boost::bind(&VideoToWebBridge::handle_snapshot, this, _1, _2, _3, _4));
  pyExtension_ = NULL;
  streaming_data_thread_ = NULL;
}

VideoToWebBridge::~VideoToWebBridge()
{
  this->stop();
}

VideoToWebBridge * VideoToWebBridge::instance()
{
  if (!s_pVideoToWebBridge) {
    s_pVideoToWebBridge = new VideoToWebBridge();
  }
  return s_pVideoToWebBridge;
}

bool VideoToWebBridge::start()
{
  if (isRunning_) {
    WARNING_MSG( "Video to web service is already running.\n" );
    return false;
  }

  dataStream_ = new RTPDataReceiver();
  dataStream_->init( PYRIDE_VIDEO_STREAM_BASE_PORT - 20, true );

  isRunning_ = true;
  struct timeval now;
  gettimeofday( &now, NULL );
  dataTS_ = now.tv_sec;

  streaming_data_thread_ = new boost::thread( &VideoToWebBridge::grabAndDispatchVideoStreamData, this );

  try {
    server_.reset(
        new async_web_server_cpp::HttpServer(address_, boost::lexical_cast<std::string>(port_),
                                             boost::bind( web_logger, handler_group_, _1, _2, _3, _4),
                                             server_threads_ ));
  }
  catch (boost::exception& e) {
    ERROR_MSG( "Exception when creating the web server! %s:%d.\n", address_.c_str(), port_);
    return false;
  }
  server_->run();

  if (pyExtension_) {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject * arg = Py_BuildValue( "(O)", Py_True );

    pyExtension_->invokeCallback( "onVideoFeedback", arg );

    Py_DECREF( arg );

    PyGILState_Release( gstate );
  }
  return true;
}

void VideoToWebBridge::stop( bool selfterminate )
{
  if (!isRunning_)
    return;

  isRunning_ = false;

  server_->stop();

  if (streaming_data_thread_) {
    if (!selfterminate)
      streaming_data_thread_->join();
    delete streaming_data_thread_;
    streaming_data_thread_ = NULL;
  }

  if (dataStream_) {
    delete dataStream_;
    dataStream_ = NULL;
  }
  image_subscribers_.clear();

  server_.reset();

  if (pyExtension_) {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject * arg = Py_BuildValue( "(O)", Py_False );

    pyExtension_->invokeCallback( "onVideoFeedback", arg );

    Py_DECREF( arg );

    PyGILState_Release( gstate );
  }
}

bool VideoToWebBridge::handle_stream( const async_web_server_cpp::HttpRequest &request,
                                   async_web_server_cpp::HttpConnectionPtr connection, const char* begin,
                                   const char* end )
{
  std::string type = request.get_query_param_value_or_default( "type", "mjpeg" );
  if (type.compare( "mjpeg" ) == 0) {
    boost::mutex::scoped_lock lock( subscriber_mutex_ );
    image_subscribers_.push_back( boost::shared_ptr<JpegImageStreamer>(new JpegImageStreamer( request, connection )) );
  }
  else {
    async_web_server_cpp::HttpReply::stock_reply(async_web_server_cpp::HttpReply::not_found)( request, connection, begin,
                                                                                             end );
  }
  return true;
}

bool VideoToWebBridge::handle_snapshot( const async_web_server_cpp::HttpRequest &request,
                                     async_web_server_cpp::HttpConnectionPtr connection, const char* begin,
                                     const char* end )
{
  /*
  boost::shared_ptr<ImageStreamer> streamer(new JpegSnapshotStreamer(request, connection, nh_));
  streamer->start();
  */
  return true;
}

bool VideoToWebBridge::handle_stream_viewer( const async_web_server_cpp::HttpRequest &request,
                                          async_web_server_cpp::HttpConnectionPtr connection, const char* begin,
                                          const char* end )
{
  /*
  std::string type = request.get_query_param_value_or_default("type", "mjpeg");
  if (type.compare( "mjpeg") == 0) {
    async_web_server_cpp::HttpReply::builder(async_web_server_cpp::HttpReply::ok).header("Connection", "close").header(
        "Server", "pyride_server").header("Content-type", "text/html;").write(connection);

    std::stringstream ss;
    ss << "<html><head><title>" << topic << "</title></head><body>";
    ss << "<h1>" << topic << "</h1>";
    ss << stream_types_[type]->create_viewer(request);
    ss << "</body></html>";
    connection->write(ss.str());
  }
  else {
    async_web_server_cpp::HttpReply::stock_reply(async_web_server_cpp::HttpReply::not_found)(request, connection, begin,
                                                                                             end);
  }
  */
  return true;
}

bool VideoToWebBridge::handle_list_streams(const async_web_server_cpp::HttpRequest &request,
                                         async_web_server_cpp::HttpConnectionPtr connection, const char* begin,
                                         const char* end)
{

  connection->write("<html>"
                    "<head><title>PyRIDE Video Feedback</title></head>"
                    "<body><h1>Remote user is currently streaming video images</h1>");
  connection->write("<a href=\"/stream?topic=mjpeg\">Click here to view image</a>");
  connection->write("</body></html>");
  return true;
}

void VideoToWebBridge::grabAndDispatchVideoStreamData()
{
  unsigned char * rawData = NULL;
  unsigned char * data = NULL;
  int dataSize = 0, rawDataSize = 0;
  bool dataSizeChanged = false;
  bool lostConnection = false;

  if (!dataStream_)
    return;

  while (isRunning_) {
    int gcount = 0;

    do {
      rawDataSize = dataStream_->grabData( &rawData, dataSizeChanged );

      data = rawData;
      dataSize = rawDataSize;
      gcount++;
      usleep( 1000 ); // 1ms
    } while (dataSize == 0 && gcount < 100);

    struct timeval now;
    gettimeofday( &now, NULL );

    if (dataSize == 0) {
      if ((now.tv_sec - dataTS_) > kVideoStreamingGapTolerance) {
        ERROR_MSG( "Missing video stream for %d second. Assuming the connection is dead. Stop!\n",
            kVideoStreamingGapTolerance );
        lostConnection = true;
        break;
      }
      else {
        continue;
      }
    }
    dataTS_ = now.tv_sec;

    double ts = double(now.tv_sec) + (double(now.tv_usec) / 1000000.0);
    //DEBUG_MSG( "Got video data %d %lu %lu.\n", dataSize, now.tv_sec, now.tv_usec );
    std::vector<unsigned char> dispatchdata( data, data+dataSize );
    {
      boost::mutex::scoped_lock lock( subscriber_mutex_, boost::try_to_lock );
      if (lock) {
        typedef std::vector<boost::shared_ptr<JpegImageStreamer> >::iterator itr_type;
        //itr_type new_end = std::remove_if(image_subscribers_.begin(), image_subscribers_.end(),
        //                                  boost::bind(&ImageStreamer::isInactive, _1));
        for (itr_type iter = image_subscribers_.begin(); iter != image_subscribers_.end();) {
          bool inerror = false;
          try {
            if (dispatchdata.size() > 0) { // don't know why sometime it send zero size.
              //DEBUG_MSG( "Sending data to chip monitor %d %.6lf.\n", (int)dispatchdata.size(), ts );
              (*iter)->sendImage( ts, dispatchdata );
            }
          }
          catch (boost::system::system_error &e) {
            // happens when client disconnects
            INFO_MSG( "client disconnect: %s.\n", e.what() );
            inerror = true;
          }
          catch (std::exception &e) {
            ERROR_MSG( "send image exception: %s.\n", e.what() );
            inerror = true;
          }
          catch (...) {
            ERROR_MSG( "send image general exception.\n" );
            inerror = true;
          }
          if (inerror) {
            iter = image_subscribers_.erase( iter );
          }
          else {
            ++iter;
          }
        }
      }
    }
  }
  if (lostConnection) {
    this->stop( true );
  }
}

JpegImageStreamer::JpegImageStreamer( const async_web_server_cpp::HttpRequest &request,
                             async_web_server_cpp::HttpConnectionPtr connection ) :
  stream_( connection )
{
  stream_.sendInitialHeader();
}

void JpegImageStreamer::sendImage( const double time, std::vector<unsigned char> & data )
{
  stream_.sendPartAndClear( time, "image/jpeg", data );
}

MultipartStream::MultipartStream( async_web_server_cpp::HttpConnectionPtr& connection, const std::string & boundry )
  : connection_(connection), boundry_(boundry) {}

void MultipartStream::sendInitialHeader() {
  async_web_server_cpp::HttpReply::builder(async_web_server_cpp::HttpReply::ok).header("Connection", "close").header(
      "Server", "pyride_server").header("Cache-Control",
                                           "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0").header(
      "Pragma", "no-cache").header("Content-type", "multipart/x-mixed-replace;boundary="+boundry_).header(
      "Access-Control-Allow-Origin", "*").write(connection_);
  connection_->write("--"+boundry_+"\r\n");
}

void MultipartStream::sendPartHeader( const double time, const std::string & type, size_t payload_size) {
  char stamp[20];
  sprintf( stamp, "%.06lf", time );
  boost::shared_ptr<std::vector<async_web_server_cpp::HttpHeader> > headers(
      new std::vector<async_web_server_cpp::HttpHeader>());

  headers->push_back( async_web_server_cpp::HttpHeader("Content-type", type ) );
  headers->push_back( async_web_server_cpp::HttpHeader("X-Timestamp", stamp ) );
  headers->push_back(
      async_web_server_cpp::HttpHeader("Content-Length", boost::lexical_cast<std::string>(payload_size)));
  connection_->write(async_web_server_cpp::HttpReply::to_buffers(*headers), headers);
}

void MultipartStream::sendPartFooter() {
  connection_->write("\r\n--"+boundry_+"\r\n");
}

void MultipartStream::sendPartAndClear(const double time, const std::string& type,
               std::vector<unsigned char> &data) {
  sendPartHeader(time, type, data.size());
  connection_->write_and_clear(data);
  sendPartFooter();
}

void MultipartStream::sendPart(const double time, const std::string& type,
             const boost::asio::const_buffer &buffer,
             async_web_server_cpp::HttpConnection::ResourcePtr resource) {
  sendPartHeader(time, type, boost::asio::buffer_size(buffer));
  connection_->write(buffer, resource);
  sendPartFooter();
}

} // namespace pyride
