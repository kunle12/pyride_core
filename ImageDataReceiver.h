/*
 * ImageDataReceiver.h
 *
 *  PyRIDE
 *
 *  Created by Xun Wang on 11/01/2017.
 *  Copyright 2017 Galaxy Network. All rights reserved.
 *
 */

#ifndef IMAGE_DATA_RECEIVER_H_
#define IMAGE_DATA_RECEIVER_H_

#include <stdio.h>
#include <jpeglib.h>
#include <opencv2/imgproc/imgproc.hpp>

#include "RTPDataReceiver.h"

namespace pyride_remote {

class ImageDataReceiver
{
public:
  ImageDataReceiver( int port, int imageWidth, int imageHeight );
  ~ImageDataReceiver();

  cv::Ptr<cv::Mat> grabVideoStreamData();

private:
  RTPDataReceiver * dataStream_;

  cv::Ptr<cv::Mat> imageMat_;

  // image data
  unsigned char * imageData_;
  struct jpeg_decompress_struct cinfo_;
  struct jpeg_error_mgr jerr_;
};

} // namespace pyride_remote

#endif /* IMAGE_DATA_RECEIVER_H_ */
