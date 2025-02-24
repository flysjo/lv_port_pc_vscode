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

#include "../normal_2.h"

#include "lvgl/src/misc/lv_utils.h"


typedef struct {
    uint32_t gid_left;
    uint32_t gid_right;
} kern_pair_ref_t;

extern "C" bool my_get_glyph_dsc_cb(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode, uint32_t unicode_next);
#if LV_VERSION_CHECK(8, 0, 0)
extern "C" const void *my_get_glyph_bitmap_cb(const lv_font_t *font, uint32_t unicode);
#elif LV_VERSION_CHECK(9, 0, 0)
extern "C" const void * my_get_glyph_bitmap_cb(lv_font_glyph_dsc_t *, uint32_t, lv_draw_buf_t *);
#endif

static const uint8_t opa4_table[16] = {0,  17, 34,  51,
                                       68, 85, 102, 119,
                                       136, 153, 170, 187,
                                       204, 221, 238, 255
                                      };

#if LV_USE_FONT_COMPRESSED
static const uint8_t opa3_table[8] = {0, 36, 73, 109, 146, 182, 218, 255};
#endif

static const uint8_t opa2_table[4] = {0, 85, 170, 255};

static int32_t unicode_list_compare(const void * ref, const void * element)
{
    return ((int32_t)(*(uint16_t *)ref)) - ((int32_t)(*(uint16_t *)element));
}

static int32_t kern_pair_8_compare(const void * ref, const void * element)
{
    const kern_pair_ref_t * ref8_p = reinterpret_cast<const kern_pair_ref_t*>(ref);
    const uint8_t * element8_p = reinterpret_cast<const uint8_t*>(element);

    /*If the MSB is different it will matter. If not return the diff. of the LSB*/
    if(ref8_p->gid_left != element8_p[0]) return (int32_t) ref8_p->gid_left - element8_p[0];
    else return (int32_t) ref8_p->gid_right - element8_p[1];

}

static int32_t kern_pair_16_compare(const void * ref, const void * element)
{
    const kern_pair_ref_t * ref16_p = reinterpret_cast<const kern_pair_ref_t *>(ref);
    const uint16_t * element16_p = reinterpret_cast<const uint16_t *>(element);

    /*If the MSB is different it will matter. If not return the diff. of the LSB*/
    if(ref16_p->gid_left != element16_p[0]) return (int32_t) ref16_p->gid_left - element16_p[0];
    else return (int32_t) ref16_p->gid_right - element16_p[1];
}

typedef struct {
    int8_t bit_pos;
    uint8_t byte_value;
} bit_iterator_t;

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

namespace els::cpro2::common::util::fontmgr
{
    class Font
    {
    public:
        Font(const char* filename) : filename_(filename), file_(nullptr), size_(0)
        {
            memset(&font_, 0, sizeof(lv_font_t));
            font_.get_glyph_dsc = my_get_glyph_dsc_cb;
#if LV_VERSION_CHECK(8, 0, 0)
            font_.get_glyph_bitmap = my_get_glyph_bitmap_cb;
#elif LV_VERSION_CHECK(9, 0, 0)
            font_.get_glyph_bitmap = my_get_glyph_bitmap_cb;
#endif
            font_.user_data = (void*)this;
        }
        ~Font()
        {
            if (file_)
            {
                fclose(file_);
            }
            if (font_.dsc)
            {
                const lv_font_fmt_txt_dsc_t * font_dsc = reinterpret_cast<const lv_font_fmt_txt_dsc_t *>(font_.dsc);
                if (font_dsc->cmaps)
                {
                    for (int i = 0; i < font_dsc->cmap_num; ++i)
                    {
                        delete font_dsc->cmaps[i].unicode_list;
                        const uint8_t * glyph_id_ofs_list = reinterpret_cast<const uint8_t*>(font_dsc->cmaps[i].glyph_id_ofs_list);
                        delete glyph_id_ofs_list;
                    }
                    delete font_dsc->cmaps;
                }
                if (font_dsc->kern_dsc && font_dsc->kern_classes)
                {
                    const lv_font_fmt_txt_kern_classes_t* kern_classes = reinterpret_cast<const lv_font_fmt_txt_kern_classes_t*>(font_dsc->kern_dsc);
                    const uint8_t* left_class_mapping = reinterpret_cast<const uint8_t*>(kern_classes->left_class_mapping);
                    const uint8_t* right_class_mapping = reinterpret_cast<const uint8_t*>(kern_classes->right_class_mapping);
                    const int8_t* class_pair_values = reinterpret_cast<const int8_t*>(kern_classes->class_pair_values);
                    delete class_pair_values;
                    delete left_class_mapping;
                    delete right_class_mapping;
                    delete kern_classes;
                }
                else if (font_dsc->kern_dsc)
                {
                    const lv_font_fmt_txt_kern_pair_t* kern_pair = reinterpret_cast<const lv_font_fmt_txt_kern_pair_t*>(font_dsc->kern_dsc);
                    const uint8_t* glyph_ids = reinterpret_cast<const uint8_t*>(kern_pair->glyph_ids);
                    const int8_t* values = reinterpret_cast<const int8_t*>(kern_pair->values);
                    delete glyph_ids;
                    delete values;
                    delete kern_pair;
                }
                if (font_dsc->glyph_dsc)
                {
                    const lv_font_fmt_txt_glyph_dsc_t* gdsc = reinterpret_cast<const lv_font_fmt_txt_glyph_dsc_t*>(font_dsc->glyph_dsc);
                    delete gdsc;
                }
                delete font_dsc;
            }
            delete glyph_offset_;
        }
        lv_font_t* get() { return &font_; }
        bool load_glyph_dsc(uint32_t gid, lv_font_fmt_txt_glyph_dsc_t & gdsc)
        {
            if (!seek(glyph_start_ + glyph_offset_[gid]))
            {
                return false;
            }

            bit_iterator_t bit_it = init_bit_iterator();
            int res;

            if(font_header_.advance_width_bits == 0) {
                gdsc.adv_w = font_header_.default_advance_width;
            }
            else {
                gdsc.adv_w = read_bits(&bit_it, font_header_.advance_width_bits, &res);
                if (!res) {
                    return -1;
                }
            }

            if(font_header_.advance_width_format == 0) {
                gdsc.adv_w *= 16;
            }

            gdsc.ofs_x = read_bits_signed(&bit_it, font_header_.xy_bits, &res);
            if (!res) {
                return -1;
            }

            gdsc.ofs_y = read_bits_signed(&bit_it, font_header_.xy_bits, &res);
            if (!res) {
                return -1;
            }

            gdsc.box_w = read_bits(&bit_it, font_header_.wh_bits, &res);
            if (!res) {
                return -1;
            }

            gdsc.box_h = read_bits(&bit_it, font_header_.wh_bits, &res);
            if (!res) {
                return -1;
            }

            if (gid == 0) {
                gdsc.adv_w = 0;
                gdsc.box_w = 0;
                gdsc.box_h = 0;
                gdsc.ofs_x = 0;
                gdsc.ofs_y = 0;
            }
            return true;
        }
        bool get_glyph_dsc(lv_font_glyph_dsc_t *dsc_out, uint32_t unicode, uint32_t unicode_next)
        {
#if LV_VERSION_CHECK(9, 0, 0)
            // get_glyph_bitmap callback v9 needs to know who called it
            dsc_out->entry = reinterpret_cast<lv_cache_entry_t*>(this);
#endif
            /*It fixes a strange compiler optimization issue: https://github.com/lvgl/lvgl/issues/4370*/
            bool is_tab = unicode == '\t';
            if(is_tab) {
                unicode = ' ';
            }
            lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *)font_.dsc;
            uint32_t gid = get_glyph_dsc_id(unicode);
            if(!gid) return false;

            int8_t kvalue = 0;
            if(fdsc->kern_dsc) {
                uint32_t gid_next = get_glyph_dsc_id(unicode_next);
                if(gid_next) {
                    kvalue = get_kern_value(gid, gid_next);
                }
            }
            /*Put together a glyph dsc*/
            lv_font_fmt_txt_glyph_dsc_t gdsc;
            if (!load_glyph_dsc(gid, gdsc))
            {
                return false;
            }
            int32_t kv = ((int32_t)((int32_t)kvalue * fdsc->kern_scale) >> 4);

            uint32_t adv_w = gdsc.adv_w;
            if(is_tab) adv_w *= 2;

            adv_w += kv;
            adv_w  = (adv_w + (1 << 3)) >> 4;

            dsc_out->adv_w = adv_w;
            dsc_out->box_h = gdsc.box_h;
            dsc_out->box_w = gdsc.box_w;
            dsc_out->ofs_x = gdsc.ofs_x;
            dsc_out->ofs_y = gdsc.ofs_y;
            dsc_out->format = (uint8_t)fdsc->bpp;
            dsc_out->is_placeholder = false;

            if(is_tab) dsc_out->box_w = dsc_out->box_w * 2;
            return true;

        }
        const void * get_glyph_bitmap(lv_font_glyph_dsc_t * g_dsc, uint32_t unicode_letter, lv_draw_buf_t * draw_buf)
        {
            uint8_t * bitmap_out = draw_buf->data;

            if(unicode_letter == '\t') unicode_letter = ' ';

            lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *)font_.dsc;
            uint32_t gid = get_glyph_dsc_id(unicode_letter);
            if(!gid) return NULL;

            // const lv_font_fmt_txt_glyph_dsc_t * gdsc = &fdsc->glyph_dsc[gid];


            if(fdsc->bitmap_format == LV_FONT_FMT_TXT_PLAIN)
            {
                int next_offset = (gid < loca_count_ - 1) ? glyph_offset_[gid + 1] : (uint32_t)glyph_length_;
                size_t startPos = glyph_start_ + glyph_offset_[gid];
                seek(startPos);

                int bmp_size = next_offset - glyph_offset_[gid] - nbits_ / 8;
                if(bmp_size == 0) return NULL;
                int res;
                int nbits_bytes = (nbits_ + 7) / 8;
                bit_iterator_t bit_it = init_bit_iterator();
                if (!read_bits(&bit_it, nbits_, &res))
                {
                    return NULL;
                }
                std::vector<uint8_t> bitmap_in_tmp(bmp_size);
                if (nbits_ % 8 == 0) {
                    if (!read(bitmap_in_tmp.data(), bmp_size, NULL))
                    {
                        return NULL;
                    }
                } else {
                    for(int k = 0; k < bmp_size - 1; ++k) {
                        bitmap_in_tmp[k] = read_bits(&bit_it, 8, &res);
                        if(!res) {
                            return NULL;
                        }
                    }
                    bitmap_in_tmp[bmp_size - 1] = read_bits(&bit_it, 8 - nbits_ % 8, &res);
                    if(!res) {
                        return NULL;
                    }

                    /*The last fragment should be on the MSB but read_bits() will place it to the LSB*/
                    bitmap_in_tmp[bmp_size - 1] = bitmap_in_tmp[bmp_size - 1] << (nbits_ % 8);
                }
                const uint8_t * bitmap_in = &bitmap_in_tmp[0];
                uint8_t * bitmap_out_tmp = bitmap_out;
                int32_t i = 0;
                int32_t x, y;
                uint32_t stride = lv_draw_buf_width_to_stride(g_dsc->box_w, LV_COLOR_FORMAT_A8);

                if(fdsc->bpp == 1) {
                    for(y = 0; y < g_dsc->box_h; y ++) {
                        for(x = 0; x < g_dsc->box_w; x++, i++) {
                            i = i & 0x7;
                            if(i == 0) bitmap_out_tmp[x] = (*bitmap_in) & 0x80 ? 0xff : 0x00;
                            else if(i == 1) bitmap_out_tmp[x] = (*bitmap_in) & 0x40 ? 0xff : 0x00;
                            else if(i == 2) bitmap_out_tmp[x] = (*bitmap_in) & 0x20 ? 0xff : 0x00;
                            else if(i == 3) bitmap_out_tmp[x] = (*bitmap_in) & 0x10 ? 0xff : 0x00;
                            else if(i == 4) bitmap_out_tmp[x] = (*bitmap_in) & 0x08 ? 0xff : 0x00;
                            else if(i == 5) bitmap_out_tmp[x] = (*bitmap_in) & 0x04 ? 0xff : 0x00;
                            else if(i == 6) bitmap_out_tmp[x] = (*bitmap_in) & 0x02 ? 0xff : 0x00;
                            else if(i == 7) {
                                bitmap_out_tmp[x] = (*bitmap_in) & 0x01 ? 0xff : 0x00;
                                bitmap_in++;
                            }
                        }
                        bitmap_out_tmp += stride;
                    }
                }
                else if(fdsc->bpp == 2) {
                    for(y = 0; y < g_dsc->box_h; y ++) {
                        for(x = 0; x < g_dsc->box_w; x++, i++) {
                            i = i & 0x3;
                            if(i == 0) bitmap_out_tmp[x] = opa2_table[(*bitmap_in) >> 6];
                            else if(i == 1) bitmap_out_tmp[x] = opa2_table[((*bitmap_in) >> 4) & 0x3];
                            else if(i == 2) bitmap_out_tmp[x] = opa2_table[((*bitmap_in) >> 2) & 0x3];
                            else if(i == 3) {
                                bitmap_out_tmp[x] = opa2_table[((*bitmap_in) >> 0) & 0x3];
                                bitmap_in++;
                            }
                        }
                        bitmap_out_tmp += stride;
                    }
                }
                else if(fdsc->bpp == 4) {
                    for(y = 0; y < g_dsc->box_h; y ++) {
                        for(x = 0; x < g_dsc->box_w; x++, i++) {
                            i = i & 0x1;
                            if(i == 0) {
                                bitmap_out_tmp[x] = opa4_table[(*bitmap_in) >> 4];
                            }
                            else if(i == 1) {
                                bitmap_out_tmp[x] = opa4_table[(*bitmap_in) & 0xF];
                                bitmap_in++;
                            }
                        }
                        bitmap_out_tmp += stride;
                    }
                }
                return draw_buf;
            }
            /*Handle compressed bitmap*/
            else {
        #if LV_USE_FONT_COMPRESSED
                bool prefilter = fdsc->bitmap_format == LV_FONT_FMT_TXT_COMPRESSED;
                decompress(&fdsc->glyph_bitmap[gdsc->bitmap_index], bitmap_out, gdsc->box_w, gdsc->box_h,
                        (uint8_t)fdsc->bpp, prefilter);
                return draw_buf;
        #else /*!LV_USE_FONT_COMPRESSED*/
                LV_LOG_WARN("Compressed fonts is used but LV_USE_FONT_COMPRESSED is not enabled in lv_conf.h");
                return NULL;
        #endif
            }

            /*If not returned earlier then the letter is not found in this font*/
            return NULL;
        }
        int8_t get_kern_value(uint32_t gid_left, uint32_t gid_right)
        {
            lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *)font_.dsc;

            int8_t value = 0;

            if(fdsc->kern_classes == 0) {
                /*Kern pairs*/
                const lv_font_fmt_txt_kern_pair_t * kdsc = reinterpret_cast<const lv_font_fmt_txt_kern_pair_t *>(fdsc->kern_dsc);
                if(kdsc->glyph_ids_size == 0) {
                    /*Use binary search to find the kern value.
                    *The pairs are ordered left_id first, then right_id secondly.*/
                    const uint16_t * g_ids = reinterpret_cast<const uint16_t *>(kdsc->glyph_ids);
                    kern_pair_ref_t g_id_both = {gid_left, gid_right};
                    uint16_t * kid_p =reinterpret_cast<uint16_t *>(_lv_utils_bsearch(&g_id_both, g_ids, kdsc->pair_cnt, 2, kern_pair_8_compare));

                    /*If the `g_id_both` were found get its index from the pointer*/
                    if(kid_p) {
                        lv_uintptr_t ofs = kid_p - g_ids;
                        value = kdsc->values[ofs];
                    }
                }
                else if(kdsc->glyph_ids_size == 1) {
                    /*Use binary search to find the kern value.
                    *The pairs are ordered left_id first, then right_id secondly.*/
                    const uint32_t * g_ids = reinterpret_cast<const uint32_t *>(kdsc->glyph_ids);
                    kern_pair_ref_t g_id_both = {gid_left, gid_right};
                    uint32_t * kid_p = reinterpret_cast<uint32_t *>(_lv_utils_bsearch(&g_id_both, g_ids, kdsc->pair_cnt, 4, kern_pair_16_compare));

                    /*If the `g_id_both` were found get its index from the pointer*/
                    if(kid_p) {
                        lv_uintptr_t ofs = kid_p - g_ids;
                        value = kdsc->values[ofs];
                    }

                }
                else {
                    /*Invalid value*/
                }
            }
            else {
                /*Kern classes*/
                const lv_font_fmt_txt_kern_classes_t * kdsc = reinterpret_cast<const lv_font_fmt_txt_kern_classes_t *>(fdsc->kern_dsc);
                uint8_t left_class = kdsc->left_class_mapping[gid_left];
                uint8_t right_class = kdsc->right_class_mapping[gid_right];

                /*If class = 0, kerning not exist for that glyph
                *else got the value form `class_pair_values` 2D array*/
                if(left_class > 0 && right_class > 0) {
                    value = kdsc->class_pair_values[(left_class - 1) * kdsc->right_class_cnt + (right_class - 1)];
                }

            }
            return value;
        }
        size_t num_chars()
        {
            return loca_count_;
        }
        size_t size_of() {
            return sizeof(this) + size_;
        }
        bool load()
        {
            file_ = fopen(filename_.c_str(), "rb");
            if (!file_)
            {
                return false;
            }
            lv_font_fmt_txt_dsc_t * font_dsc = reinterpret_cast<lv_font_fmt_txt_dsc_t*>(fontMalloc(sizeof(lv_font_fmt_txt_dsc_t)));
            memset(font_dsc, 0, sizeof(lv_font_fmt_txt_dsc_t));
            font_.dsc = reinterpret_cast<void*>(font_dsc);
            /*header*/
            int32_t header_length = read_label(0, "head");
            if(header_length < 0) {
                return false;
            }

            if (!read(&font_header_, sizeof(font_header_bin_t), NULL)) {
                return false;
            }
            font_.base_line = -font_header_.descent;
            font_.line_height = font_header_.ascent - font_header_.descent;
            font_.subpx = font_header_.subpixels_mode;
            font_.underline_position = font_header_.underline_position;
            font_.underline_thickness = font_header_.underline_thickness;
            nbits_ = font_header_.advance_width_bits + 2 * font_header_.xy_bits + 2 * font_header_.wh_bits;
            font_dsc->bpp = font_header_.bits_per_pixel;
            font_dsc->kern_scale = font_header_.kerning_scale;
            font_dsc->bitmap_format = font_header_.compression_id;

            /*cmaps*/
            uint32_t cmaps_start = header_length;
            int32_t cmaps_length = load_cmaps(font_dsc, cmaps_start);
            if(cmaps_length < 0) {
                return false;
            }
            /*loca*/
            uint32_t loca_start = cmaps_start + cmaps_length;
            int32_t loca_length = read_label(loca_start, "loca");
            if(loca_length < 0) {
                return false;
            }
            if (!read(&loca_count_, sizeof(uint32_t), NULL)) {
                return false;
            }

            bool failed = false;
            glyph_offset_ = reinterpret_cast<uint32_t*>(fontMalloc(sizeof(uint32_t) * (loca_count_ + 1)));
            if (font_header_.index_to_loc_format == 0) {
                for (unsigned int i = 0; i < loca_count_; ++i) {
                    uint16_t offset;
                    if (!read(&offset, sizeof(uint16_t), NULL)) {
                        failed = true;
                        break;
                    }
                    glyph_offset_[i] = offset;
                }
            }
            else if(font_header_.index_to_loc_format == 1) {
                if (!read(glyph_offset_, loca_count_ * sizeof(uint32_t), NULL)) {
                    failed = true;
                }
            }
            else {
                LV_LOG_WARN("Unknown index_to_loc_format: %d.", font_header_.index_to_loc_format);
                failed = true;
            }
            if(failed) {
                delete glyph_offset_;
                return false;
            }
            /* glyph */
            glyph_start_ = loca_start + loca_length;
            // glyph_length_ = read_label(glyph_start_, "glyf");
            glyph_length_ = load_glyph(font_dsc, glyph_start_, glyph_offset_, loca_count_, &font_header_);

            if(glyph_length_ < 0) {
                return -1;
            }
            if (font_header_.tables_count < 4) {
                font_dsc->kern_dsc = NULL;
                font_dsc->kern_classes = 0;
                font_dsc->kern_scale = 0;
                return true;
            }

            uint32_t kern_start = glyph_start_ + glyph_length_;
            int32_t kern_length = load_kern(font_dsc, font_header_.glyph_id_format, kern_start);
            return kern_length >= 0;
        }
    protected:
        uint32_t get_glyph_dsc_id(uint32_t letter)
        {
            if(letter == '\0') return 0;

            lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *)font_.dsc;

            uint16_t i;
            for(i = 0; i < fdsc->cmap_num; i++) {

                /*Relative code point*/
                uint32_t rcp = letter - fdsc->cmaps[i].range_start;
                if(rcp >= fdsc->cmaps[i].range_length) continue;
                uint32_t glyph_id = 0;
                if(fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY) {
                    glyph_id = fdsc->cmaps[i].glyph_id_start + rcp;
                }
                else if(fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL) {
                    const uint8_t * gid_ofs_8 = reinterpret_cast<const uint8_t*>(fdsc->cmaps[i].glyph_id_ofs_list);
                    glyph_id = fdsc->cmaps[i].glyph_id_start + gid_ofs_8[rcp];
                }
                else if(fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_SPARSE_TINY) {
                    uint16_t key = rcp;
                    uint16_t * p = reinterpret_cast<uint16_t*>(_lv_utils_bsearch(&key, fdsc->cmaps[i].unicode_list, fdsc->cmaps[i].list_length,
                                                    sizeof(fdsc->cmaps[i].unicode_list[0]), unicode_list_compare));

                    if(p) {
                        lv_uintptr_t ofs = p - fdsc->cmaps[i].unicode_list;
                        glyph_id = fdsc->cmaps[i].glyph_id_start + (uint32_t) ofs;
                    }
                }
                else if(fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_SPARSE_FULL) {
                    uint16_t key = rcp;
                    uint16_t * p = reinterpret_cast<uint16_t*>(_lv_utils_bsearch(&key, fdsc->cmaps[i].unicode_list, fdsc->cmaps[i].list_length,
                                                    sizeof(fdsc->cmaps[i].unicode_list[0]), unicode_list_compare));

                    if(p) {
                        lv_uintptr_t ofs = p - fdsc->cmaps[i].unicode_list;
                        const uint16_t * gid_ofs_16 = reinterpret_cast<const uint16_t *>(fdsc->cmaps[i].glyph_id_ofs_list);
                        glyph_id = fdsc->cmaps[i].glyph_id_start + gid_ofs_16[ofs];
                    }
                }

                return glyph_id;
            }
            return 0;

        }
        int32_t load_glyph(lv_font_fmt_txt_dsc_t * font_dsc,
                                uint32_t start, uint32_t * glyph_offset, uint32_t loca_count, font_header_bin_t * header)
        {
            int32_t glyph_length = read_label(start, "glyf");
            if(glyph_length < 0) {
                return -1;
            }
            return glyph_length;
        }
        bit_iterator_t init_bit_iterator()
        {
            bit_iterator_t it;
            it.bit_pos = -1;
            it.byte_value = 0;
            return it;
        }

        unsigned int read_bits(bit_iterator_t * it, int n_bits, int * res)
        {
            unsigned int value = 0;
            while (n_bits--) {
                it->byte_value = it->byte_value << 1;
                it->bit_pos--;

                if(it->bit_pos < 0) {
                    it->bit_pos = 7;
                    if (!read(&(it->byte_value), 1, NULL))
                    {
                        *res = 0;
                        return 0;
                    }
                }
                int8_t bit = (it->byte_value & 0x80) ? 1 : 0;

                value |= (bit << n_bits);
            }
            *res = 1;
            return value;
        }

        int read_bits_signed(bit_iterator_t * it, int n_bits, int * res)
        {
            unsigned int value = read_bits(it, n_bits, res);
            if(value & (1 << (n_bits - 1))) {
                value |= ~0u << n_bits;
            }
            return value;
        }
        size_t load_cmaps(lv_font_fmt_txt_dsc_t * font_dsc, uint32_t cmaps_start)
        {
            int32_t cmaps_length = read_label(cmaps_start, "cmap");
            if(cmaps_length < 0) {
                return -1;
            }

            uint32_t cmaps_subtables_count;
            if (!read(&cmaps_subtables_count, sizeof(uint32_t), NULL)) {
                return -1;
            }

            lv_font_fmt_txt_cmap_t * cmaps = reinterpret_cast<lv_font_fmt_txt_cmap_t*>(fontMalloc(sizeof(lv_font_fmt_txt_cmap_t)*cmaps_subtables_count));
            memset(cmaps, 0, cmaps_subtables_count * sizeof(lv_font_fmt_txt_cmap_t));

            font_dsc->cmaps = cmaps;
            font_dsc->cmap_num = cmaps_subtables_count;

            // std::unique_ptr<cmap_table_bin_t[font_dsc->cmap_num]> cmaps_tables = new cmap_table_bin_t[font_dsc->cmap_num];
            // std::vector<cmap_table_bin_t> cmaps_tables(font_dsc->cmap_num);
            auto cmaps_tables = reinterpret_cast<cmap_table_bin_t*>(fontMalloc(sizeof(cmap_table_bin_t)*cmaps_subtables_count));
            if (cmaps_tables == nullptr)
            {
                return -1;
            }
            bool success = load_cmaps_tables(font_dsc, cmaps_start, &cmaps_tables[0]);
            return success ? cmaps_length : -1;
        }
        bool load_cmaps_tables(lv_font_fmt_txt_dsc_t * font_dsc, uint32_t cmaps_start, cmap_table_bin_t * cmap_table)
        {
            if (!read(cmap_table, font_dsc->cmap_num * sizeof(cmap_table_bin_t), NULL)) {
                return false;
            }

            for (unsigned int i = 0; i < font_dsc->cmap_num; ++i) {
                if (!seek(cmaps_start + cmap_table[i].data_offset)) {
                    return false;
                }

                lv_font_fmt_txt_cmap_t * cmap = (lv_font_fmt_txt_cmap_t *) & (font_dsc->cmaps[i]);

                cmap->range_start = cmap_table[i].range_start;
                cmap->range_length = cmap_table[i].range_length;
                cmap->glyph_id_start = cmap_table[i].glyph_id_start;
                cmap->type = cmap_table[i].format_type;

                switch(cmap_table[i].format_type) {
                    case LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL: {
                            uint8_t ids_size = sizeof(uint8_t) * cmap_table[i].data_entries_count;
                            uint8_t * glyph_id_ofs_list = reinterpret_cast<uint8_t*>(fontMalloc(sizeof(uint8_t)*ids_size));
                            cmap->glyph_id_ofs_list = glyph_id_ofs_list;
                            if (!read(glyph_id_ofs_list, ids_size, NULL)) {
                                return false;
                            }
                            cmap->list_length = cmap->range_length;
                            break;
                        }
                    case LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY:
                        break;
                    case LV_FONT_FMT_TXT_CMAP_SPARSE_FULL:
                    case LV_FONT_FMT_TXT_CMAP_SPARSE_TINY: {
                            uint32_t list_size = sizeof(uint16_t) * cmap_table[i].data_entries_count;
                            uint16_t * unicode_list = reinterpret_cast<uint16_t*>(fontMalloc(sizeof(uint16_t)*list_size));

                            cmap->unicode_list = unicode_list;
                            cmap->list_length = cmap_table[i].data_entries_count;

                            if (!read(unicode_list, list_size, NULL)) {
                                return false;
                            }

                            if (cmap_table[i].format_type == LV_FONT_FMT_TXT_CMAP_SPARSE_FULL) {
                                uint16_t * buf = reinterpret_cast<uint16_t*>(fontMalloc(sizeof(uint16_t) * cmap->list_length));

                                cmap->glyph_id_ofs_list = buf;

                                if (!read(buf, sizeof(uint16_t) * cmap->list_length, NULL)) {
                                    return false;
                                }
                            }
                            break;
                        }
                    default:
                        LV_LOG_WARN("Unknown cmaps format type %d.", cmap_table[i].format_type);
                        return false;
                }
            }
            return true;
        }
        int32_t load_kern(lv_font_fmt_txt_dsc_t * font_dsc, uint8_t format, uint32_t start)
        {
            int32_t kern_length = read_label(start, "kern");
            if (kern_length < 0) {
                return -1;
            }

            uint8_t kern_format_type;
            int32_t padding;
            if (!read(&kern_format_type, sizeof(uint8_t), NULL) || !read(&padding, 3 * sizeof(uint8_t), NULL)) {
                return -1;
            }

            if (0 == kern_format_type) { /*sorted pairs*/
                lv_font_fmt_txt_kern_pair_t * kern_pair = reinterpret_cast<lv_font_fmt_txt_kern_pair_t*>(fontMalloc(sizeof(lv_font_fmt_txt_kern_pair_t)));
                memset(kern_pair, 0, sizeof(lv_font_fmt_txt_kern_pair_t));
                font_dsc->kern_dsc = kern_pair;
                font_dsc->kern_classes = 0;

                uint32_t glyph_entries;
                if (!read(&glyph_entries, sizeof(uint32_t), NULL)) {
                    return -1;
                }

                int ids_size;
                if(format == 0) {
                    ids_size = sizeof(int8_t) * 2 * glyph_entries;
                }
                else {
                    ids_size = sizeof(int16_t) * 2 * glyph_entries;
                }

                uint8_t * glyph_ids = reinterpret_cast<uint8_t*>(fontMalloc(sizeof(uint8_t)*ids_size));
                int8_t * values = reinterpret_cast<int8_t*>(fontMalloc(sizeof(int8_t)*glyph_entries));

                kern_pair->glyph_ids_size = format;
                kern_pair->pair_cnt = glyph_entries;
                kern_pair->glyph_ids = glyph_ids;
                kern_pair->values = values;

                if (!read(glyph_ids, ids_size, NULL)) {
                    return -1;
                }

                if (!read(values, glyph_entries, NULL)) {
                    return -1;
                }
            }
            else if(3 == kern_format_type) { /*array M*N of classes*/

                lv_font_fmt_txt_kern_classes_t * kern_classes = reinterpret_cast<lv_font_fmt_txt_kern_classes_t*>(fontMalloc(sizeof(lv_font_fmt_txt_kern_classes_t)));
                memset(kern_classes, 0, sizeof(lv_font_fmt_txt_kern_classes_t));

                font_dsc->kern_dsc = kern_classes;
                font_dsc->kern_classes = 1;

                uint16_t kern_class_mapping_length;
                uint8_t kern_table_rows;
                uint8_t kern_table_cols;

                if (!read(&kern_class_mapping_length, sizeof(uint16_t), NULL) ||
                    !read(&kern_table_rows, sizeof(uint8_t), NULL) ||
                    !read(&kern_table_cols, sizeof(uint8_t), NULL)) {
                    return -1;
                }

                int kern_values_length = sizeof(int8_t) * kern_table_rows * kern_table_cols;

                uint8_t * kern_left = reinterpret_cast<uint8_t*>(fontMalloc(sizeof(uint8_t) * kern_class_mapping_length));
                uint8_t * kern_right = reinterpret_cast<uint8_t*>(fontMalloc(sizeof(uint8_t) * kern_class_mapping_length));
                int8_t * kern_values = reinterpret_cast<int8_t*>(fontMalloc(sizeof(int8_t) * kern_values_length));

                kern_classes->left_class_mapping  = kern_left;
                kern_classes->right_class_mapping = kern_right;
                kern_classes->left_class_cnt = kern_table_rows;
                kern_classes->right_class_cnt = kern_table_cols;
                kern_classes->class_pair_values = kern_values;

                if(!read(kern_left, kern_class_mapping_length, NULL) ||
                   !read(kern_right, kern_class_mapping_length, NULL) ||
                   !read(kern_values, kern_values_length, NULL)) {
                    return -1;
                }
            }
            else {
                LV_LOG_WARN("Unknown kern_format_type: %d", kern_format_type);
                return -1;
            }
            return kern_length;
        }
        size_t read_label(size_t start, const char* label)
        {
            seek(start);
            uint32_t length;
            char buf[4];
            if (!read(&length, 4, NULL) ||
                !read(buf, 4, NULL) ||
                memcmp(label, buf, 4) != 0) {
                LV_LOG_WARN("Error reading '%s' label.", label);
                return -1;
            }
            return length;
        }
        bool read(void* dest, size_t size, uint32_t * br) { return fread(dest, size, 1, file_) > 0; }
        bool seek(size_t pos) { return 0 == fseek(file_, pos, SEEK_SET); }
        long int tell() { return ftell(file_); }
        void * fontMalloc(std::size_t num)
        {
            auto ptr = new uint8_t[num];
            if (ptr)
            {
                std::cout << "Allocating " << num << " bytes" << std::endl;
                size_ += sizeof(uint8_t) * num;
            }
            return ptr;
        }
    private:
        FILE * file_;
        lv_font_t font_;
        std::string filename_;
        size_t size_;
        uint32_t loca_count_;
        uint32_t glyph_start_;
        int32_t glyph_length_;
        uint32_t * glyph_offset_;
        font_header_bin_t font_header_;
        int nbits_;
    };

    class FontManager
    {
    public:
        FontManager() = default;
        ~FontManager() = default;

        FontManager(const FontManager&) = delete;

        lv_font_t* Add(const char* filename)
        {
            return nullptr;
        }
    };
}

using namespace els::cpro2::common::util::fontmgr;

extern "C" bool my_get_glyph_dsc_cb(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode, uint32_t unicode_next) {
    Font *fontPtr = reinterpret_cast<Font*>(font->user_data);
    return fontPtr->get_glyph_dsc(dsc_out, unicode, unicode_next);
}
#if LV_VERSION_CHECK(8, 0, 0)
extern "C" const void *my_get_glyph_bitmap_cb(const lv_font_t *font, uint32_t unicode) {
    Font *fontPtr = reinterpret_cast<Font*>(font->user_data);
    return fontPtr->get_glyph_bitmap(unicode);
}

#elif LV_VERSION_CHECK(9, 0, 0)
extern "C" const void * my_get_glyph_bitmap_cb(lv_font_glyph_dsc_t * g_dsc, uint32_t unicode_letter, lv_draw_buf_t * draw_buf)
{
    Font *fontPtr = reinterpret_cast<Font*>(g_dsc->entry);
    return fontPtr->get_glyph_bitmap(g_dsc, unicode_letter, draw_buf);
}
#endif

std::map<lv_font_t*, Font*> font_map;


extern "C" {

    void fontmgr_init()
    {
        const lv_font_t* font_normal_2 =  &normal_2;
        std::cout << "normal_2 font size: " << sizeof(normal_2) << std::endl;
    }

    lv_font_t *fontmgr_load(const char *name)
    {
        Font* font = new Font(name);
        if (!font->load())
        {
            delete font;
            return NULL;
        }
        size_t size = font->size_of();
        size_t num_chars = font->num_chars();
        float kvot = size / num_chars;
        std::cout << "Font[" << name << "] size: " << size << " num chars: " << num_chars << " / " << kvot << std::endl;
        font_map[font->get()] = font;
        return font->get();
    }

    void fontmgr_unload(lv_font_t *font)
    {
        auto it = font_map.find(font);
        if (it != font_map.end())
        {
            delete it->second;
            font_map.erase(it);
        }
    }
}
