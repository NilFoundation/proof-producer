//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#pragma once

#include <nil/actor/network/packet.hh>
#include <map>
#include <iostream>

namespace nil {
    namespace actor {

        namespace net {

            template<typename Offset, typename Tag>
            class packet_merger {
            private:
                static uint64_t &linearizations_ref() {
                    static thread_local uint64_t linearization_count;
                    return linearization_count;
                }

            public:
                std::map<Offset, packet> map;

                static uint64_t linearizations() {
                    return linearizations_ref();
                }

                void merge(Offset offset, packet p) {
                    bool insert = true;
                    auto beg = offset;
                    auto end = beg + p.len();
                    // Fisrt, try to merge the packet with existing segment
                    for (auto it = map.begin(); it != map.end();) {
                        auto &seg_pkt = it->second;
                        auto seg_beg = it->first;
                        auto seg_end = seg_beg + seg_pkt.len();
                        // There are 6 cases:
                        if (seg_beg <= beg && end <= seg_end) {
                            // 1) seg_beg beg end seg_end
                            // We already have data in this packet
                            return;
                        } else if (beg <= seg_beg && seg_end <= end) {
                            // 2) beg seg_beg seg_end end
                            // The new segment contains more data than this old segment
                            // Delete the old one, insert the new one
                            it = map.erase(it);
                            insert = true;
                            break;
                        } else if (beg < seg_beg && seg_beg <= end && end <= seg_end) {
                            // 3) beg seg_beg end seg_end
                            // Merge two segments, trim front of old segment
                            auto trim = end - seg_beg;
                            seg_pkt.trim_front(trim);
                            p.append(std::move(seg_pkt));
                            // Delete the old one, insert the new one
                            it = map.erase(it);
                            insert = true;
                            break;
                        } else if (seg_beg <= beg && beg <= seg_end && seg_end < end) {
                            // 4) seg_beg beg seg_end end
                            // Merge two segments, trim front of new segment
                            auto trim = seg_end - beg;
                            p.trim_front(trim);
                            // Append new data to the old segment, keep the old segment
                            seg_pkt.append(std::move(p));
                            seg_pkt.linearize();
                            ++linearizations_ref();
                            insert = false;
                            break;
                        } else {
                            // 5) beg end < seg_beg seg_end
                            //   or
                            // 6) seg_beg seg_end < beg end
                            // Can not merge with this segment, keep looking
                            it++;
                            insert = true;
                        }
                    }

                    if (insert) {
                        p.linearize();
                        ++linearizations_ref();
                        map.emplace(beg, std::move(p));
                    }

                    // Second, merge adjacent segments after this packet has been merged,
                    // becasue this packet might fill a "whole" and make two adjacent
                    // segments mergable
                    for (auto it = map.begin(); it != map.end();) {
                        // The first segment
                        auto &seg_pkt = it->second;
                        auto seg_beg = it->first;
                        auto seg_end = seg_beg + seg_pkt.len();

                        // The second segment
                        auto it_next = it;
                        it_next++;
                        if (it_next == map.end()) {
                            break;
                        }
                        auto &p = it_next->second;
                        auto beg = it_next->first;
                        auto end = beg + p.len();

                        // Merge the the second segment into first segment if possible
                        if (seg_beg <= beg && beg <= seg_end && seg_end < end) {
                            // Merge two segments, trim front of second segment
                            auto trim = seg_end - beg;
                            p.trim_front(trim);
                            // Append new data to the first segment, keep the first segment
                            seg_pkt.append(std::move(p));

                            // Delete the second segment
                            map.erase(it_next);

                            // Keep merging this first segment with its new next packet
                            // So we do not update the iterator: it
                            continue;
                        } else if (end <= seg_end) {
                            // The first segment has all the data in the second segment
                            // Delete the second segment
                            map.erase(it_next);
                            continue;
                        } else if (seg_end < beg) {
                            // Can not merge first segment with second segment
                            it = it_next;
                            continue;
                        } else {
                            // If we reach here, we have a bug with merge.
                            std::cerr << "packet_merger: merge error\n";
                            abort();
                            break;
                        }
                    }
                }
            };

        }    // namespace net

    }    // namespace actor
}    // namespace nil
