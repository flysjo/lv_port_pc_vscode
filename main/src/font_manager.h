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
#include "filefont.h"
// #include "grabber_types.h"
// #include "grabber_fixedfonts.h"

#if defined(__cplusplus)
#include <memory>
#include <unordered_map>

namespace els::cpro2::common::platform::gui::fonts
{
   typedef uint32_t FontId;

   class FontManager
   {
   public:
      FontManager() = default;
      ~FontManager() = default;
      FontManager(const FontManager&) = delete;

      /**
       * @brief load a font from file and store it in the font manager
       *
       * @param filename      name of the file to load
       * @param fontId        id of the font to store
       * @return lv_font_t*   pointer to the loaded font, NULL if failed to load
       */
      lv_font_t* Load(const char* filename, FontId fontId);

      /**
       * @brief unload a font from the font manager
       *
       * @param fontId  id of the font to unload
       * @return true   font unloaded
       * @return false  id not found
       */
      bool Unload(FontId fontId);

      /**
       * @brief get a font from the font manager
       *
       * @param fontId     id of the font to get
       * @return const lv_font_t*
       */
      const lv_font_t* Get(FontId fontId);

      /**
       * @brief Load all expected fonts from the filesystem at once.
       */
      void LoadAllFonts();

   private:
      std::unordered_map<FontId, std::shared_ptr<IFileFont>> font_map_;
   };

   std::unique_ptr<FontManager> MakeFontManager();

}  // namespace els::cpro2::common::platform::gui::fonts
#endif  // __cplusplus

#endif  // FONT_MANAGER_H
