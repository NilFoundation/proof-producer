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

#include <fcntl.h>
#include <sys/stat.h>
#include <type_traits>

namespace nil {
    namespace actor {

        /// \addtogroup fileio-module
        /// @{

        /// Enumeration describing how a file is to be opened.
        ///
        /// \see file::open_file_dma()
        enum class open_flags {
            rw = O_RDWR,
            ro = O_RDONLY,
            wo = O_WRONLY,
            create = O_CREAT,
            truncate = O_TRUNC,
            exclusive = O_EXCL,
            dsync = O_DSYNC,
        };

        inline open_flags operator|(open_flags a, open_flags b) {
            return open_flags(std::underlying_type_t<open_flags>(a) | std::underlying_type_t<open_flags>(b));
        }

        inline void operator|=(open_flags &a, open_flags b) {
            a = (a | b);
        }

        inline open_flags operator&(open_flags a, open_flags b) {
            return open_flags(std::underlying_type_t<open_flags>(a) & std::underlying_type_t<open_flags>(b));
        }

        inline void operator&=(open_flags &a, open_flags b) {
            a = (a & b);
        }

        /// Enumeration describing the type of a directory entry being listed.
        ///
        /// \see file::list_directory()
        enum class directory_entry_type {
            unknown,
            block_device,
            char_device,
            directory,
            fifo,
            link,
            regular,
            socket,
        };

        /// Enumeration describing the type of a particular filesystem
        enum class fs_type {
            other,
            xfs,
            ext2,
            ext3,
            ext4,
            btrfs,
            hfs,
            tmpfs,
        };

        // Access flags for files/directories
        enum class access_flags {
            exists = F_OK,
            read = R_OK,
            write = W_OK,
            execute = X_OK,

            // alias for directory access
            lookup = execute,
        };

        inline access_flags operator|(access_flags a, access_flags b) {
            return access_flags(std::underlying_type_t<access_flags>(a) | std::underlying_type_t<access_flags>(b));
        }

        inline access_flags operator&(access_flags a, access_flags b) {
            return access_flags(std::underlying_type_t<access_flags>(a) & std::underlying_type_t<access_flags>(b));
        }

        // Permissions for files/directories
        enum class file_permissions {
            user_read = S_IRUSR,       // Read by owner
            user_write = S_IWUSR,      // Write by owner
            user_execute = S_IXUSR,    // Execute by owner

            group_read = S_IRGRP,       // Read by group
            group_write = S_IWGRP,      // Write by group
            group_execute = S_IXGRP,    // Execute by group

            others_read = S_IROTH,       // Read by others
            others_write = S_IWOTH,      // Write by others
            others_execute = S_IXOTH,    // Execute by others

            user_permissions = user_read | user_write | user_execute,
            group_permissions = group_read | group_write | group_execute,
            others_permissions = others_read | others_write | others_execute,
            all_permissions = user_permissions | group_permissions | others_permissions,

            default_file_permissions =
                user_read | user_write | group_read | group_write | others_read | others_write,    // 0666
            default_dir_permissions = all_permissions,                                             // 0777
        };

        inline constexpr file_permissions operator|(file_permissions a, file_permissions b) {
            return file_permissions(std::underlying_type_t<file_permissions>(a) |
                                    std::underlying_type_t<file_permissions>(b));
        }

        inline constexpr file_permissions operator&(file_permissions a, file_permissions b) {
            return file_permissions(std::underlying_type_t<file_permissions>(a) &
                                    std::underlying_type_t<file_permissions>(b));
        }

        /// @}

    }    // namespace actor
}    // namespace nil
