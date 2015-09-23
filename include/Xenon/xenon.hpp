#ifndef XENON_HPP
#define XENON_HPP

#include <RadonFramework/Radon.hpp>

namespace Xenon {

class DDSProcessor
{
public:
    /// Accept a memory block which contains a dds file.
    RF_Type::Bool Load(RF_Mem::AutoPointerArray<RF_Type::UInt8>& Data);

    /// Process the loaded data and generate an binary output.
    void Process();

    /// Return a memory block which contains the processed data.
    const RF_Mem::AutoPointerArray<RF_Type::UInt8>& GetOutput()const;

    RF_Type::Size InputSize()const;
    RF_Type::Size OutputSize()const;
    RF_Type::Float32 CompressionRatio()const;
    RF_Type::Float32 Percentage()const;
protected:
    RF_Mem::AutoPointerArray<RF_Type::UInt8> m_Input;
    RF_Mem::AutoPointerArray<RF_Type::UInt8> m_Output;
};

}

#endif // XENON_HPP