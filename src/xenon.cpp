#include "Xenon/xenon.hpp"
#include "Xenon/dds.hpp"
#include <encode.h>

namespace Xenon {

RF_Type::Bool DDSProcessor::Load(RF_Mem::AutoPointerArray<RF_Type::UInt8>& Data)
{
    RF_Type::Bool result = false;
    m_Input = Data;

    DDS_HEADER* ddsHeader = reinterpret_cast<DDS_HEADER*>(m_Input.Get());
    if(ddsHeader->FOURCC == DDS::FOURCC &&
       ddsHeader->ddpfPixelFormat.dwFlags & DDS::DDPF_FOURCC &&
       (ddsHeader->ddpfPixelFormat.dwFourCC == DDS::DXT5))
    {
        result = true;
    }

    return result;
}


void DeInterleave_DXT5(const DDS_DXT5* ddsData, const RF_Type::Size Blocks, 
    RF_Collect::Array<RF_Mem::AutoPointerArray<RF_Type::UInt8> >& Buffers)
{
    struct Channel
    {
        void Init(RF_Type::UInt8* Data, RF_Type::Size Offset, RF_Type::Size ASize, 
                  const DDS_DXT5* DDS, RF_Type::Size Blocks)
        {
            data = Data; offset = Offset; size = ASize; dds = DDS; blocks = Blocks;
        }
        RF_Type::UInt8* data; RF_Type::Size offset, size; const DDS_DXT5* dds; RF_Type::Size blocks;
    };

    RF_Collect::Array<Channel> channels(6);
    channels(0).Init(Buffers(0).Get(), 0, 1, ddsData, Blocks);
    channels(1).Init(Buffers(1).Get(), 1, 1, ddsData, Blocks);
    channels(2).Init(Buffers(11).Get(), 2, 6, ddsData, Blocks);
    channels(3).Init(Buffers(8).Get(), 8, 2, ddsData, Blocks);
    channels(4).Init(Buffers(9).Get(), 10, 2, ddsData, Blocks);
    channels(5).Init(Buffers(10).Get(), 12, 4, ddsData, Blocks);

    RF_Algo::ForEach(channels, [](RF_Collect::Array<Channel>::EnumeratorType& Enum) {
        const RF_Type::UInt8* blockIn = reinterpret_cast<const RF_Type::UInt8*>(Enum->dds);
        for(RF_Type::Size i = 0; i < Enum->blocks; ++i)
        {
            RF_SysMem::Copy(Enum->data + i * Enum->size, blockIn + Enum->offset, Enum->size);
            blockIn += 16;
        }
    });
}

void DeInterleave(RF_Collect::Array<RF_Mem::AutoPointerArray<RF_Type::UInt8> >& Buffers,
    RF_Type::Size Elements)
{
    struct Channel
    {
        void Init(const RF_Type::UInt8* Data, RF_Type::Size Offset, RF_Type::Size Bits,
                  RF_Type::Size Blocks, RF_Type::UInt8* Out)
        {
            data = Data; offset = Offset; bits = Bits; blocks = Blocks; out = Out;
        }
        const RF_Type::UInt8* data; RF_Type::Size offset, bits; RF_Type::Size blocks;
        RF_Type::UInt8* out;
    };

    RF_Collect::Array<Channel> channels(6);
    channels(0).Init(Buffers(8).Get(), 5, 0, Elements, Buffers(2).Get());
    channels(1).Init(Buffers(8).Get(), 6, 5, Elements, Buffers(3).Get());
    channels(2).Init(Buffers(8).Get(), 5, 11, Elements, Buffers(4).Get());
    channels(3).Init(Buffers(9).Get(), 5, 0, Elements, Buffers(5).Get());
    channels(4).Init(Buffers(9).Get(), 6, 5, Elements, Buffers(6).Get());
    channels(5).Init(Buffers(9).Get(), 5, 11, Elements, Buffers(7).Get());

    RF_Algo::ForEach(channels, [](RF_Collect::Array<Channel>::EnumeratorType& Enum) {
        const RF_Type::UInt8* blockIn = Enum->data;
        for(RF_Type::Size i = 0; i < Enum->blocks; ++i)
        {
            Enum->out[i] = static_cast<RF_Type::UInt8>(reinterpret_cast<const RF_Type::UInt16*>(blockIn)[i] >> Enum->offset) & (0xff >> (8 - Enum->bits));
        }
    });
}

int memcmp_signed;

int unsigned_memcmp(void *p1, void *p2, unsigned int i)
{
    unsigned char *pc1 = (unsigned char *)p1;
    unsigned char *pc2 = (unsigned char *)p2;
    while(i--) {
        if(*pc1 < *pc2)
            return -1;
        else if(*pc1++ > *pc2++)
            return 1;
    }
    return 0;
}

int bounded_compare(const unsigned int *i1, const unsigned int *i2)
{
    static int ticker = 0;

    unsigned int l1 = (unsigned int)(length - *i1);
    unsigned int l2 = (unsigned int)(length - *i2);
    int result;
    if(memcmp_signed)
        result = unsigned_memcmp(buffer + *i1,
        buffer + *i2,
        l1 < l2 ? l1 : l2);
    else
        result = memcmp(buffer + *i1,
        buffer + *i2,
        l1 < l2 ? l1 : l2);
    if(result == 0)
        return l2 - l1;
    else
        return result;
};

void BurrowWheeler(RF_Collect::Array<RF_Mem::AutoPointerArray<RF_Type::UInt8> >& Buffers,
                  RF_Type::Size Elements)
{
    static RF_Type::UInt32 BLOCK_SIZE = 200000;

    struct Channel
    {
        void Init(RF_Type::UInt8* Data, RF_Type::Size Blocks)
        {
            data = Data;
            blocks = Blocks;
        }
        RF_Type::UInt8* data; RF_Type::Size blocks;
    };

    RF_Collect::Array<Channel> channels(10);
    channels(0).Init(Buffers(2).Get(), Elements);
    channels(1).Init(Buffers(3).Get(), Elements);
    channels(2).Init(Buffers(4).Get(), Elements);
    channels(3).Init(Buffers(5).Get(), Elements);
    channels(4).Init(Buffers(6).Get(), Elements);
    channels(5).Init(Buffers(7).Get(), Elements);
    channels(6).Init(Buffers(0).Get(), Elements);
    channels(7).Init(Buffers(1).Get(), Elements);
    channels(8).Init(Buffers(10).Get(), Elements);
    channels(9).Init(Buffers(11).Get(), Elements);

    RF_Algo::ForEach(channels, [](RF_Collect::Array<Channel>::EnumeratorType& Enum) {
        RF_Type::Int32 indices[BLOCK_SIZE+1];
        RF_Type::UInt8* blockIn = Enum->data;
        RF_Type::Size rounds = ((Enum->blocks - 1) / BLOCK_SIZE) + 1;
        for(RF_Type::Size i = 0; i < rounds; ++i)
        {
            RF_Type::Size length = BLOCK_SIZE;
            if(i == rounds-1)
                length = Enum->blocks % BLOCK_SIZE;
            for (RF_Type::Size j = 0; j <= length; ++j)
            {
                indices[j] = j;
            }
            qsort(indices, length +1, 4, bounded_compare);

            RF_Type::UInt32 first, last;
            for (j = 0;j <= length; ++j)
            {
                if(indices[j]==1)
                    first = j;
                if(indices[j]==0)
                    last = j;
            }
        }
    });
}

void DDSProcessor::Process()
{
    RF_Collect::Array<RF_Mem::AutoPointerArray<RF_Type::UInt8> > buffers(12);
    RF_Type::Size dataSize = m_Input.Size() - sizeof(DDS_HEADER);
    RF_Type::Size blocks = dataSize / sizeof(DDS_DXT5);

    for(RF_Type::Size i = 0; i < 8; i++)
    {
        buffers(i) = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    }
    buffers(8) = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(RF_Type::UInt16)*blocks);
    buffers(9) = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(RF_Type::UInt16)*blocks);
    buffers(10) = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(RF_Type::UInt32)*blocks);
    buffers(11) = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(AlphaIndices)*blocks);

    DDS_HEADER* ddsHeader = reinterpret_cast<DDS_HEADER*>(m_Input.Get());
    DDS_DXT5* ddsData = reinterpret_cast<DDS_DXT5*>(ddsHeader + 1);

    DeInterleave_DXT5(ddsData, blocks, buffers);
    DeInterleave(buffers, blocks);
    DyadicDecomposition(buffers, blocks);
    
    RF_Type::Size size = sizeof(DDS_HEADER);
    brotli::BrotliParams param;

    auto r0 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    RF_Type::Size bufSize= r0.Count();    
    if (brotli::BrotliCompressBuffer(param, buffers(2).Count(), buffers(2).Get(), &bufSize, r0.Get()))
        size += bufSize;
    
    auto g0 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = g0.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(3).Count(), buffers(3).Get(), &bufSize, g0.Get()))
        size += bufSize;

    auto b0 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = b0.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(4).Count(), buffers(4).Get(), &bufSize, b0.Get()))
        size += bufSize;

    auto r1 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = r1.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(5).Count(), buffers(5).Get(), &bufSize, r1.Get()))
        size += bufSize;

    auto g1 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = g1.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(6).Count(), buffers(6).Get(), &bufSize, g1.Get()))
        size += bufSize;

    auto b1 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = b1.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(7).Count(), buffers(7).Get(), &bufSize, b1.Get()))
        size += bufSize;

    auto a0 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = a0.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(0).Count(), buffers(0).Get(), &bufSize, a0.Get()))
        size += bufSize;

    auto a1 = RF_Mem::AutoPointerArray<RF_Type::UInt8>(blocks);
    bufSize = a1.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(1).Count(), buffers(1).Get(), &bufSize, a1.Get()))
        size += bufSize;

    auto cbits = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(RF_Type::UInt32)*blocks);
    bufSize = cbits.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(10).Count(), buffers(10).Get(), &bufSize, cbits.Get()))
        size += bufSize;

    auto abits = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(AlphaIndices)*blocks);
    bufSize = abits.Count();
    if(brotli::BrotliCompressBuffer(param, buffers(11).Count(), buffers(11).Get(), &bufSize, abits.Get()))
        size += bufSize;

    m_Output = RF_Mem::AutoPointerArray<RF_Type::UInt8>(sizeof(DDS_HEADER));
    RF_SysMem::Copy(m_Output.Get(), ddsHeader, sizeof(ddsHeader));
}

const RF_Mem::AutoPointerArray<RF_Type::UInt8>& DDSProcessor::GetOutput() const
{
    return m_Output;
}

RF_Type::Size DDSProcessor::InputSize() const
{
    return m_Input.Count();
}

RF_Type::Size DDSProcessor::OutputSize() const
{
    return m_Output.Count();
}

RF_Type::Float32 DDSProcessor::CompressionRatio() const
{
    return static_cast<RF_Type::Float32>(m_Output.Count()) /
        static_cast<RF_Type::Float32>(m_Input.Count());
}

RF_Type::Float32 DDSProcessor::Percentage() const
{
    return 100.0f-((static_cast<RF_Type::Float32>(m_Output.Count()) /
            static_cast<RF_Type::Float32>(m_Input.Count())) * 100.0f);
}

}