// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_charset.h"
#include "util/string_utils.h"
namespace mysql {

struct CharsetReg {
    const char* name;
    uint8_t id;
    uint64_t hash;
};

#define CHARSET_REG(NAME, ID) {#NAME, ID, StringUtils::Hash3(#NAME)}

static CharsetReg s_charset_map[] {
    CHARSET_REG(big5,1),
    CHARSET_REG(latin2,2),
    CHARSET_REG(dec8,3),
    CHARSET_REG(cp850,4),
    CHARSET_REG(latin1,5),
    CHARSET_REG(hp8,6),
    CHARSET_REG(koi8r,7),
    CHARSET_REG(latin1,8),
    CHARSET_REG(latin2,9),
    CHARSET_REG(swe7,10),
    CHARSET_REG(ascii,11),
    CHARSET_REG(ujis,12),
    CHARSET_REG(sjis,13),
    CHARSET_REG(cp1251,14),
    CHARSET_REG(latin1,15),
    CHARSET_REG(hebrew,16),
    CHARSET_REG(tis620,18),
    CHARSET_REG(euckr,19),
    CHARSET_REG(latin7,20),
    CHARSET_REG(latin2,21),
    CHARSET_REG(koi8u,22),
    CHARSET_REG(cp1251,23),
    CHARSET_REG(gb2312,24),
    CHARSET_REG(greek,25),
    CHARSET_REG(cp1250,26),
    CHARSET_REG(latin2,27),
    CHARSET_REG(gbk,28),
    CHARSET_REG(cp1257,29),
    CHARSET_REG(latin5,30),
    CHARSET_REG(latin1,31),
    CHARSET_REG(armscii8,32),
    CHARSET_REG(utf8,33),
    CHARSET_REG(cp1250,34),
    CHARSET_REG(ucs2,35),
    CHARSET_REG(cp866,36),
    CHARSET_REG(keybcs2,37),
    CHARSET_REG(macce,38),
    CHARSET_REG(macroman,39),
    CHARSET_REG(cp852,40),
    CHARSET_REG(latin7,41),
    CHARSET_REG(latin7,42),
    CHARSET_REG(macce,43),
    CHARSET_REG(cp1250,44),
    CHARSET_REG(utf8mb4,45),
    CHARSET_REG(utf8mb4,46),
    CHARSET_REG(latin1,47),
    CHARSET_REG(latin1,48),
    CHARSET_REG(latin1,49),
    CHARSET_REG(cp1251,50),
    CHARSET_REG(cp1251,51),
    CHARSET_REG(cp1251,52),
    CHARSET_REG(macroman,53),
    CHARSET_REG(utf16,54),
    CHARSET_REG(utf16,55),
    CHARSET_REG(utf16le,56),
    CHARSET_REG(cp1256,57),
    CHARSET_REG(cp1257,58),
    CHARSET_REG(cp1257,59),
    CHARSET_REG(utf32,60),
    CHARSET_REG(utf32,61),
    CHARSET_REG(utf16le,62),
    CHARSET_REG(binary,63),
    CHARSET_REG(armscii8,64),
    CHARSET_REG(ascii,65),
    CHARSET_REG(cp1250,66),
    CHARSET_REG(cp1256,67),
    CHARSET_REG(cp866,68),
    CHARSET_REG(dec8,69),
    CHARSET_REG(greek,70),
    CHARSET_REG(hebrew,71),
    CHARSET_REG(hp8,72),
    CHARSET_REG(keybcs2,73),
    CHARSET_REG(koi8r,74),
    CHARSET_REG(koi8u,75),
    CHARSET_REG(utf8,76),
    CHARSET_REG(latin2,77),
    CHARSET_REG(latin5,78),
    CHARSET_REG(latin7,79),
    CHARSET_REG(cp850,80),
    CHARSET_REG(cp852,81),
    CHARSET_REG(swe7,82),
    CHARSET_REG(utf8,83),
    CHARSET_REG(big5,84),
    CHARSET_REG(euckr,85),
    CHARSET_REG(gb2312,86),
    CHARSET_REG(gbk,87),
    CHARSET_REG(sjis,88),
    CHARSET_REG(tis620,89),
    CHARSET_REG(ucs2,90),
    CHARSET_REG(ujis,91),
    CHARSET_REG(geostd8,92),
    CHARSET_REG(geostd8,93),
    CHARSET_REG(latin1,94),
    CHARSET_REG(cp932,95),
    CHARSET_REG(cp932,96),
    CHARSET_REG(eucjpms,97),
    CHARSET_REG(eucjpms,98),
    CHARSET_REG(cp1250,99),
    CHARSET_REG(utf16,101),
    CHARSET_REG(utf16,102),
    CHARSET_REG(utf16,103),
    CHARSET_REG(utf16,104),
    CHARSET_REG(utf16,105),
    CHARSET_REG(utf16,106),
    CHARSET_REG(utf16,107),
    CHARSET_REG(utf16,108),
    CHARSET_REG(utf16,109),
    CHARSET_REG(utf16,110),
    CHARSET_REG(utf16,111),
    CHARSET_REG(utf16,112),
    CHARSET_REG(utf16,113),
    CHARSET_REG(utf16,114),
    CHARSET_REG(utf16,115),
    CHARSET_REG(utf16,116),
    CHARSET_REG(utf16,117),
    CHARSET_REG(utf16,118),
    CHARSET_REG(utf16,119),
    CHARSET_REG(utf16,120),
    CHARSET_REG(utf16,121),
    CHARSET_REG(utf16,122),
    CHARSET_REG(utf16,123),
    CHARSET_REG(utf16,124),
    CHARSET_REG(ucs2,128),
    CHARSET_REG(ucs2,129),
    CHARSET_REG(ucs2,130),
    CHARSET_REG(ucs2,131),
    CHARSET_REG(ucs2,132),
    CHARSET_REG(ucs2,133),
    CHARSET_REG(ucs2,134),
    CHARSET_REG(ucs2,135),
    CHARSET_REG(ucs2,136),
    CHARSET_REG(ucs2,137),
    CHARSET_REG(ucs2,138),
    CHARSET_REG(ucs2,139),
    CHARSET_REG(ucs2,140),
    CHARSET_REG(ucs2,141),
    CHARSET_REG(ucs2,142),
    CHARSET_REG(ucs2,143),
    CHARSET_REG(ucs2,144),
    CHARSET_REG(ucs2,145),
    CHARSET_REG(ucs2,146),
    CHARSET_REG(ucs2,147),
    CHARSET_REG(ucs2,148),
    CHARSET_REG(ucs2,149),
    CHARSET_REG(ucs2,150),
    CHARSET_REG(ucs2,151),
    CHARSET_REG(ucs2,159),
    CHARSET_REG(utf32,160),
    CHARSET_REG(utf32,161),
    CHARSET_REG(utf32,162),
    CHARSET_REG(utf32,163),
    CHARSET_REG(utf32,164),
    CHARSET_REG(utf32,165),
    CHARSET_REG(utf32,166),
    CHARSET_REG(utf32,167),
    CHARSET_REG(utf32,168),
    CHARSET_REG(utf32,169),
    CHARSET_REG(utf32,170),
    CHARSET_REG(utf32,171),
    CHARSET_REG(utf32,172),
    CHARSET_REG(utf32,173),
    CHARSET_REG(utf32,174),
    CHARSET_REG(utf32,175),
    CHARSET_REG(utf32,176),
    CHARSET_REG(utf32,177),
    CHARSET_REG(utf32,178),
    CHARSET_REG(utf32,179),
    CHARSET_REG(utf32,180),
    CHARSET_REG(utf32,181),
    CHARSET_REG(utf32,182),
    CHARSET_REG(utf32,183),
    CHARSET_REG(utf8,192),
    CHARSET_REG(utf8,193),
    CHARSET_REG(utf8,194),
    CHARSET_REG(utf8,195),
    CHARSET_REG(utf8,196),
    CHARSET_REG(utf8,197),
    CHARSET_REG(utf8,198),
    CHARSET_REG(utf8,199),
    CHARSET_REG(utf8,200),
    CHARSET_REG(utf8,201),
    CHARSET_REG(utf8,202),
    CHARSET_REG(utf8,203),
    CHARSET_REG(utf8,204),
    CHARSET_REG(utf8,205),
    CHARSET_REG(utf8,206),
    CHARSET_REG(utf8,207),
    CHARSET_REG(utf8,208),
    CHARSET_REG(utf8,209),
    CHARSET_REG(utf8,210),
    CHARSET_REG(utf8,211),
    CHARSET_REG(utf8,212),
    CHARSET_REG(utf8,213),
    CHARSET_REG(utf8,214),
    CHARSET_REG(utf8,215),
    CHARSET_REG(utf8,223),
    CHARSET_REG(utf8mb4,224),
    CHARSET_REG(utf8mb4,225),
    CHARSET_REG(utf8mb4,226),
    CHARSET_REG(utf8mb4,227),
    CHARSET_REG(utf8mb4,228),
    CHARSET_REG(utf8mb4,229),
    CHARSET_REG(utf8mb4,230),
    CHARSET_REG(utf8mb4,231),
    CHARSET_REG(utf8mb4,232),
    CHARSET_REG(utf8mb4,233),
    CHARSET_REG(utf8mb4,234),
    CHARSET_REG(utf8mb4,235),
    CHARSET_REG(utf8mb4,236),
    CHARSET_REG(utf8mb4,237),
    CHARSET_REG(utf8mb4,238),
    CHARSET_REG(utf8mb4,239),
    CHARSET_REG(utf8mb4,240),
    CHARSET_REG(utf8mb4,241),
    CHARSET_REG(utf8mb4,242),
    CHARSET_REG(utf8mb4,243),
    CHARSET_REG(utf8mb4,244),
    CHARSET_REG(utf8mb4,245),
    CHARSET_REG(utf8mb4,246),
    CHARSET_REG(utf8mb4,247),
    CHARSET_REG(gb18030,248),
    CHARSET_REG(gb18030,249),
    CHARSET_REG(gb18030,250),
    CHARSET_REG(utf8mb4,255),
    {NULL, 0, 0}
};

uint8_t charset_to_id(const char* name) {
    auto hash = StringUtils::Hash3(name);
    CharsetReg* reg;
    for (reg = s_charset_map; reg->name; ++reg) {
        if (reg->hash == hash) {
            return reg->id;
        }
    }
    return 0;
}

const char* charset_to_name(uint8_t id) {
    CharsetReg* reg;
    for (reg = s_charset_map; reg->name; ++reg) {
        if (reg->id == id) {
            return reg->name;
        }
    }
    return "unknown";
}

}