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
   FontManager::~FontManager()
   {
      for (auto it = font_map_.begin(); it != font_map_.end(); ++it)
      {
         auto ptr = reinterpret_cast<FileFont*>(it->second);
         delete ptr;
      }
   }
   lv_font_t* FontManager::load(const char* filename, FontId fontId)
   {
      FileFont* font = new FileFont();
      if (!font->load(filename))
      {
         delete font;
         return NULL;
      }
      #if defined(SIMULATOR)
      size_t size = font->size_of();
      size_t num_chars = font->num_chars();
      float kvot = size / num_chars;
      std::cout << "Font[" << filename << "] size: " << size << " num chars: " << num_chars << " / " << kvot << std::endl;
    #endif
      font_map_[fontId] = font;
      return font->get();
   }
   bool FontManager::unload(FontId fontId)
   {
      auto it = font_map_.find(fontId);
      if (it != font_map_.end())
      {
         auto ptr = reinterpret_cast<FileFont*>(it->second);
         delete ptr;
         font_map_.erase(it);
         return true;
      }
      return false;
   }
   const lv_font_t* FontManager::get(FontId fontId, const lv_font_t* fallBack)
   {
      if (font_map_.find(fontId) != font_map_.end())
      {
         FileFont *font = reinterpret_cast<FileFont*>(font_map_[fontId]);
         return font->get();
      }
      return fallBack;
   }

}  // namespace els::cpro2::common::platform::gui::fonts