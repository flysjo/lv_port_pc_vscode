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
   constexpr size_t kMaxGlyphsInCache = 20;

   class GlyphItem
   {
   public:
      GlyphItem(uint32_t gid, const lv_font_glyph_dsc_t &dsc, uint32_t now) : gid_(gid), glyph_dsc_(dsc), accessedTimeStamp(now) {}
      ~GlyphItem() {}
      void touch()
      {
         accessedTimeStamp = lv_tick_get() / 100;
#if defined(SIMULATOR)
         std::cout << "touch gid: " << gid_ << " -> " << accessedTimeStamp << std::endl;
#endif
      }
      bool has_bitmap()
      {
         touch();
         return !bitmap_.empty();
      }
      size_t size_of() const { return sizeof(GlyphItem) + bitmap_.size(); }
      uint32_t gid_;
      lv_font_glyph_dsc_t glyph_dsc_;
      std::vector<uint8_t> bitmap_;
      uint32_t accessedTimeStamp;
   };

   class GlyphCache
   {
   public:
      GlyphCache() : cache_() {}
      GlyphItem *get(uint32_t glyph_id)
      {
         auto it = cache_.find(glyph_id);
         if (it == cache_.end())
         {
            return nullptr;
         }
         it->second->touch();
         return it->second;
      }

      void checkSize()
      {
         while (cache_.size() >= kMaxGlyphsInCache)
         {
            uint32_t oldest = UINT32_MAX;
            uint32_t oldest_gid = UINT32_MAX;
            for (auto it = cache_.begin(); it != cache_.end(); ++it)
            {
#if defined(SIMULATOR)
// std::cout << "cached gid: " << it->first << " -> " << it->second->accessedTimeStamp << " usage:" <<  it->second.use_count() << std::endl;
#endif
               if (it->second->accessedTimeStamp < oldest)
               {
                  oldest = it->second->accessedTimeStamp;
                  oldest_gid = it->first;
                  // std::cout << "  new oldest gid: " << oldest_gid << std::endl;
               }
            }
            if (oldest_gid != UINT32_MAX)
            {
               if (cache_.find(oldest_gid) != cache_.end())
               {
#if defined(SIMULATOR)
                  std::cout << "gid: " << oldest_gid << " removed from cache" << std::endl;
#endif
                  cache_.erase(oldest_gid);
               }
            }
         }
#if defined(SIMULATOR)
         std::cout << "current cache memory usage: " << size_of() << std::endl;
#endif
}
      void add_dsc(uint32_t glyph_id, const lv_font_glyph_dsc_t &glyph_dsc)
      {
         checkSize();
         auto sp = new GlyphItem(glyph_id, glyph_dsc, lv_tick_get());
         cache_[glyph_id] = sp;
#if defined(SIMULATOR)
         std::cout << "gid: " << glyph_id << " added to cache" << std::endl;
#endif
      }

      size_t size_of() const
      {
         size_t size = sizeof(GlyphCache);
         for (auto it = cache_.begin(); it != cache_.end(); ++it)
         {
            size += it->second->size_of();
         }
         return size;
      }

   private:
      std::map<uint32_t, GlyphItem *> cache_;
   };

   typedef struct font_header_bin
   {
      uint32_t version;
      uint16_t tables_count;
      uint16_t font_size;
      uint16_t ascent;
      int16_t descent;
      uint16_t typo_ascent;
      int16_t typo_descent;
      uint16_t typo_line_gap;
      int16_t min_y;
      int16_t max_y;
      uint16_t default_advance_width;
      uint16_t kerning_scale;
      uint8_t index_to_loc_format;
      uint8_t glyph_id_format;
      uint8_t advance_width_format;
      uint8_t bits_per_pixel;
      uint8_t xy_bits;
      uint8_t wh_bits;
      uint8_t advance_width_bits;
      uint8_t compression_id;
      uint8_t subpixels_mode;
      uint8_t padding;
      int16_t underline_position;
      uint16_t underline_thickness;
   } font_header_bin_t;

   typedef struct cmap_table_bin
   {
      uint32_t data_offset;
      uint32_t range_start;
      uint16_t range_length;
      uint16_t glyph_id_start;
      uint16_t data_entries_count;
      uint8_t format_type;
      uint8_t padding;
   } cmap_table_bin_t;

   class FileFont
   {
   public:
      FileFont();
      ~FileFont();

      bool load(const char *fontPath);
      lv_font_t *get() { return &font_; }

      /* callback from lvgl font engine */
      bool get_glyph_dsc(lv_font_glyph_dsc_t *dsc_out, uint32_t unicode, uint32_t unicode_next);
      const uint8_t *get_glyph_bitmap(uint32_t unicode_letter);
      size_t num_chars() const
      {
         return loca_count_;
      }
      size_t size_of() const { return sizeof(FileFont) + size_; };

   protected:
      int32_t read_label(uint32_t pos, const char *label);
      bool load_glyph_dsc(uint32_t gid, lv_font_fmt_txt_glyph_dsc_t &gdsc);
      int32_t load_cmaps(lv_font_fmt_txt_dsc_t *font_dsc, uint32_t start);
      bool load_cmaps_tables(lv_font_fmt_txt_dsc_t *font_dsc, uint32_t cmaps_start, cmap_table_bin_t *cmap_table);
      int32_t load_kern(lv_font_fmt_txt_dsc_t *font_dsc, uint8_t format, uint32_t start);
      /* getters */
      int8_t get_kern_value(uint32_t gid_left, uint32_t gid_right);
      uint32_t get_glyph_dsc_id(uint32_t unicode);
      int nbits() const { return font_header_.advance_width_bits + 2 * font_header_.xy_bits + 2 * font_header_.wh_bits; }
      /* file access */
      bool seek(uint32_t pos);
      long int tell() const;
      bool read(void *buffer, size_t size, size_t *read_size);
      void *fontMalloc(std::size_t num);

   private:
      std::unique_ptr<IMonitoredFile> file_;
      lv_font_t font_;
      size_t size_;
      uint32_t loca_count_;
      uint32_t glyph_start_;
      int32_t glyph_length_;
      uint32_t *glyph_offset_;

      font_header_bin_t font_header_;
      GlyphCache glyph_cache_;
   };

}  // namespace els::cpro2::common::platform::gui::fonts

#endif  // CPRO2_COMMON_PLATFORM_GUI_FILEFONT_H_
