#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>

#include <fmt\format.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/MemoryBufferCache.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Sema/Sema.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "aot_ops.hpp"
#include "font.hpp"
#include "llvm_aot.hpp"

constexpr auto EXECUTION_OFFSET = 0x200;
#define ADDR "{:#3X}"
#define BYTE "{:#2X}"
#define REG "{:#1X}"

// http://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/
void InitializeLLVM() {
    static bool LLVMinit = false;
    if (LLVMinit) {
        return;
    }
    // We have not initialized any pass managers for any device yet.
    // Run the global LLVM pass initialization functions.
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto& Registry = *llvm::PassRegistry::getPassRegistry();

    llvm::initializeCore(Registry);
    llvm::initializeScalarOpts(Registry);
    llvm::initializeVectorization(Registry);
    llvm::initializeIPO(Registry);
    llvm::initializeAnalysis(Registry);
    llvm::initializeTransformUtils(Registry);
    llvm::initializeInstCombine(Registry);
    llvm::initializeInstrumentation(Registry);
    llvm::initializeTarget(Registry);

    LLVMinit = true;
}

std::function<void()> Compile() {
    InitializeLLVM();

    auto diagnosticOptions = new clang::DiagnosticOptions();
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> ids;
    // presumably llvm will clean this up later...
    auto textDiagnosticPrinter = new clang::TextDiagnosticPrinter(llvm::outs(), diagnosticOptions);
    llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diagnosticsEngine(
        new clang::DiagnosticsEngine(ids, diagnosticOptions, textDiagnosticPrinter));

    clang::CompilerInstance compilerInstance;
    compilerInstance.createDiagnostics(textDiagnosticPrinter, false);
    auto& compilerInvocation = compilerInstance.getInvocation();
    std::string triple = llvm::sys::getProcessTriple();

    // clang seems to be ignoring the InputKind of the individual file
    compilerInvocation.setLangDefaults(
        *compilerInvocation.getLangOpts(), clang::InputKind::CXX, llvm::Triple(triple),
        compilerInvocation.getPreprocessorOpts(), clang::LangStandard::Kind::lang_gnucxx17);

    compilerInvocation.getFrontendOpts().Inputs = {
        clang::FrontendInputFile("source.cpp", clang::InputKind::CXX)};

    compilerInvocation.getTargetOpts().Triple = triple;
    auto& codeGenOptions = compilerInvocation.getCodeGenOpts();
    codeGenOptions.CodeModel = "large";
    codeGenOptions.ThreadModel = "posix";
    codeGenOptions.OptimizationLevel = 3;

    llvm::LLVMContext context;
    clang::EmitAssemblyAction action(&context);

    if (!compilerInstance.ExecuteAction(action)) {
        std::cout << "compilation failed";
        return nullptr;
    }

    std::unique_ptr<llvm::Module> module = action.takeModule();

    llvm::PassBuilder passBuilder;
    llvm::LoopAnalysisManager loopAnalysisManager(codeGenOptions.DebugPassManager);
    llvm::FunctionAnalysisManager functionAnalysisManager(codeGenOptions.DebugPassManager);
    llvm::CGSCCAnalysisManager cGSCCAnalysisManager(codeGenOptions.DebugPassManager);
    llvm::ModuleAnalysisManager moduleAnalysisManager(codeGenOptions.DebugPassManager);

    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cGSCCAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager,
                                     cGSCCAnalysisManager, moduleAnalysisManager);

    llvm::ModulePassManager modulePassManager =
        passBuilder.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O3);
    modulePassManager.run(*module, moduleAnalysisManager);

    llvm::EngineBuilder builder(std::move(module));
    builder.setMCJITMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
    builder.setOptLevel(llvm::CodeGenOpt::Level::Aggressive);
    auto executionEngine = builder.create();

    return reinterpret_cast<void (*)()>(executionEngine->getFunctionAddress("main"));
};

void LLVMAOT::Run(Chip8::Interface& interface, std::vector<std::uint8_t> game) {
    {
        // pass in the interface and a seed for RND
        source_builder << fmt::format(
            R"(class Interface;
Interface& interface = *reinterpret_cast<Interface*>({:p});
static constexpr unsigned int seed = )" BYTE ";\n",
            reinterpret_cast<void*>(&interface),
            std::uint32_t(std::chrono::system_clock::now().time_since_epoch().count()));

        // pass in game data
        source_builder << "static constexpr unsigned char game[]{";
        for (auto byte : game)
            source_builder << fmt::format(BYTE ",", byte);
        source_builder << "};";

        // include opcode definitions
        source_builder << AOT_OPS;

        source_builder << R"(
int main(){
    using namespace Opcodes;

    for (auto i = 0; i < sizeof(FONT); i++)
	    memory[i] = FONT[i];
    for (auto i = 0; i < sizeof(game); i++)
	    memory[0x200 + i] = game[i];

)";
        // create the jump table
        source_builder << "static constexpr void* jump_table[]{";
        for (auto i = 0; i < EXECUTION_OFFSET; i++)
            source_builder << "nullptr,";
        for (auto i = EXECUTION_OFFSET; i < game.size() + EXECUTION_OFFSET; i += 2)
            source_builder << fmt::format("&&l{:3X},nullptr,", i);
        source_builder << "};\n";

        // generate C++ from game code
        for (auto pos = game.begin(); pos < game.end(); pos += 2, program_counter += 2) {
            source_builder << fmt::format("l{:3X}: ", program_counter);
            opcode = *pos << 8 | *(pos + 1);
            (this->*opcode_table[op()])();
            source_builder << "\n";
        }
        source_builder << "    return 0;\n}";

        std::ofstream source_file("source.cpp", std::ios::out);
        source_file << source_builder.str();
    }

    const auto main = Compile();
    if (main)
        main();
    else
        std::cout << "function not found";
}

// TODO: rewrite with function-like macros
#define c ", "
#define IMM "_Vx_byte<" REG c BYTE ">();", X(), kk()
#define REGS "_Vx_Vy<" REG c REG ">();", X(), Y()
#define ONE_REG "_Vx<" REG ">();", X()

void LLVMAOT::NOOP() {}

void LLVMAOT::split_0() {
    (this->*opcode_table_0[kk()])();
}

void LLVMAOT::CLS() {
    source_builder << "CLS();";
}

void LLVMAOT::RET() {
    source_builder << fmt::format("RET(" ADDR ");", program_counter);
}

void LLVMAOT::JP_addr() {
    source_builder << fmt::format("JP_addr(" ADDR c ADDR ");", program_counter, nnn());
}

void LLVMAOT::CALL_addr() {
    source_builder << fmt::format("CALL_addr(" ADDR c ADDR ");", program_counter, nnn());
}

void LLVMAOT::SE_Vx_byte() {
    source_builder << fmt::format("SE_Vx_byte(" ADDR c REG c BYTE ");", program_counter, X(), kk());
}

void LLVMAOT::SNE_Vx_byte() {
    source_builder << fmt::format("SNE_Vx_byte(" ADDR c REG c BYTE ");", program_counter, X(),
                                  kk());
}

void LLVMAOT::SE_Vx_Vy() {
    source_builder << fmt::format("SE_Vx_Vy(" ADDR c REG c REG ");", program_counter, X(), Y());
}

void LLVMAOT::LD_Vx_byte() {
    source_builder << fmt::format("LD" IMM);
}

void LLVMAOT::ADD_Vx_byte() {
    source_builder << fmt::format("ADD" IMM);
}

void LLVMAOT::split_8() {
    (this->*opcode_table_8[n()])();
}

void LLVMAOT::LD_Vx_Vy() {
    source_builder << fmt::format("LD" REGS);
}

void LLVMAOT::OR_Vx_Vy() {
    source_builder << fmt::format("OR" REGS);
}

void LLVMAOT::AND_Vx_Vy() {
    source_builder << fmt::format("AND" REGS);
}

void LLVMAOT::XOR_Vx_Vy() {
    source_builder << fmt::format("XOR" REGS);
}

void LLVMAOT::ADD_Vx_Vy() {
    source_builder << fmt::format("ADD" REGS);
}

void LLVMAOT::SUB_Vx_Vy() {
    source_builder << fmt::format("SUB" REGS);
}

void LLVMAOT::SHR_Vx() {
    source_builder << fmt::format("SHR_Vx<" REG ">();", X());
}

void LLVMAOT::SUBN_Vx_Vy() {
    source_builder << fmt::format("SUBN" REGS);
}

void LLVMAOT::SHL_Vx() {
    source_builder << fmt::format("SHL_Vx<" REG ">();", X());
}

void LLVMAOT::SNE_Vx_Vy() {
    source_builder << fmt::format("SNE_Vx_Vy(" ADDR c REG c REG ");", program_counter, X(), Y());
}

void LLVMAOT::LD_I_addr() {
    source_builder << fmt::format("LD_I_addr<" ADDR ">();", nnn());
}

void LLVMAOT::JP_V0_addr() {
    source_builder << fmt::format("JP_V0_addr(" ADDR c ADDR ");", program_counter, nnn());
}

void LLVMAOT::RND_Vx_byte() {
    source_builder << fmt::format("RND" IMM);
}

void LLVMAOT::DRW_Vx_Vy_nibble() {
    source_builder << fmt::format("DRW_Vx_Vy_nibble<" REG c REG c BYTE ">();", X(), Y(), n());
}

void LLVMAOT::split_E() {
    (this->*opcode_table_E[kk()])();
}

void LLVMAOT::SKP_Vx() {
    source_builder << fmt::format("SKP_Vx(" ADDR c REG ");", program_counter, X());
}

void LLVMAOT::SKNP_Vx() {
    source_builder << fmt::format("SKNP_Vx(" ADDR c REG ");", program_counter, X());
}

void LLVMAOT::split_F() {
    (this->*opcode_table_F[kk()])();
}

void LLVMAOT::LD_Vx_DT() {
    source_builder << fmt::format("LD_Vx_DT<" REG ">();", X());
}

void LLVMAOT::LD_Vx_K() {
    source_builder << fmt::format("LD_Vx_K<" REG ">();", X());
}

void LLVMAOT::LD_DT_Vx() {
    source_builder << fmt::format("LD_DT" ONE_REG);
}

void LLVMAOT::LD_ST_Vx() {
    source_builder << fmt::format("LD_ST" ONE_REG);
}

void LLVMAOT::ADD_I_Vx() {
    source_builder << fmt::format("ADD_I" ONE_REG);
}

void LLVMAOT::LD_F_Vx() {
    source_builder << fmt::format("LD_F" ONE_REG);
}

void LLVMAOT::LD_B_Vx() {
    source_builder << fmt::format("LD_B" ONE_REG);
}

void LLVMAOT::LD_I_Vx() {
    source_builder << fmt::format("LD_I" ONE_REG);
}

void LLVMAOT::LD_Vx_I() {
    source_builder << fmt::format("LD_Vx_I<" REG ">();", X());
}