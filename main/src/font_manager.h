/**
 * @brief font manager interface
 *
 * @copyright Copyright (c) 2025 Pandema AB, Electrolux Professional
 *
 * @file font_manager.h
 * @author Claes Ivarsson (ci@pandema.com)
 * @date 2025-02-19
 *
 */

#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include "lvgl.h"

#if defined(__cplusplus)
#include <unordered_map>

namespace els::cpro2::common::platform::gui::fonts
{
   enum class FontId : uint8_t
   {
      kSmall,
      kMedium,
      kLarge
   };
   class FontManager
   {
   public:
      FontManager() = default;
      ~FontManager();
      FontManager(const FontManager&) = delete;
      lv_font_t* load(const char* filename, FontId fontI);
      bool unload(FontId fontId);
      const lv_font_t* get(FontId fontId, const lv_font_t* fallBack = NULL);

   private:
      std::unordered_map<FontId, void*> font_map_;
   };

}  // namespace els::cpro2::common::platform::gui::fonts
#endif  // __cplusplus

#endif  // FONT_MANAGER_H
