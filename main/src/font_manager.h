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

#include "lvgl/lvgl.h"

#if defined(__cplusplus)
#include <unordered_map>

namespace els::cpro2::common::util::fontmgr
{
    class FontManager
    {
    public:
        FontManager() = default;
        ~FontManager();
        FontManager(const FontManager&) = delete;
        lv_font_t* load(const char* filename);
        bool unload(lv_font_t* font);
    private:
        std::unordered_map<lv_font_t*, void*> font_map_;
    };

    extern FontManager theFontManager;

} // namespace els::cpro2::common::util::fontmgr
#endif  // __cplusplus

#if defined(__cplusplus)
extern "C"
{
#endif  // __cplusplus

    void fontmgr_init();

    lv_font_t *fontmgr_load(const char *name);

    bool fontmgr_unload(lv_font_t *font);

#if defined(__cplusplus)
}
#endif  // __cplusplus

#endif // FONT_MANAGER_H
