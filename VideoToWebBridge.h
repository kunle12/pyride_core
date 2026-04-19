/*
 *  VideoToWebBridge.h
 *  PyRIDE
 *
 *  Created by Xun Wang on 13/06/2017
 *
 */

#ifndef VIDEO_TO_WEB_BRIDGE_H
#define VIDEO_TO_WEB_BRIDGE_H

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "async_web_server_cpp/http_connection.hpp"
#include "async_web_server_cpp/http_request.hpp"
#include "async_web_server_cpp/http_server.hpp"

#include "PyModuleStub.h"
#include "RTPDataReceiver.h"

namespace pyride {

using namespace pyride_remote;

class MultipartStream {
public:
  MultipartStream(async_web_server_cpp::HttpConnectionPtr &connection,
                  const std::string &boundry = "boundarydonotcross");

  void sendInitialHeader();
  void sendPartHeader(const double time, const std::string &type,
                      size_t payload_size);
  void sendPartFooter();
  void sendPartAndClear(const double time, const std::string &type,
                        std::vector<unsigned char> &data);
  void sendPart(const double time, const std::string &type,
                const boost::asio::const_buffer &buffer,
                async_web_server_cpp::HttpConnection::ResourcePtr resource);

private:
  async_web_server_cpp::HttpConnectionPtr connection_;
  std::string boundry_;
};

class JpegImageStreamer {
public:
  JpegImageStreamer(const async_web_server_cpp::HttpRequest &request,
                    async_web_server_cpp::HttpConnectionPtr connection);

  void sendImage(const double time, std::vector<unsigned char> &data);

private:
  MultipartStream stream_;
};

class VideoToWebBridge {
public:
  static VideoToWebBridge *instance();
  ~VideoToWebBridge();

  bool start();
  void stop(bool selfterminate = false);

  void setPyModuleExtension(PyModuleExtension *extension) {
    pyExtension_ = extension;
  }

  bool handle_stream(const async_web_server_cpp::HttpRequest &request,
                     async_web_server_cpp::HttpConnectionPtr connection,
                     const char *begin, const char *end);

  bool handle_stream_viewer(const async_web_server_cpp::HttpRequest &request,
                            async_web_server_cpp::HttpConnectionPtr connection,
                            const char *begin, const char *end);

  bool handle_snapshot(const async_web_server_cpp::HttpRequest &request,
                       async_web_server_cpp::HttpConnectionPtr connection,
                       const char *begin, const char *end);

  bool handle_list_streams(const async_web_server_cpp::HttpRequest &request,
                           async_web_server_cpp::HttpConnectionPtr connection,
                           const char *begin, const char *end);

private:
  int port_;
  std::string address_;
  int server_threads_;
  bool isRunning_;
  long dataTS_;

  RTPDataReceiver *dataStream_;
  PyModuleExtension *pyExtension_;

  std::thread *streaming_data_thread_;

  std::shared_ptr<async_web_server_cpp::HttpServer> server_;
  async_web_server_cpp::HttpRequestHandlerGroup handler_group_;
  std::vector<std::shared_ptr<JpegImageStreamer>> image_subscribers_;
  std::mutex subscriber_mutex_;

  static VideoToWebBridge *s_pVideoToWebBridge;

  VideoToWebBridge();

  void grabAndDispatchVideoStreamData();
};

} // namespace pyride

#endif /* VIDEO_TO_WEB_BRIDGE_H */
