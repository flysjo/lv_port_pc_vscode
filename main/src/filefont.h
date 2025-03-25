//* ******************************************************************************************************
//*    @brief filefont header file
//*    @copyright Copyright (c) 2025 Electrolux Professional
//*    @file filefont.h
//********************************************************************************************************

#ifndef CPRO2_COMMON_PLATFORM_GUI_FILEFONT_H_
#define CPRO2_COMMON_PLATFORM_GUI_FILEFONT_H_

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

namespace els::cpro2::common::platform::gui::fonts
{
   class IFileFont
   {
   protected:
      IFileFont() = default;

   public:
      virtual ~IFileFont() = default;
      virtual bool Load(const char *fontPath) = 0;
      virtual lv_font_t *Get() = 0;
      virtual size_t NumChars() const = 0;
      virtual size_t SizeOf() const = 0;
      /* callback from lvgl font engine */
      virtual bool GetGlyphDsc(lv_font_glyph_dsc_t *dsc_out, uint32_t unicode, uint32_t unicode_next) = 0;
      virtual const uint8_t *GetGlyphBitmap(uint32_t unicode_letter) = 0;
   };

   std::shared_ptr<IFileFont> CreateFileFontObject();

}  // namespace els::cpro2::common::platform::gui::fonts

#endif  // CPRO2_COMMON_PLATFORM_GUI_FILEFONT_H_
