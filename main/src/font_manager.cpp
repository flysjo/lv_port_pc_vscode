/**
 * @brief font manager
 * handles fonts in filesystem
 *
 * @copyright Copyright (c) 2025 Pandema AB, Electrolux Professional
 *
 * @file font_manager.cpp
 * @author Claes Ivarsson (ci@pandema.com)
 * @date 2025-02-19
 *
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "font_manager.h"
#include "filefont.h"
#if defined(SIMULATOR)
#include <iostream>
#endif

namespace els::cpro2::common::platform::gui::fonts
{
   lv_font_t* FontManager::Load(const char* filename, FontId fontId)
   {
      auto font = CreateFileFontObject();
      if (!font->Load(filename))
      {
         return NULL;
      }
#if defined(SIMULATOR)
      size_t size = font->SizeOf();
      size_t num_chars = font->NumChars();
      float kvot = size / num_chars;
      std::cout << "Font[" << filename << "] size: " << size << " num chars: " << num_chars << " / " << kvot << std::endl;
#endif
      font_map_[fontId] = font;
      return font_map_[fontId]->Get();
   }

   bool FontManager::Unload(FontId fontId)
   {
      auto it = font_map_.find(fontId);
      if (it != font_map_.end())
      {
         font_map_.erase(it);
         return true;
      }
      return false;
   }

   const lv_font_t* FontManager::Get(FontId fontId, const lv_font_t* fallBack)
   {
      if (font_map_.find(fontId) != font_map_.end())
      {
         return font_map_[fontId]->Get();
      }
      return fallBack;
   }

}  // namespace els::cpro2::common::platform::gui::fonts