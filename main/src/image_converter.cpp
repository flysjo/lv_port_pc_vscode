//* ******************************************************************************************************
//*    @brief image converter implementation file
//*    @copyright Copyright (c) 2025 Electrolux Professional
//*    @file filefont.cpp
//********************************************************************************************************

// Included files
#include "image_converter.h"
#include "lvgl/src/misc/lv_color.h"

#define LODEPNG_NO_COMPILE_CPP
extern "C"
{
#include "lvgl/src/extra/libs/png/lodepng.h"
}

// Included system files
#include <string.h>

using namespace els::cpro2::common::platform::file_utils;
using namespace els::cpro2::common::platform;

// *******************************************************************************************************
// Private macro definitions
// *******************************************************************************************************

// *******************************************************************************************************
// Private type definitions
// *******************************************************************************************************

// *******************************************************************************************************
// Private attributes definitions
// *******************************************************************************************************

// *******************************************************************************************************
// Private functions definitions
// *******************************************************************************************************

// *******************************************************************************************************
// Public functions definitions
// *******************************************************************************************************

namespace els::cpro2::common::platform::gui::images
{
   typedef struct
   {
      uint16_t rgb565;  // RGB565
      uint8_t alpha;    // A8
   } RGB565A8;

   bool ConvertPngToBin(const char *src_filename, const char *filename)
   {
      file_monitor::MonitoredFile inFile;
      if (inFile.Open(src_filename, FA_READ) != FR_OK)
      {
         LV_LOG_ERROR("input file could not be opened: %s", src_filename);
         return false;
      }
      /* filesize */
      size_t fileSize = inFile.GetSize();
      if (fileSize == 0)
      {
         LV_LOG_ERROR("invalid file size");
         return false;
      }
      std::vector<uint8_t> buffer;
      buffer.resize(fileSize);
      size_t readSize = 0;
      inFile.Read(buffer.data(), fileSize, readSize);
      inFile.Close();
      uint8_t *imageBuffer = 0;
      unsigned width = 0, height = 0;

      unsigned result = lodepng_decode32(&imageBuffer, &width, &height, buffer.data(), fileSize);
      if (result != 0)
      {
         LV_LOG_ERROR("%s", lodepng_error_text(result));
         return false;
      }
      size_t pixels = width * height;
      size_t outSize = pixels * 3;  // RGB565A8
      buffer.resize(outSize);
      auto pixelPtr = reinterpret_cast<uint16_t *>(buffer.data());
      auto alphaPtr = reinterpret_cast<uint8_t *>(buffer.data() + pixels * sizeof(uint16_t));
      auto sourcePtr = reinterpret_cast<uint32_t *>(imageBuffer);
      for (size_t i = 0; i < pixels; i++)
      {
         uint8_t r = (*sourcePtr >> 0) & 0xff;
         uint8_t g = (*sourcePtr >> 8) & 0xff;
         uint8_t b = (*sourcePtr >> 16) & 0xff;
         uint8_t a = (*sourcePtr >> 24) & 0xff;
         lv_color_t c = lv_color_make(r, g, b);
         *pixelPtr = c.full;
         *alphaPtr = a;
         pixelPtr++;
         alphaPtr++;
         sourcePtr++;
      }
      lv_mem_free(imageBuffer);

      file_monitor::MonitoredFile outFile;
      if (outFile.Open(filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
      {
         LV_LOG_ERROR("output file could not be opened: %s", filename);
         outFile.Close();
         return false;
      }

      lv_img_header_t header = {
         .cf = LV_IMG_CF_RGB565A8,
         .always_zero = 0,
         .w = width,
         .h = height,
      };

      outFile.Write(&header, sizeof(header), readSize);
      outFile.Write(buffer.data(), outSize, readSize);
      outFile.Close();
      return true;
   }

}  // namespace els::cpro2::common::platform::images
