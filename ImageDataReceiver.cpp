/*
 * ImageDataReceiver.cpp
 *
 * PyRIDE
 *
 *  Created by Xun Wang on 11/01/2017.
 *
 */
#include <unistd.h>
#include "ImageDataReceiver.h"

namespace pyride_remote {

ImageDataReceiver::ImageDataReceiver( int port, int imageWidth, int imageHeight )
{
  imageData_ = new unsigned char[imageWidth*imageHeight*3];

  dataStream_ = new RTPDataReceiver();
  dataStream_->init( port, true );

  cinfo_.err = jpeg_std_error( &jerr_ );
  jpeg_create_decompress( &cinfo_ );
}

ImageDataReceiver::~ImageDataReceiver()
{
  if (imageData_) {
    delete [] imageData_;
    imageData_ = NULL;
    jpeg_destroy_decompress( &cinfo_ );
  }
  if (dataStream_)
    delete dataStream_;
}

ImageDataPtr ImageDataReceiver::grabVideoStreamData()
{
  unsigned char * rawData = NULL;
  unsigned char * data = NULL;
  int dataSize = 0, rawDataSize = 0;
  bool dataSizeChanged = false;

  if (!dataStream_)
    return ImageDataPtr();

  int gcount = 0;

  do {
    rawDataSize = dataStream_->grabData( &rawData, dataSizeChanged );

    data = rawData;
    dataSize = rawDataSize;
    gcount++;
    usleep( 1000 ); // 1ms
  } while (dataSize == 0 && gcount < 100);

  if (dataSize == 0)
    return ImageDataPtr();

  //printf( "Got video data %d.\n", dataSize );

  jpeg_mem_src( &cinfo_, data, dataSize );
  jpeg_read_header( &cinfo_, true );
  jpeg_start_decompress( &cinfo_ );
  int rowStride = cinfo_.output_width * cinfo_.output_components;

  unsigned char *buffer_array[1];

  while (cinfo_.output_scanline < cinfo_.output_height) {
    buffer_array[0] = imageData_ + (cinfo_.output_scanline) * rowStride;

    jpeg_read_scanlines( &cinfo_, buffer_array, 1 );
  }

  jpeg_finish_decompress( &cinfo_ );

  ImageDataPtr mat = new cv::Mat(cinfo_.output_height, cinfo_.output_width, CV_8UC3, imageData_ );
  cv::cvtColor( *mat, *mat, cv::COLOR_RGB2BGR );
  return mat;
}

} // namespace pyride_remote
