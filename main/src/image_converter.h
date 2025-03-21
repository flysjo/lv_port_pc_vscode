//* ******************************************************************************************************
//*    @brief filefont header file
//*    @copyright Copyright (c) 2025 Electrolux Professional
//*    @file filefont.h
//********************************************************************************************************

#ifndef CPRO2_COMMON_PLATFORM_PNG_CONV_H_
#define CPRO2_COMMON_PLATFORM_PNG_CONV_H_

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
   /**
    * @brief convert PNG file to binary file
    *
    * @param src_filename
    * @param filename
    * @return true
    * @return false
    */
   bool ConvertPngToBin(const char *src_filename, const char *filename);

}  // namespace els::cpro2::common::platform::gui::images

#endif  // CPRO2_COMMON_PLATFORM_PNG_CONV_H_
