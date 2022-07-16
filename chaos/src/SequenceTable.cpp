/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <plog/Log.h>
#include "SequenceTable.hpp"

using namespace Chaos;

bool SequenceTable::addDefinedSequence(const std::string& name, std::shared_ptr<Sequence> new_sequence) {
  PLOG_DEBUG << "Adding pre-defined sequence: " << name;
 	auto [it, result] = sequence_map.try_emplace(name, new_sequence);
  return (result);
}

std::shared_ptr<Sequence> SequenceTable::getSequence(const std::string& name) {
  auto iter = sequence_map.find(name);
  if (iter != sequence_map.end()) {
    return iter->second;
  }
  return nullptr;
}

void SequenceTable::addToSequence(Sequence& seq, const std::string& name) {
  std::shared_ptr<Sequence> new_sequence = getSequence(name);
  if (new_sequence) {
    seq.addSequence(new_sequence);
  }
}

void SequenceTable::addDelayToSequence(Sequence& sequence, double delay) {
  sequence.addDelay(delay);
}

void SequenceTable::clearSequenceList() {
  if (sequence_map.size() > 0) {
    PLOG_DEBUG << "Clearing existing sequence data";
    sequence_map.clear();
  }
}