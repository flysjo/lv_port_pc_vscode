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
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "font_manager.h"
#include "filefont.h"


#include "../normal_2.h"

using namespace els::cpro2::common::util;

namespace els::cpro2::common::util::fontmgr
{
    FontManager::~FontManager()
    {
        for (auto it = font_map_.begin(); it != font_map_.end(); ++it)
        {
            auto ptr = reinterpret_cast<font::FileFont*>(it->second);
            delete ptr;
        }
    }
    lv_font_t* FontManager::load(const char* filename)
    {
        font::FileFont* font = new font::FileFont();
        if (!font->load(filename))
        {
            delete font;
            return NULL;
        }
        size_t size = font->size_of();
        size_t num_chars = font->num_chars();
        float kvot = size / num_chars;
        std::cout << "Font[" << filename << "] size: " << size << " num chars: " << num_chars << " / " << kvot << std::endl;
        font_map_[font->get()] = font;
        return font->get();
    }
    bool FontManager::unload(lv_font_t* font)
    {
        auto it = font_map_.find(font);
        if (it != font_map_.end())
        {
            auto ptr = reinterpret_cast<font::FileFont*>(it->second);
            delete ptr;
            font_map_.erase(it);
            return true;
        }
        return false;
    }
    FontManager theFontManager;
} // els::cpro2::common::util::fontmgr

extern "C" {

    void fontmgr_init()
    {
        const lv_font_t* font_normal_2 =  &normal_2;
        std::cout << "normal_2 font size: " << sizeof(normal_2) << std::endl;
    }

    lv_font_t *fontmgr_load(const char *name)
    {
        return fontmgr::theFontManager.load(name);
    }

    bool fontmgr_unload(lv_font_t *font)
    {
        return fontmgr::theFontManager.unload(font);
    }
}
