#include <RadonFramework/Radon.hpp>
#include <Xenon/xenon.hpp>

enum ApplicationOptions: RF_Type::Size
{
    ApplicationDirectory = 0,
    InputFile,
    OutputFile,
    MAX
};

void main(int argc, const char* argv[])
{
    RadonFramework::Radon radon;
    RF_Mem::AutoPointer<RF_Diag::Appender> console(new RF_IO::LogConsole);
    RF_IO::Log::AddAppender(console);

#ifdef RF_DEBUG
    RF_Mem::AutoPointer<RF_Diag::Appender> debugger(new RF_IO::LogDebuggerOutput);
    RF_IO::Log::AddAppender(debugger);
#endif // RF_DEBUG

    RF_Mem::AutoPointerArray<RF_IO::OptionRule> rules(ApplicationOptions::MAX);
    rules[ApplicationOptions::ApplicationDirectory].Init(0, 0,
        RF_IO::StandardRuleChecker::Text, 0, RF_IO::OptionRule::Required);

    rules[ApplicationOptions::InputFile].Init("-i", "--filein",
        RF_IO::StandardRuleChecker::Text, "Input file name.",
        RF_IO::OptionRule::Required);

    rules[ApplicationOptions::OutputFile].Init("-o", "--fileout",
        RF_IO::StandardRuleChecker::Text, "Output directory",
        RF_IO::OptionRule::Required);

    RF_Type::String outputDirectory;
    RF_IO::Parameter parameters;
    if(parameters.ParsingWithLogging(argv, argc, rules))
    {
        Xenon::DDSProcessor processor;
        RF_IO::File inputFile;
        inputFile.SetLocation(RF_Type::String("file:///")+parameters.Values()[ApplicationOptions::InputFile].Value());
        if(inputFile.Exists())
        {
            if(processor.Load(inputFile.Read()))
            {
                processor.Process();
                RF_IO::File outputFile;
                outputFile.SetLocation(RF_Type::String("file:///")+parameters.Values()[ApplicationOptions::OutputFile].Value());

                RF_IO::Directory outputFolder;
                outputFolder.SetLocation(RF_Type::String("file:///") + outputFile.Path());
                if(!outputFolder.Exists())
                    outputFolder.CreateNewDirectory();

                outputFile.CreateNewFile();
                outputFile.Write(processor.GetOutput());

                RF_IO::LogInfo("DXT(%u) vs Xenon(%u) ratio: %f(%.2f%%)", processor.InputSize(),
                    processor.OutputSize(), processor.CompressionRatio(), processor.Percentage());
            }
        }
    }

    RF_Pattern::Singleton<RF_Thread::ThreadPool>::GetInstance().DisableAndWaitTillDone();
}