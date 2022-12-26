/*
 * Copyright (c) 2022, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "SourceFileHandler.h"
#include "JaktGenerator.h"
#include <filesystem>
#include <llvm/Support/raw_ostream.h>
#include <algorithm>
#include <system_error>

namespace jakt_bindgen {


SourceFileHandler::SourceFileHandler(std::string Namespace, std::filesystem::path out_dir, std::filesystem::path base_dir)
    : listener(std::move(Namespace), Finder)
    , out_dir(std::move(out_dir))
    , base_dir(std::move(base_dir))
{
}

bool SourceFileHandler::handleBeginSource(clang::CompilerInstance& CI)
{
    if (!clang::tooling::SourceFileCallbacks::handleBeginSource(CI))
        return false;

    auto main_file_id = CI.getSourceManager().getMainFileID();
    auto maybe_path = CI.getSourceManager().getNonBuiltinFilenameForID(main_file_id);
    if (!maybe_path.has_value())
        return false;

    current_filepath = std::filesystem::canonical(maybe_path.value().str()).lexically_relative(base_dir);
    llvm::outs() << "Processing " << current_filepath << "\n";

    listener.resetForNextFile();

    return true;
}

void SourceFileHandler::handleEndSource()
{
    std::string new_filename = out_dir / current_filepath.filename().replace_extension(".jakt");
    std::transform(new_filename.begin(), new_filename.end(), new_filename.begin(),
        [](unsigned char c) { return std::tolower(c); });


    std::error_code os_errc = {};
    llvm::raw_fd_ostream os(new_filename, os_errc, llvm::sys::fs::CD_CreateAlways);
    if (os_errc) {
        llvm::errs() << "Can't open file " << new_filename;
    }

    JaktGenerator generator(os, listener);

    generator.generate(current_filepath);
}

}
