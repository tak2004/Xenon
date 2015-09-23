#include "Xenon/xenon.hpp"
#include "Xenon/dds.hpp"

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