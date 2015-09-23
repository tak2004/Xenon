#ifndef XENON_DDS_HPP
#define XENON_DDS_HPP

#include <RadonFramework/Radon.hpp>

namespace Xenon {

struct DDS
{
    enum { FOURCC = 0x20534444};
    enum { DDPF_FOURCC = 0x4};
    enum { DXT5 = '5TXD',
        DXT3 = '3TXD',
        DXT1 = '1TXD'
    };
};

struct DDS_PIXELFORMAT
{
    RF_Type::UInt32 dwSize;
    RF_Type::UInt32 dwFlags;
    RF_Type::UInt32 dwFourCC;
    RF_Type::UInt32 dwRGBBitCount;
    RF_Type::UInt32 dwRBitMask;
    RF_Type::UInt32 dwGBitMask;
    RF_Type::UInt32 dwBBitMask;
    RF_Type::UInt32 dwABitMask;
};
static_assert(sizeof(DDS_PIXELFORMAT) == 32, "DDS_HEADER must be 128 byte large!");

struct DDS_HEADER
{
    RF_Type::UInt32 FOURCC;
    RF_Type::UInt32 dwSize;
    RF_Type::UInt32 dwFlags;
    RF_Type::UInt32 dwHeight;
    RF_Type::UInt32 dwWidth;
    RF_Type::UInt32 dwPitchLinear;
    RF_Type::UInt32 dwDepth;
    RF_Type::UInt32 dwMipMapCount;
    RF_Type::UInt32 reserved1[11];
    DDS_PIXELFORMAT ddpfPixelFormat;
    RF_Type::UInt32 ddsCaps[4];
    RF_Type::UInt32 reserved2;
};
static_assert(sizeof(DDS_HEADER) == 128, "DDS_HEADER must be 128 byte large!");

struct AlphaIndices
{
    RF_Type::UInt8 bits[6];
};

struct DDS_DXT5
{
    RF_Type::UInt8 a0;
    RF_Type::UInt8 a1;
    AlphaIndices aBits;
    RF_Type::UInt16 c0;
    RF_Type::UInt16 c1;
    RF_Type::UInt32 cBits;
};
static_assert(sizeof(DDS_DXT5) == 16, "DDS_DXT5_YCgCo must be 64 byte large!");

}

#endif // XENON_DDS_HPP