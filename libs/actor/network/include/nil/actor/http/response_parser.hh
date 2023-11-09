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
//
//
#pragma once

#include <nil/actor/core/ragel.hh>
#include <memory>
#include <unordered_map>

namespace nil {
    namespace actor {

        struct http_response {
            sstring _version;
            std::unordered_map<sstring, sstring> _headers;
        };

#line 33 "src/http/response_parser.rl"

#line 89 "src/http/response_parser.rl"

        class http_response_parser : public ragel_parser_base<http_response_parser> {

#line 45 "include/nil/actor/network/http/response_parser.hh"
            static const int start = 1;
            static const int error = 0;

            static const int en_main = 1;

#line 92 "src/http/response_parser.rl"

        public:
            enum class state {
                error,
                eof,
                done,
            };
            std::unique_ptr<http_response> _rsp;
            sstring _field_name;
            sstring _value;
            state _state;

        public:
            void init() {
                init_base();
                _rsp.reset(new http_response());
                _state = state::eof;

#line 70 "include/nil/actor/network/http/response_parser.hh"
                { _fsm_cs = (int)start; }

#line 108 "src/http/response_parser.rl"
            }
            char *parse(char *p, char *pe, char *eof) {
                sstring_builder::guard g(_builder, p, pe);
                auto str = [this, &g, &p] {
                    g.mark_end(p);
                    return get_str();
                };
                bool done = false;
                if (p != pe) {
                    _state = state::error;
                }
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmisleading-indentation"
#endif

#line 90 "include/nil/actor/network/http/response_parser.hh"
                {
                    if (p == pe)
                        goto _test_eof;
                    switch (_fsm_cs) {
                        case 1:
                            goto st_case_1;
                        case 0:
                            goto st_case_0;
                        case 2:
                            goto st_case_2;
                        case 3:
                            goto st_case_3;
                        case 4:
                            goto st_case_4;
                        case 5:
                            goto st_case_5;
                        case 6:
                            goto st_case_6;
                        case 7:
                            goto st_case_7;
                        case 8:
                            goto st_case_8;
                        case 9:
                            goto st_case_9;
                        case 10:
                            goto st_case_10;
                        case 11:
                            goto st_case_11;
                        case 12:
                            goto st_case_12;
                        case 13:
                            goto st_case_13;
                        case 14:
                            goto st_case_14;
                        case 15:
                            goto st_case_15;
                        case 16:
                            goto st_case_16;
                        case 17:
                            goto st_case_17;
                        case 34:
                            goto st_case_34;
                        case 18:
                            goto st_case_18;
                        case 19:
                            goto st_case_19;
                        case 20:
                            goto st_case_20;
                        case 21:
                            goto st_case_21;
                        case 22:
                            goto st_case_22;
                        case 23:
                            goto st_case_23;
                        case 24:
                            goto st_case_24;
                        case 25:
                            goto st_case_25;
                        case 26:
                            goto st_case_26;
                        case 27:
                            goto st_case_27;
                        case 28:
                            goto st_case_28;
                        case 29:
                            goto st_case_29;
                        case 30:
                            goto st_case_30;
                        case 31:
                            goto st_case_31;
                        case 32:
                            goto st_case_32;
                        case 33:
                            goto st_case_33;
                    }
                    goto st_out;
                    p += 1;
                    if (p == pe)
                        goto _test_eof1;
                st_case_1:
                    if (((*(p))) == 72) {
                        goto _st2;
                    }
                    { goto _st0; }
                st_case_0:
                _st0:
                    _fsm_cs = 0;
                    goto _pop;
                _st2:
                    p += 1;
                    if (p == pe)
                        goto _test_eof2;
                st_case_2:
                    if (((*(p))) == 84) {
                        goto _st3;
                    }
                    { goto _st0; }
                _st3:
                    p += 1;
                    if (p == pe)
                        goto _test_eof3;
                st_case_3:
                    if (((*(p))) == 84) {
                        goto _st4;
                    }
                    { goto _st0; }
                _st4:
                    p += 1;
                    if (p == pe)
                        goto _test_eof4;
                st_case_4:
                    if (((*(p))) == 80) {
                        goto _st5;
                    }
                    { goto _st0; }
                _st5:
                    p += 1;
                    if (p == pe)
                        goto _test_eof5;
                st_case_5:
                    if (((*(p))) == 47) {
                        goto _st6;
                    }
                    { goto _st0; }
                _st6:
                    p += 1;
                    if (p == pe)
                        goto _test_eof6;
                st_case_6:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _ctr7;
                    }
                    { goto _st0; }
                _ctr7 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 243 "include/nil/actor/network/http/response_parser.hh"

                    goto _st7;
                _st7:
                    p += 1;
                    if (p == pe)
                        goto _test_eof7;
                st_case_7:
                    if (((*(p))) == 46) {
                        goto _st8;
                    }
                    { goto _st0; }
                _st8:
                    p += 1;
                    if (p == pe)
                        goto _test_eof8;
                st_case_8:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _st9;
                    }
                    { goto _st0; }
                _st9:
                    p += 1;
                    if (p == pe)
                        goto _test_eof9;
                st_case_9:
                    if (((*(p))) == 32) {
                        goto _ctr11;
                    }
                    if (9 <= ((*(p))) && ((*(p))) <= 13) {
                        goto _ctr11;
                    }
                    { goto _st0; }
                _ctr11 : {
#line 43 "src/http/response_parser.rl"

                    _rsp->_version = str();
                }

#line 289 "include/nil/actor/network/http/response_parser.hh"

                    goto _st10;
                _st10:
                    p += 1;
                    if (p == pe)
                        goto _test_eof10;
                st_case_10:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _st11;
                    }
                    { goto _st0; }
                _st11:
                    p += 1;
                    if (p == pe)
                        goto _test_eof11;
                st_case_11:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _st12;
                    }
                    { goto _st0; }
                _st12:
                    p += 1;
                    if (p == pe)
                        goto _test_eof12;
                st_case_12:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _st13;
                    }
                    { goto _st0; }
                _st13:
                    p += 1;
                    if (p == pe)
                        goto _test_eof13;
                st_case_13:
                    if (((*(p))) == 32) {
                        goto _st14;
                    }
                    if (9 <= ((*(p))) && ((*(p))) <= 13) {
                        goto _st14;
                    }
                    { goto _st0; }
                _st14:
                    p += 1;
                    if (p == pe)
                        goto _test_eof14;
                st_case_14:
                    switch (((*(p)))) {
                        case 10: {
                            goto _st0;
                        }
                        case 13: {
                            goto _st15;
                        }
                    }
                    { goto _st14; }
                _st15:
                    p += 1;
                    if (p == pe)
                        goto _test_eof15;
                st_case_15:
                    if (((*(p))) == 10) {
                        goto _st16;
                    }
                    { goto _st0; }
                _st16:
                    p += 1;
                    if (p == pe)
                        goto _test_eof16;
                st_case_16:
                    switch (((*(p)))) {
                        case 13: {
                            goto _st17;
                        }
                        case 33: {
                            goto _ctr20;
                        }
                        case 124: {
                            goto _ctr20;
                        }
                        case 126: {
                            goto _ctr20;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr20;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr20;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr20;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr20;
                            }
                        } else {
                            goto _ctr20;
                        }
                    } else {
                        goto _ctr20;
                    }
                    { goto _st0; }
                _ctr34 : {
#line 55 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] = std::move(_value);
                }

#line 418 "include/nil/actor/network/http/response_parser.hh"

                    goto _st17;
                _ctr46 : {
#line 59 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] += sstring(" ") + std::move(_value);
                }

#line 428 "include/nil/actor/network/http/response_parser.hh"

                    goto _st17;
                _ctr63 : {
#line 55 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] = std::move(_value);
                }

#line 438 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 59 "src/http/response_parser.rl"

                        _rsp->_headers[_field_name] += sstring(" ") + std::move(_value);
                    }

#line 446 "include/nil/actor/network/http/response_parser.hh"

                    goto _st17;
                _st17:
                    p += 1;
                    if (p == pe)
                        goto _test_eof17;
                st_case_17:
                    if (((*(p))) == 10) {
                        goto _ctr21;
                    }
                    { goto _st0; }
                _ctr21 : {
#line 63 "src/http/response_parser.rl"

                    done = true;
                    {
                        p += 1;
                        _fsm_cs = 34;
                        goto _out;
                    }
                }

#line 468 "include/nil/actor/network/http/response_parser.hh"

                    goto _st34;
                _st34:
                    p += 1;
                    if (p == pe)
                        goto _test_eof34;
                st_case_34 : { goto _st0; }
                _ctr20 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 486 "include/nil/actor/network/http/response_parser.hh"

                    goto _st18;
                _ctr35 : {
#line 55 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] = std::move(_value);
                }

#line 496 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 39 "src/http/response_parser.rl"

                        g.mark_start(p);
                    }

#line 504 "include/nil/actor/network/http/response_parser.hh"

                    goto _st18;
                _st18:
                    p += 1;
                    if (p == pe)
                        goto _test_eof18;
                st_case_18:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr23;
                        }
                        case 32: {
                            goto _ctr23;
                        }
                        case 33: {
                            goto _st18;
                        }
                        case 58: {
                            goto _ctr24;
                        }
                        case 124: {
                            goto _st18;
                        }
                        case 126: {
                            goto _st18;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _st18;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _st18;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _st18;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _st18;
                            }
                        } else {
                            goto _st18;
                        }
                    } else {
                        goto _st18;
                    }
                    { goto _st0; }
                _ctr23 : {
#line 47 "src/http/response_parser.rl"

                    _field_name = str();
                }

#line 565 "include/nil/actor/network/http/response_parser.hh"

                    goto _st19;
                _st19:
                    p += 1;
                    if (p == pe)
                        goto _test_eof19;
                st_case_19:
                    switch (((*(p)))) {
                        case 9: {
                            goto _st19;
                        }
                        case 32: {
                            goto _st19;
                        }
                        case 58: {
                            goto _st20;
                        }
                    }
                    { goto _st0; }
                _ctr24 : {
#line 47 "src/http/response_parser.rl"

                    _field_name = str();
                }

#line 594 "include/nil/actor/network/http/response_parser.hh"

                    goto _st20;
                _st20:
                    p += 1;
                    if (p == pe)
                        goto _test_eof20;
                st_case_20:
                    if (((*(p))) == 13) {
                        goto _ctr28;
                    }
                    { goto _ctr27; }
                _ctr27 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 615 "include/nil/actor/network/http/response_parser.hh"

                    goto _st21;
                _st21:
                    p += 1;
                    if (p == pe)
                        goto _test_eof21;
                st_case_21:
                    if (((*(p))) == 13) {
                        goto _ctr30;
                    }
                    { goto _st21; }
                _ctr28 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 636 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 644 "include/nil/actor/network/http/response_parser.hh"

                    goto _st22;
                _ctr30 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 654 "include/nil/actor/network/http/response_parser.hh"

                    goto _st22;
                _st22:
                    p += 1;
                    if (p == pe)
                        goto _test_eof22;
                st_case_22:
                    if (((*(p))) == 10) {
                        goto _st23;
                    }
                    { goto _st0; }
                _st23:
                    p += 1;
                    if (p == pe)
                        goto _test_eof23;
                st_case_23:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr33;
                        }
                        case 13: {
                            goto _ctr34;
                        }
                        case 32: {
                            goto _ctr33;
                        }
                        case 33: {
                            goto _ctr35;
                        }
                        case 124: {
                            goto _ctr35;
                        }
                        case 126: {
                            goto _ctr35;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr35;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr35;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr35;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr35;
                            }
                        } else {
                            goto _ctr35;
                        }
                    } else {
                        goto _ctr35;
                    }
                    { goto _st0; }
                _ctr38 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 726 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 734 "include/nil/actor/network/http/response_parser.hh"

                    goto _st24;
                _ctr33 : {
#line 55 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] = std::move(_value);
                }

#line 744 "include/nil/actor/network/http/response_parser.hh"

                    goto _st24;
                _ctr45 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 754 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 59 "src/http/response_parser.rl"

                        _rsp->_headers[_field_name] += sstring(" ") + std::move(_value);
                    }

#line 762 "include/nil/actor/network/http/response_parser.hh"

                    goto _st24;
                _ctr62 : {
#line 55 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] = std::move(_value);
                }

#line 772 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 780 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 59 "src/http/response_parser.rl"

                        _rsp->_headers[_field_name] += sstring(" ") + std::move(_value);
                    }

#line 788 "include/nil/actor/network/http/response_parser.hh"

                    goto _st24;
                _st24:
                    p += 1;
                    if (p == pe)
                        goto _test_eof24;
                st_case_24:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr38;
                        }
                        case 13: {
                            goto _ctr39;
                        }
                        case 32: {
                            goto _ctr38;
                        }
                    }
                    { goto _ctr37; }
                _ctr37 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 817 "include/nil/actor/network/http/response_parser.hh"

                    goto _st25;
                _ctr41 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 827 "include/nil/actor/network/http/response_parser.hh"

                    goto _st25;
                _st25:
                    p += 1;
                    if (p == pe)
                        goto _test_eof25;
                st_case_25:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr41;
                        }
                        case 13: {
                            goto _ctr42;
                        }
                        case 32: {
                            goto _ctr41;
                        }
                    }
                    { goto _st25; }
                _ctr39 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 856 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 864 "include/nil/actor/network/http/response_parser.hh"

                    goto _st26;
                _ctr42 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 874 "include/nil/actor/network/http/response_parser.hh"

                    goto _st26;
                _st26:
                    p += 1;
                    if (p == pe)
                        goto _test_eof26;
                st_case_26:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr41;
                        }
                        case 10: {
                            goto _st27;
                        }
                        case 13: {
                            goto _ctr42;
                        }
                        case 32: {
                            goto _ctr41;
                        }
                    }
                    { goto _st25; }
                _st27:
                    p += 1;
                    if (p == pe)
                        goto _test_eof27;
                st_case_27:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr45;
                        }
                        case 13: {
                            goto _ctr46;
                        }
                        case 32: {
                            goto _ctr45;
                        }
                        case 33: {
                            goto _ctr47;
                        }
                        case 124: {
                            goto _ctr47;
                        }
                        case 126: {
                            goto _ctr47;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr47;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr47;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr47;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr47;
                            }
                        } else {
                            goto _ctr47;
                        }
                    } else {
                        goto _ctr47;
                    }
                    { goto _st25; }
                _ctr47 : {
#line 59 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] += sstring(" ") + std::move(_value);
                }

#line 957 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 39 "src/http/response_parser.rl"

                        g.mark_start(p);
                    }

#line 965 "include/nil/actor/network/http/response_parser.hh"

                    goto _st28;
                _ctr64 : {
#line 55 "src/http/response_parser.rl"

                    _rsp->_headers[_field_name] = std::move(_value);
                }

#line 975 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 59 "src/http/response_parser.rl"

                        _rsp->_headers[_field_name] += sstring(" ") + std::move(_value);
                    }

#line 983 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 39 "src/http/response_parser.rl"

                        g.mark_start(p);
                    }

#line 991 "include/nil/actor/network/http/response_parser.hh"

                    goto _st28;
                _st28:
                    p += 1;
                    if (p == pe)
                        goto _test_eof28;
                st_case_28:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr49;
                        }
                        case 13: {
                            goto _ctr42;
                        }
                        case 32: {
                            goto _ctr49;
                        }
                        case 33: {
                            goto _st28;
                        }
                        case 58: {
                            goto _ctr50;
                        }
                        case 124: {
                            goto _st28;
                        }
                        case 126: {
                            goto _st28;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _st28;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _st28;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _st28;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _st28;
                            }
                        } else {
                            goto _st28;
                        }
                    } else {
                        goto _st28;
                    }
                    { goto _st25; }
                _ctr52 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 1055 "include/nil/actor/network/http/response_parser.hh"

                    goto _st29;
                _ctr49 : {
#line 47 "src/http/response_parser.rl"

                    _field_name = str();
                }

#line 1065 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 1073 "include/nil/actor/network/http/response_parser.hh"

                    goto _st29;
                _st29:
                    p += 1;
                    if (p == pe)
                        goto _test_eof29;
                st_case_29:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr52;
                        }
                        case 13: {
                            goto _ctr42;
                        }
                        case 32: {
                            goto _ctr52;
                        }
                        case 58: {
                            goto _st30;
                        }
                    }
                    { goto _st25; }
                _ctr50 : {
#line 47 "src/http/response_parser.rl"

                    _field_name = str();
                }

#line 1105 "include/nil/actor/network/http/response_parser.hh"

                    goto _st30;
                _st30:
                    p += 1;
                    if (p == pe)
                        goto _test_eof30;
                st_case_30:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr55;
                        }
                        case 13: {
                            goto _ctr56;
                        }
                        case 32: {
                            goto _ctr55;
                        }
                    }
                    { goto _ctr54; }
                _ctr54 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 1134 "include/nil/actor/network/http/response_parser.hh"

                    goto _st31;
                _ctr55 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 1144 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 1152 "include/nil/actor/network/http/response_parser.hh"

                    goto _st31;
                _ctr58 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 1162 "include/nil/actor/network/http/response_parser.hh"

                    goto _st31;
                _st31:
                    p += 1;
                    if (p == pe)
                        goto _test_eof31;
                st_case_31:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr58;
                        }
                        case 13: {
                            goto _ctr59;
                        }
                        case 32: {
                            goto _ctr58;
                        }
                    }
                    { goto _st31; }
                _ctr56 : {
#line 39 "src/http/response_parser.rl"

                    g.mark_start(p);
                }

#line 1191 "include/nil/actor/network/http/response_parser.hh"

                    {
#line 51 "src/http/response_parser.rl"

                        _value = str();
                    }

#line 1199 "include/nil/actor/network/http/response_parser.hh"

                    goto _st32;
                _ctr59 : {
#line 51 "src/http/response_parser.rl"

                    _value = str();
                }

#line 1209 "include/nil/actor/network/http/response_parser.hh"

                    goto _st32;
                _st32:
                    p += 1;
                    if (p == pe)
                        goto _test_eof32;
                st_case_32:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr41;
                        }
                        case 10: {
                            goto _st33;
                        }
                        case 13: {
                            goto _ctr42;
                        }
                        case 32: {
                            goto _ctr41;
                        }
                    }
                    { goto _st25; }
                _st33:
                    p += 1;
                    if (p == pe)
                        goto _test_eof33;
                st_case_33:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr62;
                        }
                        case 13: {
                            goto _ctr63;
                        }
                        case 32: {
                            goto _ctr62;
                        }
                        case 33: {
                            goto _ctr64;
                        }
                        case 124: {
                            goto _ctr64;
                        }
                        case 126: {
                            goto _ctr64;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr64;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr64;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr64;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr64;
                            }
                        } else {
                            goto _ctr64;
                        }
                    } else {
                        goto _ctr64;
                    }
                    { goto _st25; }
                st_out:
                _test_eof1:
                    _fsm_cs = 1;
                    goto _test_eof;
                _test_eof2:
                    _fsm_cs = 2;
                    goto _test_eof;
                _test_eof3:
                    _fsm_cs = 3;
                    goto _test_eof;
                _test_eof4:
                    _fsm_cs = 4;
                    goto _test_eof;
                _test_eof5:
                    _fsm_cs = 5;
                    goto _test_eof;
                _test_eof6:
                    _fsm_cs = 6;
                    goto _test_eof;
                _test_eof7:
                    _fsm_cs = 7;
                    goto _test_eof;
                _test_eof8:
                    _fsm_cs = 8;
                    goto _test_eof;
                _test_eof9:
                    _fsm_cs = 9;
                    goto _test_eof;
                _test_eof10:
                    _fsm_cs = 10;
                    goto _test_eof;
                _test_eof11:
                    _fsm_cs = 11;
                    goto _test_eof;
                _test_eof12:
                    _fsm_cs = 12;
                    goto _test_eof;
                _test_eof13:
                    _fsm_cs = 13;
                    goto _test_eof;
                _test_eof14:
                    _fsm_cs = 14;
                    goto _test_eof;
                _test_eof15:
                    _fsm_cs = 15;
                    goto _test_eof;
                _test_eof16:
                    _fsm_cs = 16;
                    goto _test_eof;
                _test_eof17:
                    _fsm_cs = 17;
                    goto _test_eof;
                _test_eof34:
                    _fsm_cs = 34;
                    goto _test_eof;
                _test_eof18:
                    _fsm_cs = 18;
                    goto _test_eof;
                _test_eof19:
                    _fsm_cs = 19;
                    goto _test_eof;
                _test_eof20:
                    _fsm_cs = 20;
                    goto _test_eof;
                _test_eof21:
                    _fsm_cs = 21;
                    goto _test_eof;
                _test_eof22:
                    _fsm_cs = 22;
                    goto _test_eof;
                _test_eof23:
                    _fsm_cs = 23;
                    goto _test_eof;
                _test_eof24:
                    _fsm_cs = 24;
                    goto _test_eof;
                _test_eof25:
                    _fsm_cs = 25;
                    goto _test_eof;
                _test_eof26:
                    _fsm_cs = 26;
                    goto _test_eof;
                _test_eof27:
                    _fsm_cs = 27;
                    goto _test_eof;
                _test_eof28:
                    _fsm_cs = 28;
                    goto _test_eof;
                _test_eof29:
                    _fsm_cs = 29;
                    goto _test_eof;
                _test_eof30:
                    _fsm_cs = 30;
                    goto _test_eof;
                _test_eof31:
                    _fsm_cs = 31;
                    goto _test_eof;
                _test_eof32:
                    _fsm_cs = 32;
                    goto _test_eof;
                _test_eof33:
                    _fsm_cs = 33;
                    goto _test_eof;

                _test_eof : { }
                    if (_fsm_cs >= 34)
                        goto _out;
                _pop : { }
                _out : { }
                }

#line 121 "src/http/response_parser.rl"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
                if (!done) {
                    p = nullptr;
                } else {
                    _state = state::done;
                }
                return p;
            }
            auto get_parsed_response() {
                return std::move(_rsp);
            }
            bool eof() const {
                return _state == state::eof;
            }
        };

    }    // namespace actor
}    // namespace nil
