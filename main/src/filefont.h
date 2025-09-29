/**
 * @brief font class for file based fonts
 *
 * @copyright Copyright (c) 2025 Pandema AB, Electrolux Professional
 *
 * @file font.h
 * @author Claes Ivarsson (ci@pandema.com)
 * @date 2025-02-25
 *
 */

#ifndef _FONT_H_
#define _FONT_H_

#include <cstdio>
#include <vector>

#include "lvgl/lvgl.h"

namespace els::cpro2::common::util::font
{
    typedef struct font_header_bin {
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

    typedef struct cmap_table_bin {
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
    lv_font_t* get() { return &font_; }

    /* callback from lvgl font engine */
    bool get_glyph_dsc(lv_font_glyph_dsc_t *dsc_out, uint32_t unicode, uint32_t unicode_next);
    const void * get_glyph_bitmap(lv_font_glyph_dsc_t * g_dsc, lv_draw_buf_t * draw_buf);
    size_t num_chars() const { return loca_count_; }
    size_t size_of() const { return sizeof(FileFont) + size_; };
protected:
    int32_t read_label(uint32_t pos, const char * label);
    bool load_glyph_bitmap(uint32_t gid, std::vector<uint8_t> & bitmap_out);
    bool load_glyph_dsc(uint32_t gid, lv_font_fmt_txt_glyph_dsc_t & gdsc);
    int32_t load_cmaps(lv_font_fmt_txt_dsc_t * font_dsc, uint32_t start);
    bool load_cmaps_tables(lv_font_fmt_txt_dsc_t * font_dsc, uint32_t cmaps_start, cmap_table_bin_t * cmap_table);
    int32_t load_kern(lv_font_fmt_txt_dsc_t * font_dsc, uint8_t format, uint32_t start);
    /* getters */
    int8_t get_kern_value(uint32_t gid_left, uint32_t gid_right);
    uint32_t get_glyph_dsc_id(uint32_t unicode);
    int nbits() const { return font_header_.advance_width_bits + 2 * font_header_.xy_bits + 2 * font_header_.wh_bits; }
    /* file access */
    bool seek(uint32_t pos);
    long int tell() const;
    bool read(void * buffer, size_t size, size_t * read_size);
    void * fontMalloc(std::size_t num);

private:
    FILE * file_;
    lv_font_t font_;
    size_t size_;
    uint32_t loca_count_;
    uint32_t glyph_start_;
    int32_t glyph_length_;
    uint32_t * glyph_offset_;
    font_header_bin_t font_header_;
};

} // namespace els::cpro2::common::util::font

#endif // _FONT_H_
