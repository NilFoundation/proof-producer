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

#include <nil/actor/core/file.hh>

namespace nil {
    namespace actor {

        /// \addtogroup fileio-module
        /// @{

        /// Base class for layered file implementations.
        ///
        /// A layered file implementation implements `file_impl` virtual
        /// functions such as dma_read() by forwarding them to another, existing
        /// file called the underlying file. This base class simplifies construction
        /// of layered files by performing standard tasks such as setting up the
        /// file alignment. Actual implementation of the I/O methods is left for the
        /// derived class.
        class layered_file_impl : public file_impl {
        protected:
            file _underlying_file;

        public:
            /// Constructs a layered file. This sets up the underlying_file() method
            /// and initializes alignment constants to be the same as the underlying file.
            explicit layered_file_impl(file underlying_file) noexcept : _underlying_file(std::move(underlying_file)) {
                _memory_dma_alignment = _underlying_file.memory_dma_alignment();
                _disk_read_dma_alignment = _underlying_file.disk_read_dma_alignment();
                _disk_write_dma_alignment = _underlying_file.disk_write_dma_alignment();
            }

            /// The underlying file which can be used to back I/O methods.
            file &underlying_file() noexcept {
                return _underlying_file;
            }

            /// The underlying file which can be used to back I/O methods.
            const file &underlying_file() const noexcept {
                return _underlying_file;
            }
        };

        /// @}

    }    // namespace actor
}    // namespace nil
