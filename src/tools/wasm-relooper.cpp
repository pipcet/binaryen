/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
n *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// A WebAssembly optimizer, loads code, optionally runs passes on it,
// then writes it.
//

#include <memory>

#include "pass.h"
#include "support/command-line.h"
#include "support/file.h"
#include "wasm-builder.h"
#include "wasm-printing.h"
#include "wasm-s-parser.h"
#include "wasm-validator.h"
#include "wasm-io.h"
#include "cfg/Relooper.h"

using namespace wasm;

//
// main
//

int main(int argc, const char* argv[]) {
  Name entry;
  PassOptions passOptions;
  bool emitBinary = true;

  Options options("wasm-relooper", "Optimize .wast files");
  options
      .add("--output", "-o", "Output file (stdout if not specified)",
           Options::Arguments::One,
           [](Options* o, const std::string& argument) {
             o->extra["output"] = argument;
             Colors::disable();
           })
      .add("--emit-text", "-S", "Emit text instead of binary for the output file",
           Options::Arguments::Zero,
           [&](Options *o, const std::string &argument) { emitBinary = false; })
      .add_positional("INFILE", Options::Arguments::One,
                      [](Options* o, const std::string& argument) {
                        o->extra["infile"] = argument;
                      });

  options.debug = true;
  options.extra["infile"] = "b.wasm";

  auto input(read_file<std::string>(options.extra["infile"], Flags::Text, options.debug ? Flags::Debug : Flags::Release));

  Module wasm;

  {
    if (options.debug) std::cerr << "reading...\n";
    ModuleReader reader;
    reader.setDebug(options.debug);

    try {
      reader.read(options.extra["infile"], wasm);
    } catch (ParseException& p) {
      p.dump(std::cerr);
      Fatal() << "error in parsing input";
    }
  }

  if (!WasmValidator().validate(wasm)) {
    Fatal() << "error in validating input";
  }

  auto relooper = new CFG::Relooper();

  auto builder = new Builder(wasm);
  auto block1 = new CFG::Block(builder->makeNop());

  block1->BranchesIn.insert(block1);
  block1->AddBranchTo(block1, nullptr);
  block1->Id = 1;
  auto block2 = new CFG::Block(builder->makeNop());

  block2->BranchesIn.insert(block2);
  block2->AddBranchTo(block2, nullptr);
  block2->Id = 2;
  auto block3 = new CFG::Block(builder->makeNop());

  block1->BranchesIn.insert(block3);
  block2->BranchesIn.insert(block3);
  block3->AddBranchTo(block1, builder->makeNop());
  block3->AddBranchTo(block2, nullptr);
  block3->Id = 3;
  CFG::RelooperBuilder rb(wasm, 42);
  relooper->Calculate(block3);

  wasm::Expression* expr = relooper->Root->Render(rb, false);

  WasmPrinter::printExpression(expr, std::cout, false, false);
}
