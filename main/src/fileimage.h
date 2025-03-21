//* ******************************************************************************************************
//*    @brief filefont header file
//*    @copyright Copyright (c) 2025 Electrolux Professional
//*    @file filefont.h
//********************************************************************************************************

#ifndef CPRO2_COMMON_PLATFORM_GUI_IMAGES_H_
#define CPRO2_COMMON_PLATFORM_GUI_IMAGES_H_

// Included files
#include "file_monitor.h"
#include "lvgl.h"

// Included system files
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#if defined(SIMULATOR)
#include <iostream>
#endif

// *******************************************************************************************************
// Public macro declarations
// *******************************************************************************************************

// *******************************************************************************************************
// Public type declarations
// *******************************************************************************************************

// *******************************************************************************************************
// Public attributes declarations (should be avoided)
// *******************************************************************************************************

// *******************************************************************************************************
// Public functions declarations
// *******************************************************************************************************

using IMonitoredFile = els::cpro2::common::platform::file_utils::file_monitor::IMonitoredFile;

namespace els::cpro2::common::platform::gui::images
{
   class FileImage
   {
   public:
      FileImage();
      ~FileImage();
      bool load(const char *filename);
      lv_img_dsc_t* get() { return &img_; }
   private:
      lv_img_dsc_t img_;
   };

}  // namespace els::cpro2::common::platform::gui::images

#endif  // CPRO2_COMMON_PLATFORM_GUI_IMAGES_H_
