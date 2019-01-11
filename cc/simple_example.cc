// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <iostream>

#include "cc/dual_net/factory.h"
#include "cc/game.h"
#include "cc/init.h"
#include "cc/logging.h"
#include "cc/mcts_player.h"
#include "cc/platform/utils.h"
#include "cc/random.h"
#include "cc/zobrist.h"
#include "gflags/gflags.h"

// Inference flags.
DEFINE_string(model, "",
              "Path to a minigo model. The format of the model depends on the "
              "inference engine. For engine=tf, the model should be a GraphDef "
              "proto. For engine=lite, the model should be .tflite "
              "flatbuffer.");
DEFINE_int32(num_readouts, 100,
             "Number of readouts to make during tree search for each move.");

namespace minigo {
namespace {

void SimpleExample() {
  // Determine whether ANSI color codes are supported (used when printing
  // the board state after each move).
  const bool use_ansi_colors = FdSupportsAnsiColors(fileno(stderr));

  // Load the model specified by the command line arguments.
  auto model_factory = NewDualNetFactory();
  auto model = model_factory->NewDualNet(FLAGS_model);

  // Create the player.
  MctsPlayer::Options options;
  options.inject_noise = false;
  options.soft_pick = false;
  options.num_readouts = FLAGS_num_readouts;
  MctsPlayer player(std::move(model), options);

  // Create a game object that tracks the move history & final score.
  Game game(player.name(), player.name(), options.game_options);

  // Tell the model factory we're starting a game.
  // TODO(tommadams): Remove this once BatchingDualNetFactory is no longer a
  // DualNetFactory.
  model_factory->StartGame(player.network(), player.network());

  // Play the game.
  while (!player.root()->game_over() && !player.root()->at_move_limit()) {
    auto move = player.SuggestMove();

    const auto& position = player.root()->position;
    MG_LOG(INFO) << player.root()->position.ToPrettyString(use_ansi_colors);
    MG_LOG(INFO) << "Move: " << position.n()
                 << " Captures X: " << position.num_captures()[0]
                 << " O: " << position.num_captures()[1];
    MG_LOG(INFO) << player.root()->Describe();

    MG_CHECK(player.PlayMove(move, &game));
  }

  // Tell the model factory we're ending a game.
  // TODO(tommadams): Remove this once BatchingDualNetFactory is no longer a
  // DualNetFactory.
  model_factory->EndGame(player.network(), player.network());

  std::cout << game.result_string() << std::endl;
}

}  // namespace
}  // namespace minigo

int main(int argc, char* argv[]) {
  minigo::Init(&argc, &argv);
  minigo::zobrist::Init(0);
  minigo::SimpleExample();
  return 0;
}