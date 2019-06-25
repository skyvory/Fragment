// Copyright (C) 2016 by rr-
//
// This file is part of arc_unpacker.
//
// arc_unpacker is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// arc_unpacker is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with arc_unpacker. If not, see <http://www.gnu.org/licenses/>.

#include "dec/glib/glib2/musume.h"
#include "algo/format.h"
#include "algo/range.h"
#include "err.h"

using namespace au;
using namespace au::dec::glib::glib2;

static const std::array<size_t, 4> permutations[6] =
{
    {3, 2, 1, 0},
    {0, 2, 1, 3},
    {1, 0, 3, 2},
    {3, 0, 2, 1},
    {2, 1, 3, 0},
    {3, 2, 1, 0},
};

static const std::function<u8(u8, size_t)> funcs[] =
{
    [](u8 byte, size_t acc)
    {
        return (byte >> (acc & 7)) | (byte << (8 - (acc & 7)));
    },
    [](u8 byte, size_t acc) { return byte ^ acc; },
    [](u8 byte, size_t acc) { return byte ^ 0xFF; },
    [](u8 byte, size_t acc) { return (byte - 0x64) ^ 0xFF; },
    [](u8 byte, size_t acc) { return byte + acc; },
    [](u8 byte, size_t acc) { return (byte << 4) | (byte >> 4); },
};

static const std::vector<u16> decoder_table
{
    0x0DF0,0xB0B7,0x45B8,0xC9B4,0xCFB3,0xF0B0,0xA85F,0x2C0B,0x648D,0xD4E1,
    0x11EA,0xDAA7,0xFB5F,0xE83A,0x82A4,0x0F5D,0xAE64,0x6F6F,0xEC17,0x040E,
    0x25F2,0xD2A1,0x3F4E,0x9EB3,0xA3C3,0x126C,0xCBE3,0x8597,0x6E19,0xA4E5,
    0x644E,0xBD12,0x43FF,0x635D,0xB5FB,0x8B1E,0x7840,0xFFB8,0x6385,0x8C22,
    0x2D2F,0x479B,0x185C,0x1970,0x0C73,0xE029,0xE04D,0xF855,0xA1F2,0xDCE5,
    0x464B,0x997A,0x50F3,0xC59D,0x4FCF,0x4434,0x03E6,0x4440,0xB794,0xAD98,
    0xC4D3,0x851C,0x7643,0x0824,0x624B,0xF308,0x8BC9,0x8FCD,0x0805,0x28C6,
    0x7FDA,0x7171,0xC93C,0x3F85,0xE0B9,0x64A8,0xBF46,0x5652,0xEBAA,0x9507,
    0x4DF4,0x692C,0x773D,0x65CF,0x2298,0x7D43,0xC1F2,0x2DE8,0x6758,0xEA01,
    0x8B65,0xE7FE,0x5466,0x8F59,0x44B2,0x1CCB,0x92DA,0xB384,0xC5C4,0x5179,
    0xDC2E,0xBC2A,0xA07D,0x74D5,0x3492,0x0BF6,0xC3F5,0xB68C,0xAABB,0x114E,
    0xB7B8,0x6636,0xA419,0xF92F,0xE2B4,0x273B,0xCC0B,0x4F64,0x2EFE,0xBF77,
    0xD00A,0x4FA9,0x1AFF,0x5930,0xC18B,0x80BD,0x22D2,0xD8D2,0x8B8C,0x1F8C,
    0x6B41,0x27A1,0xE38A,0xF328,0x910B,0x6D91,0x81A2,0xC5AE,0xE7B9,0x98F7,
    0x044D,0x7851,0xCF13,0x5EE3,0x8C5A,0x5D13,0xE3A9,0x5599,0x60BE,0x0775,
    0x81AC,0x63C8,0x80A1,0x1EAB,0xC3EB,0x2847,0x29C6,0x5D02,0xB5DB,0xDA13,
    0xC3DB,0x8B4C,0x0347,0x89B1,0x8743,0x3407,0x9870,0xA97D,0xBC0F,0x2452,
    0xE7B5,0x1016,0x1759,0xE219,0x41FE,0x6BEE,0x2EB6,0xAE15,0x9010,0xD486,
    0x7C58,0xBD7F,0xBDC9,0x5385,0xC1DF,0x55FD,0x1543,0x9DD2,0xDF04,0x561C,
    0xB184,0x0E81,0xB145,0x23D7,0xBBA8,0x4C08,0xC6DC,0x68FE,0xB6FC,0xA2E1,
    0x49C2,0x4C6E,0x8474,0x3FBC,0x4059,0xE75C,0xF748,0x2542,0x3E89,0x1932,
    0xAEC3,0x1A71,0x3229,0x5F8F,0xDF64,0x34EA,0xB7AF,0xBEA4,0x5129,0xBA1B,
    0x00A0,0x0731,0x3F10,0x1B52,0xD0D6,0xCB01,0x0CD2,0xC522,0xAAF5,0x3B34,
    0x7EEA,0x1FE8,0xAE3B,0x144D,0xCA14,0x3CD4,0x9AFB,0x673A,0x3B41,0x5EA0,
    0x8A5F,0x0289,0x7DDC,0x9FBB,0x75DC,0x59D3,0x4907,0x83D5,0x747C,0xFC0D,
    0x650B,0xBA84,0xF6CF,0x21AF,0xFF7E,0xACBA,0xC8E1,0x4DBB,0xCBD9,0x3415,
    0x2BBB,0x6DA3,0x349E,0xD30B,0xCEDB,0x739E,0xE0F3,0xCB44,0x7E1F,0x274B,
    0x3FCE,0xEEC8,0x4306,0x7EC3,0x4CBF,0x766B,0x582C,0x47D1,0x688E,0x15BC,
    0xEEEA,0xD2B8,0x8674,0x088B,0xE90E,0x3A3F,0xF70A,0x2393,0x68B3,0x871E,
    0xF6FC,0x4631,0xFEAD,0x89B5,0x1242,0xD68C,0x1214,0x028A,0xE3F3,0xBC86,
    0x679E,0xB301,0x2827,0xC90F,0xA4DD,0x0E14,0x4225,0x46EF,0x0C69,0x12BB,
    0xA14B,0x4319,0x5054,0x1BBE,0x1144,0x29B8,0x74E7,0x94C8,0xA217,0x2001,
    0xE86F,0x01B3,0x4713,0x737A,0x45B2,0x68BB,0x86FD,0x696D,0xB604,0x0D2E,
    0xC643,0x6F65,0x6EC4,0xCBC0,0xE505,0x0677,0x6427,0x6481,0xB195,0xE877,
    0x4216,0xD938,0xD165,0xBB70,0x9980,0x1F0A,0xA96C,0x1DB9,0x7B0A,0xB400,
    0xC636,0x17EA,0x129A,0x46F8,0x40DC,0xEDC0,0x7BC5,0x4521,0x15E5,0xB0F5,
    0x4E1D,0xD7AF,0xF85C,0xF100,0x490E,0x74E6,0x9D7E,0x112D,0x5063,0x6704,
    0xE9B9,0xA519,0xA626,0x0CBC,0x5880,0x8224,0xF706,0x2A08,0x317B,0x2E86,
    0x46CF,0xFE7E,0x9D15,0xBC5E,0x35FC,0x9BCD,0xA1B0,0x9191,0xFBFC,0x54D9,
    0xE1B3,0xD381,0xE3DA,0x41BD,0xD27D,0x9B0C,0xBAB2,0xC6FA,0xB910,0xD4C1,
    0xAB62,0x756D,0xD4FF,0xE1C4,0xC052,0x32E2,0x6063,0x6CDF,0x8BD6,0xBCE0,
    0x8026,0x63E8,0x2201,0x9800,0x5572,0x674C,0xDD3B,0x1E95,0xE9EF,0x698E,
    0xF509,0xA410,0xCA5F,0xD175,0xB525,0xAD0F,0x4E44,0x3E58,0x554E,0x97B3,
    0x50F5,0x8BAC,0x5A32,0x77AA,0xD14F,0x3ED8,0x7892,0x5A02,0x0E66,0x8269,
    0x120A,0xADEA,0x82A1,0xE1F0,0x8B35,0xE7E1,0xFD1F,0xFD65,0x7E3A,0xB4B7,
    0x5FFC,0xF75A,0xB2DB,0x5EEA,0xE63E,0x13C7,0xB5F9,0x02B9,0x9F2E,0x620A,
    0x2108,0x10BB,0xB124,0x6617,0x774D,0x49A5,0x93CD,0xEDEC,0xA50B,0x7996,
    0x8E82,0x1782,0x87A5,0x1D08,0x8C23,0x7208,0x5FD5,0x3FFC,0xE23C,0x182C,
    0x0338,0xA32B,0x9C08,0x907C,0xDF0E,0xAF61,0x5613,0xACB7,0x8AA9,0xF1BA,
    0x6AC2,0xCAD5,0xED60,0x2C74,0xA172,0x4DC4,0x30CE,0x02AB,0x3C3C,0x260E,
    0xAEDD,0xCD27,0x8D18,0x8E4E,0x94CD,0x8A82,0x573A,0x84A8,0x5B10,0x8E93,
    0x1DB2,0x8D7E,0xB892,0xCD2E,0x4D24,0xC8FD,0x571D,0x978E,0x94E2,0x7471,
    0x288C,0x8722,0x7B92,0x48ED,0x2583,0xBE7F,0x0D86,0x347B,0x90EE,0x955F,
    0x6277,0x64E3,0xDAFE,0xBC9B,0x96CE,0xEBE4,0xEFBA,0x6C19,0x42A8,0xC2DE,
    0xA5FD,0x68BD,0xD469,0x5511,0x6C69,0x671E,0x2E77,0xCF3B,0x2052,0x88EC,
    0x6577,0xED28,0xA811,0xB7F5,0x213F,0x1F24,0xEEB5,0x9511,0xC44D,0x6532,
    0xD969,0xAA6B,0xF7E7,0xC645,0xC868,0xB65B,0x0029,0xEAD2,0x58A4,0x0153,
    0x9E90,0x6E05,0xDC61,0x2F9B,0x8CD4,0xCC13,0x5621,0x056C,0xC7A3,0xD666,
    0x40CC,0x4ED9,0x6071,0x0E42,0x23E5,0xE18F,0x1126,0x4889,0x2AF2,0xACBB,
    0x2AD3,0xDC2B,0x54E8,0x1478,0x9FF3,0x9DD0,0x63BB,0x0430,0xE5D4,0x91C9,
    0xB055,0xE925,0x6292,0x6228,0x14DE,0x4D80,0x962A,0x801E,0x1576,0x2D36,
    0xE3C3,0x6F7F,0x07EA,0xAA9F,0xCAE9,0x8F76,0xD1CB,0xE490,0x6956,0x98CA,
    0x4DB0,0x922E,0x3E05,0x1538,0xFFA3,0x4A24,0x0467,0x7940,0x7B14,0xD4E8,
    0xA9E8,0xF9D2,0xDDD3,0xCEF2,0x3A55,0xF118,0xD0F2,0x5688,0xE2BE,0xC0ED,
    0xFED3,0x33B1,0xAC96,0xA6F2,0x0F0B,0xE596,0x0A88,0xB346,0x2208,0x18A1,
    0xAE40,0x7ECC,0xA2E4,0xB661,0xBDEE,0xC14C,0xD218,0x8280,0x7F61,0x8FA3,
    0x4AA3,0xA61B,0xDD56,0xB4AD,0xE436,0x5AF9,0xE595,0x8603,0x6FF3,0xBF3F,
    0x2FCD,0xD446,0x475A,0x508F,0xC27E,0x6096,0x9A42,0x19C4,0x6109,0xEC7A,
    0x5937,0x218F,0x6FFF,0x2033,0x922C,0x5C66,0xC20D,0xE424,0xF193,0x159D,
    0xCF98,0x7AE0,0x79BA,0xF8AC,0x0927,0x0BAC,0xE3F4,0x26A7,0x1C7C,0x8D05,
    0x5A6A,0x6A85,0x6947,0xEB07,0x03F4,0x166F,0x8DE4,0xFB48,0xF828,0x3977,
    0x5A67,0x4D08,0x838D,0xCBB8,0xD737,0x0EFE,0xED83,0xF808,0xB4AC,0x721E,
    0x1F51,0xD55C,0xD2E0,0xE243,0x301F,0x4630,0x26D7,0x3D02,0xC71E,0x14F0,
    0xE56A,0xABC7,0x26B7,0x1609,0x1A11,0xADA1,0xCB41,0x6C4F,0x84F2,0xB45D,
    0x1CE3,0x70A3,0x9661,0x4374,0x6C5A,0xE76C,0x7333,0xE26D,0xEFA8,0xF66A,
    0x6E77,0xF1CC,0xB7E8,0x0433,0x96F6,0x8779,0x8181,0xF982,0xD352,0x6338,
    0x89E9,0x9C75,0x30F7,0x4B4A,0x2B4E,0xE942,0x1A26,0xB237,0xD4F4,0x5300,
    0xC4D0,0x58EF,0xA81D,0xD733,0x1A6C,0x8096,0xA37C,0x9B5B,0x390D,0xE007,
    0xED2F,0x8287,0xA549,0xB63B,0xA3E4,0xF115,0xCD68,0xF39A,0xD125,0x3F16,
    0x47CD,0x582E,0x508D,0xED45,0x9073,0x60EF,0x2591,0xF962,0x7C2E,0xADB1,
    0x2FB5,0x376F,0x4F41,0xA24A,0x2AAA,0xFA2B,0x5FA6,0xADCF,0x1644,0x575E,
    0x8583,0x055D,0x2AE7,0xA515,0x2700,0x5FC3,0x6001,0xCB0E,0x1351,0x7C19,
    0xBFD3,0xF360,0x706C,0x1227,0x6411,0x3649,0xF843,0x7CFE,0x5DC3,0x640D,
    0x7E05,0x251C,0x8EDF,0x1F31,0x39F8,0x075B,0x3BBA,0x75B8,0x4CBC,0xAA60,
    0xE1D4,0x35C9,0x3516,0xB0D0,0x7B3B,0xF861,0x6B87,0x3362,0x1704,0x7F1C,
    0x587B,0x966B,0xD7DF,0xBE5D,0x82EF,0xE193,0xD841,0xBC82,0x9AF9,0x8526,
    0x7018,0x8E13,0x5E23,0xF49D,0x4035,0x9118,0x2C41,0x826C,0x561A,0x5E7B,
    0x38D4,0xC263,0x5979,0xB15A,0x4D89,0xC11C,0x8516,0x0343,0xF590,0xFF38,
    0x8385,0x4B5B,0xEEF2,0x99B4,0xB11B,0x94D6,0x8961,0xE9F9,0x3138,0xB3A0,
    0x09CC,0x7A4F,0x47DE,0xE478,0xD9FF,0xE62C,0x9453,0x6D0C,0x2FC2,0x7444,
};

std::unique_ptr<Decoder> MusumePlugin::create_decoder(
    const std::array<u32, 4> &keys) const
{
    const u32 target = ((keys[0] * 95) >> 13) & 0xFFFF;
    int index = -1;
    for (const auto i : algo::range(decoder_table.size()))
        if (decoder_table[i] == target)
            index = i;
    if (index == -1)
    {
        throw err::NotSupportedError(
            algo::format("Unsupported key: %08x", keys[0]));
    }

    const auto tmp1 = index / 150;
    const auto tmp2 = index / 30;
    const auto src_permutation = tmp1;
    const auto dst_permutation
        = tmp2 - 5 * tmp1 - ((tmp2 - 5 * tmp1 < tmp1) - 1);
    const auto func1_index = (index / 5) % 6;
    const auto func2_index = index % 5 - ((index % 5 < func1_index) - 1);

    return std::unique_ptr<Decoder>(
        new Decoder
        {
            permutations[src_permutation],
            permutations[dst_permutation],
            funcs[func2_index],
            funcs[func1_index],
        });
}

std::unique_ptr<Decoder> MusumePlugin::create_header_decoder() const
{
    return create_decoder({0x8465B49B, 0x4D619A7B, 0x7365C6AD, 0x7CFD70A7});
}