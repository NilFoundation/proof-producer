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

#include <nil/actor/core/sstring.hh>
#include <nil/actor/detail/log-impl.hh>
#include <nil/actor/core/lowres_clock.hh>

#include <unordered_map>
#include <exception>
#include <iosfwd>
#include <atomic>
#include <mutex>

#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>

#include <fmt/format.h>

/// \addtogroup logging
/// @{

namespace nil {
    namespace actor {

        /// \brief log level used with \see {logger}
        /// used with the logger.do_log method.
        /// Levels are in increasing order. That is if you want to see debug(3) logs you
        /// will also see error(0), warn(1), info(2).
        ///
        enum class log_level {
            error,
            warn,
            info,
            debug,
            trace,
        };

        std::ostream &operator<<(std::ostream &out, log_level level);
        std::istream &operator>>(std::istream &in, log_level &level);
    }    // namespace actor
}    // namespace nil

// Boost doesn't auto-deduce the existence of the streaming operators for some reason

namespace boost {
    template<>
    nil::actor::log_level lexical_cast(const std::string &source);

}

namespace nil {
    namespace actor {

        class logger;
        class logger_registry;

        /// \brief Logger class for ostream or syslog.
        ///
        /// Java style api for logging.
        /// \code {.cpp}
        /// static nil::actor::logger logger("lsa-api");
        /// logger.info("Triggering compaction");
        /// \endcode
        /// The output format is: (depending on level)
        /// DEBUG  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
        ///
        /// It is possible to rate-limit log messages, see \ref logger::rate_limit.
        class logger {
            sstring _name;
            std::atomic<log_level> _level = {log_level::info};
            static std::ostream *_out;
            static std::atomic<bool> _ostream;
            static std::atomic<bool> _syslog;

        public:
            class log_writer {
            public:
                virtual ~log_writer() = default;
                virtual detail::log_buf::inserter_iterator operator()(detail::log_buf::inserter_iterator) = 0;
            };
            template<typename Func>
#ifdef BOOST_HAS_CONCEPTS
                requires requires(Func fn, detail::log_buf::inserter_iterator it) { it = fn(it); }
#endif
            class lambda_log_writer : public log_writer {
                Func _func;

            public:
                lambda_log_writer(Func &&func) : _func(std::forward<Func>(func)) {
                }
                virtual ~lambda_log_writer() override = default;
                virtual detail::log_buf::inserter_iterator operator()(detail::log_buf::inserter_iterator it) override {
                    return _func(it);
                }
            };

        private:
            // We can't use an std::function<> as it potentially allocates.
            void do_log(log_level level, log_writer &writer);
            void failed_to_log(std::exception_ptr ex) noexcept;

        public:
            /// Apply a rate limit to log message(s)
            ///
            /// Pass this to \ref logger::log() to apply a rate limit to the message.
            /// The rate limit is applied to all \ref logger::log() calls this rate
            /// limit is passed to. Example:
            ///
            ///     void handle_request() {
            ///         static thread_local logger::rate_limit my_rl(std::chrono::seconds(10));
            ///         // ...
            ///         my_log.log(log_level::info, my_rl, "a message we don't want to log on every request, only at
            ///         most once each 10 seconds");
            ///         // ...
            ///     }
            ///
            /// The rate limit ensures that at most one message per interval will be
            /// logged. If there were messages dropped due to rate-limiting the
            /// following snippet will be prepended to the first non-dropped log
            /// messages:
            ///
            ///     (rate limiting dropped $N similar messages)
            ///
            /// Where $N is the number of messages dropped.
            class rate_limit {
                friend class logger;

                using clock = lowres_clock;

            private:
                clock::duration _interval;
                clock::time_point _next;
                uint64_t _dropped_messages = 0;

            private:
                bool check();
                bool has_dropped_messages() const {
                    return bool(_dropped_messages);
                }
                uint64_t get_and_reset_dropped_messages() {
                    return std::exchange(_dropped_messages, 0);
                }

            public:
                explicit rate_limit(std::chrono::milliseconds interval);
            };

        public:
            explicit logger(sstring name);
            logger(logger &&x);
            ~logger();

            bool is_shard_zero() noexcept;

            /// Test if desired log level is enabled
            ///
            /// \param level - enum level value (info|error...)
            /// \return true if the log level has been enabled.
            bool is_enabled(log_level level) const noexcept {
                return __builtin_expect(level <= _level.load(std::memory_order_relaxed), false);
            }

            /// logs to desired level if enabled, otherwise we ignore the log line
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void log(log_level level, const char *fmt, Args &&...args) noexcept {
                if (is_enabled(level)) {
                    try {
                        lambda_log_writer writer([&](detail::log_buf::inserter_iterator it) {
#if FMT_VERSION >= 80000
                            return fmt::format_to(it, fmt::runtime(fmt), std::forward<Args>(args)...);
#else
                            return fmt::format_to(it, fmt, std::forward<Args>(args)...);
#endif
                        });
                        do_log(level, writer);
                    } catch (...) {
                        failed_to_log(std::current_exception());
                    }
                }
            }

            /// logs with a rate limit to desired level if enabled, otherwise we ignore the log line
            ///
            /// If there were messages dropped due to rate-limiting the following snippet
            /// will be prepended to the first non-dropped log messages:
            ///
            ///     (rate limiting dropped $N similar messages)
            ///
            /// Where $N is the number of messages dropped.
            ///
            /// \param rl - the \ref rate_limit to apply to this log
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void log(log_level level, rate_limit &rl, const char *fmt, Args &&...args) noexcept {
                if (is_enabled(level) && rl.check()) {
                    try {
                        lambda_log_writer writer([&](detail::log_buf::inserter_iterator it) {
                            if (rl.has_dropped_messages()) {
                                it = fmt::format_to(it, "(rate limiting dropped {} similar messages) ",
                                                    rl.get_and_reset_dropped_messages());
                            }
#if FMT_VERSION >= 80000
                            return fmt::format_to(it, fmt::runtime(fmt), std::forward<Args>(args)...);
#else
                            return fmt::format_to(it, fmt, std::forward<Args>(args)...);
#endif
                        });
                        do_log(level, writer);
                    } catch (...) {
                        failed_to_log(std::current_exception());
                    }
                }
            }

            /// \cond internal
            /// logs to desired level if enabled, otherwise we ignore the log line
            ///
            /// \param writer a function which writes directly to the underlying log buffer
            ///
            /// This is a low level method for use cases where it is very important to
            /// avoid any allocations. The \arg writer will be passed a
            /// detail::log_buf::inserter_iterator that allows it to write into the log
            /// buffer directly, avoiding the use of any intermediary buffers.
            void log(log_level level, log_writer &writer) noexcept {
                if (is_enabled(level)) {
                    try {
                        do_log(level, writer);
                    } catch (...) {
                        failed_to_log(std::current_exception());
                    }
                }
            }
            /// logs to desired level if enabled, otherwise we ignore the log line
            ///
            /// \param writer a function which writes directly to the underlying log buffer
            ///
            /// This is a low level method for use cases where it is very important to
            /// avoid any allocations. The \arg writer will be passed a
            /// detail::log_buf::inserter_iterator that allows it to write into the log
            /// buffer directly, avoiding the use of any intermediary buffers.
            /// This is rate-limited version, see \ref rate_limit.
            void log(log_level level, rate_limit &rl, log_writer &writer) noexcept {
                if (is_enabled(level) && rl.check()) {
                    try {
                        lambda_log_writer writer_wrapper([&](detail::log_buf::inserter_iterator it) {
                            if (rl.has_dropped_messages()) {
                                it = fmt::format_to(it, "(rate limiting dropped {} similar messages) ",
                                                    rl.get_and_reset_dropped_messages());
                            }
                            return writer(it);
                        });
                        do_log(level, writer_wrapper);
                    } catch (...) {
                        failed_to_log(std::current_exception());
                    }
                }
            }
            /// \endcond

            /// Log with error tag:
            /// ERROR  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void error(const char *fmt, Args &&...args) noexcept {
                log(log_level::error, fmt, std::forward<Args>(args)...);
            }
            /// Log with warning tag:
            /// WARN  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void warn(const char *fmt, Args &&...args) noexcept {
                log(log_level::warn, fmt, std::forward<Args>(args)...);
            }
            /// Log with info tag:
            /// INFO  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void info(const char *fmt, Args &&...args) noexcept {
                log(log_level::info, fmt, std::forward<Args>(args)...);
            }
            /// Log with info tag on shard zero only:
            /// INFO  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void info0(const char *fmt, Args &&...args) noexcept {
                if (is_shard_zero()) {
                    log(log_level::info, fmt, std::forward<Args>(args)...);
                }
            }
            /// Log with debug tag:
            /// DEBUG  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void debug(const char *fmt, Args &&...args) noexcept {
                log(log_level::debug, fmt, std::forward<Args>(args)...);
            }
            /// Log with trace tag:
            /// TRACE  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
            ///
            /// \param fmt - {fmt} style format
            /// \param args - args to print string
            ///
            template<typename... Args>
            void trace(const char *fmt, Args &&...args) noexcept {
                log(log_level::trace, fmt, std::forward<Args>(args)...);
            }

            /// \return name of the logger. Usually one logger per module
            ///
            const sstring &name() const noexcept {
                return _name;
            }

            /// \return current log level for this logger
            ///
            log_level level() const noexcept {
                return _level.load(std::memory_order_relaxed);
            }

            /// \param level - set the log level
            ///
            void set_level(log_level level) noexcept {
                _level.store(level, std::memory_order_relaxed);
            }

            /// Set output stream, default is std::cerr
            static void set_ostream(std::ostream &out) noexcept;

            /// Also output to ostream. default is true
            static void set_ostream_enabled(bool enabled) noexcept;

            /// Also output to stdout. default is true
            [[deprecated("Use set_ostream_enabled instead")]] static void set_stdout_enabled(bool enabled) noexcept;

            /// Also output to syslog. default is false
            ///
            /// NOTE: syslog() can block, which will stall the reactor thread.
            ///       this should be rare (will have to fill the pipe buffer
            ///       before syslogd can clear it) but can happen.
            static void set_syslog_enabled(bool enabled) noexcept;
        };

        /// \brief used to keep a static registry of loggers
        /// since the typical use case is to do:
        /// \code {.cpp}
        /// static nil::actor::logger("my_module");
        /// \endcode
        /// this class is used to wrap around the static map
        /// that holds pointers to all logs
        ///
        class logger_registry {
            mutable std::mutex _mutex;
            std::unordered_map<sstring, logger *> _loggers;

        public:
            /// loops through all registered loggers and sets the log level
            /// Note: this method locks
            ///
            /// \param level - desired level: error,info,...
            void set_all_loggers_level(log_level level);

            /// Given a name for a logger returns the log_level enum
            /// Note: this method locks
            ///
            /// \return log_level for the given logger name
            log_level get_logger_level(const sstring &name) const;

            /// Sets the log level for a given logger
            /// Note: this method locks
            ///
            /// \param name - name of logger
            /// \param level - desired level of logging
            void set_logger_level(const sstring &name, log_level level);

            /// Returns a list of registered loggers
            /// Note: this method locks
            ///
            /// \return all registered loggers
            std::vector<sstring> get_all_logger_names();

            /// Registers a logger with the static map
            /// Note: this method locks
            ///
            void register_logger(logger *l);
            /// Unregisters a logger with the static map
            /// Note: this method locks
            ///
            void unregister_logger(logger *l);
            /// Swaps the logger given the from->name() in the static map
            /// Note: this method locks
            ///
            void moved(logger *from, logger *to);
        };

        logger_registry &global_logger_registry();

        enum class logger_timestamp_style {
            none,
            boot,
            real,
        };

        enum class logger_ostream_type {
            none,
            stdout,
            stderr,
        };

        struct logging_settings final {
            std::unordered_map<sstring, log_level> logger_levels;
            log_level default_level;
            bool stdout_enabled;
            bool syslog_enabled;
            logger_timestamp_style stdout_timestamp_style = logger_timestamp_style::real;
            logger_ostream_type logger_ostream = logger_ostream_type::stderr;
        };

        /// Shortcut for configuring the logging system all at once.
        ///
        void apply_logging_settings(const logging_settings &);

        /// \cond internal

        extern thread_local uint64_t logging_failures;

        sstring pretty_type_name(const std::type_info &);

        sstring level_name(log_level level);

        template<typename T>
        class logger_for : public logger {
        public:
            logger_for() : logger(pretty_type_name(typeid(T))) {
            }
        };

        /// \endcond
    }    // namespace actor
}    // namespace nil

// Pretty-printer for exceptions to be logged, e.g., std::current_exception().
namespace std {
    std::ostream &operator<<(std::ostream &, const std::exception_ptr &);
    std::ostream &operator<<(std::ostream &, const std::exception &);
    std::ostream &operator<<(std::ostream &, const std::system_error &);
}    // namespace std

/// @}
