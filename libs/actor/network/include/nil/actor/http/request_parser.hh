#line 1 "src/http/request_parser.rl"
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

#include <nil/actor/core/ragel.hh>

#include <memory>
#include <unordered_map>

#include <nil/actor/http/request.hh>

namespace nil {
    namespace actor {

        using namespace httpd;

#line 33 "src/http/request_parser.rl"

#line 130 "src/http/request_parser.rl"

        class http_request_parser : public ragel_parser_base<http_request_parser> {

#line 45 "include/nil/actor/network/http/request_parser.hh"
            static const int start = 1;
            static const int error = 0;

            static const int en_main = 1;

#line 133 "src/http/request_parser.rl"

        public:
            enum class state {
                error,
                eof,
                done,
            };
            std::unique_ptr<httpd::request> _req;
            sstring _field_name;
            sstring _value;
            state _state;

        public:
            void init() {
                init_base();
                _req.reset(new httpd::request());
                _state = state::eof;

#line 70 "include/nil/actor/network/http/request_parser.hh"
                { _fsm_cs = (int)start; }

#line 149 "src/http/request_parser.rl"
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
                //#ifdef __clang__
                //#pragma clang diagnostic push
                //#pragma clang diagnostic ignored "-Wmisleading-indentation"
                //#endif

#line 90 "include/nil/actor/network/http/request_parser.hh"
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
                        case 29:
                            goto st_case_29;
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
                    }
                    goto st_out;
                    p += 1;
                    if (p == pe)
                        goto _test_eof1;
                st_case_1:
                    if (65 <= ((*(p))) && ((*(p))) <= 90) {
                        goto _ctr2;
                    }
                    { goto _st0; }
                st_case_0:
                _st0:
                    _fsm_cs = 0;
                    goto _pop;
                _ctr2 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 178 "include/nil/actor/network/http/request_parser.hh"

                    goto _st2;
                _st2:
                    p += 1;
                    if (p == pe)
                        goto _test_eof2;
                st_case_2:
                    if (((*(p))) == 32) {
                        goto _ctr4;
                    }
                    if (65 <= ((*(p))) && ((*(p))) <= 90) {
                        goto _st2;
                    }
                    { goto _st0; }
                _ctr4 : {
#line 43 "src/http/request_parser.rl"

                    _req->_method = str();
                }

#line 202 "include/nil/actor/network/http/request_parser.hh"

                    goto _st3;
                _st3:
                    p += 1;
                    if (p == pe)
                        goto _test_eof3;
                st_case_3:
                    switch (((*(p)))) {
                        case 13: {
                            goto _ctr7;
                        }
                        case 32: {
                            goto _st0;
                        }
                    }
                    { goto _ctr6; }
                _ctr6 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 228 "include/nil/actor/network/http/request_parser.hh"

                    goto _st4;
                _st4:
                    p += 1;
                    if (p == pe)
                        goto _test_eof4;
                st_case_4:
                    switch (((*(p)))) {
                        case 13: {
                            goto _st5;
                        }
                        case 32: {
                            goto _ctr10;
                        }
                    }
                    { goto _st4; }
                _ctr7 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 254 "include/nil/actor/network/http/request_parser.hh"

                    goto _st5;
                _st5:
                    p += 1;
                    if (p == pe)
                        goto _test_eof5;
                st_case_5:
                    switch (((*(p)))) {
                        case 10: {
                            goto _st0;
                        }
                        case 13: {
                            goto _st5;
                        }
                        case 32: {
                            goto _ctr10;
                        }
                    }
                    { goto _st4; }
                _ctr10 : {
#line 47 "src/http/request_parser.rl"

                    _req->_url = str();
                }

#line 283 "include/nil/actor/network/http/request_parser.hh"

                    goto _st6;
                _st6:
                    p += 1;
                    if (p == pe)
                        goto _test_eof6;
                st_case_6:
                    if (((*(p))) == 72) {
                        goto _st7;
                    }
                    { goto _st0; }
                _st7:
                    p += 1;
                    if (p == pe)
                        goto _test_eof7;
                st_case_7:
                    if (((*(p))) == 84) {
                        goto _st8;
                    }
                    { goto _st0; }
                _st8:
                    p += 1;
                    if (p == pe)
                        goto _test_eof8;
                st_case_8:
                    if (((*(p))) == 84) {
                        goto _st9;
                    }
                    { goto _st0; }
                _st9:
                    p += 1;
                    if (p == pe)
                        goto _test_eof9;
                st_case_9:
                    if (((*(p))) == 80) {
                        goto _st10;
                    }
                    { goto _st0; }
                _st10:
                    p += 1;
                    if (p == pe)
                        goto _test_eof10;
                st_case_10:
                    if (((*(p))) == 47) {
                        goto _st11;
                    }
                    { goto _st0; }
                _st11:
                    p += 1;
                    if (p == pe)
                        goto _test_eof11;
                st_case_11:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _ctr17;
                    }
                    { goto _st0; }
                _ctr17 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 359 "include/nil/actor/network/http/request_parser.hh"

                    goto _st12;
                _st12:
                    p += 1;
                    if (p == pe)
                        goto _test_eof12;
                st_case_12:
                    if (((*(p))) == 46) {
                        goto _st13;
                    }
                    { goto _st0; }
                _st13:
                    p += 1;
                    if (p == pe)
                        goto _test_eof13;
                st_case_13:
                    if (48 <= ((*(p))) && ((*(p))) <= 57) {
                        goto _st14;
                    }
                    { goto _st0; }
                _st14:
                    p += 1;
                    if (p == pe)
                        goto _test_eof14;
                st_case_14:
                    if (((*(p))) == 13) {
                        goto _ctr21;
                    }
                    { goto _st0; }
                _ctr21 : {
#line 51 "src/http/request_parser.rl"

                    _req->_version = str();
                }

#line 402 "include/nil/actor/network/http/request_parser.hh"

                    goto _st15;
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
                            goto _ctr25;
                        }
                        case 124: {
                            goto _ctr25;
                        }
                        case 126: {
                            goto _ctr25;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr25;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr25;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr25;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr25;
                            }
                        } else {
                            goto _ctr25;
                        }
                    } else {
                        goto _ctr25;
                    }
                    { goto _st0; }
                _ctr41 : {
#line 78 "src/http/request_parser.rl"

                    if (_req->_headers.count(_field_name)) {
                        // RFC 7230, section 3.2.2.  Field Parsing:
                        // A recipient MAY combine multiple header fields with the same field name into one
                        // "field-name: field-value" pair, without changing the semantics of the message,
                        // by appending each subsequent field value to the combined field value in order, separated by a
                        // comma.
                        _req->_headers[_field_name] += sstring(",") + std::move(_value);
                    } else {
                        _req->_headers[_field_name] = std::move(_value);
                    }
                }

#line 476 "include/nil/actor/network/http/request_parser.hh"

                    goto _st17;
                _ctr55 : {
#line 90 "src/http/request_parser.rl"

                    // RFC 7230, section 3.2.4.  Field Order:
                    // A server that receives an obs-fold in a request message that is not
                    // within a message/http container MUST either reject the message [...]
                    // or replace each received obs-fold with one or more SP octets [...]
                    _req->_headers[_field_name] += sstring(" ") + std::move(_value);
                }

#line 490 "include/nil/actor/network/http/request_parser.hh"

                    goto _st17;
                _st17:
                    p += 1;
                    if (p == pe)
                        goto _test_eof17;
                st_case_17:
                    if (((*(p))) == 10) {
                        goto _ctr26;
                    }
                    { goto _st0; }
                _ctr26 : {
#line 98 "src/http/request_parser.rl"

                    done = true;
                    {
                        p += 1;
                        _fsm_cs = 29;
                        goto _out;
                    }
                }

#line 512 "include/nil/actor/network/http/request_parser.hh"

                    goto _st29;
                _st29:
                    p += 1;
                    if (p == pe)
                        goto _test_eof29;
                st_case_29 : { goto _st0; }
                _ctr25 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 530 "include/nil/actor/network/http/request_parser.hh"

                    goto _st18;
                _ctr42 : {
#line 78 "src/http/request_parser.rl"

                    if (_req->_headers.count(_field_name)) {
                        // RFC 7230, section 3.2.2.  Field Parsing:
                        // A recipient MAY combine multiple header fields with the same field name into one
                        // "field-name: field-value" pair, without changing the semantics of the message,
                        // by appending each subsequent field value to the combined field value in order, separated by a
                        // comma.
                        _req->_headers[_field_name] += sstring(",") + std::move(_value);
                    } else {
                        _req->_headers[_field_name] = std::move(_value);
                    }
                }

#line 548 "include/nil/actor/network/http/request_parser.hh"

                    {
#line 39 "src/http/request_parser.rl"

                        g.mark_start(p);
                    }

#line 556 "include/nil/actor/network/http/request_parser.hh"

                    goto _st18;
                _ctr56 : {
#line 90 "src/http/request_parser.rl"

                    // RFC 7230, section 3.2.4.  Field Order:
                    // A server that receives an obs-fold in a request message that is not
                    // within a message/http container MUST either reject the message [...]
                    // or replace each received obs-fold with one or more SP octets [...]
                    _req->_headers[_field_name] += sstring(" ") + std::move(_value);
                }

#line 570 "include/nil/actor/network/http/request_parser.hh"

                    {
#line 39 "src/http/request_parser.rl"

                        g.mark_start(p);
                    }

#line 578 "include/nil/actor/network/http/request_parser.hh"

                    goto _st18;
                _st18:
                    p += 1;
                    if (p == pe)
                        goto _test_eof18;
                st_case_18:
                    switch (((*(p)))) {
                        case 33: {
                            goto _st18;
                        }
                        case 58: {
                            goto _ctr28;
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
                _ctr28 : {
#line 55 "src/http/request_parser.rl"

                    _field_name = str();
                }

#line 633 "include/nil/actor/network/http/request_parser.hh"

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
                        case 13: {
                            goto _ctr31;
                        }
                        case 32: {
                            goto _st19;
                        }
                        case 127: {
                            goto _st0;
                        }
                    }
                    if (0 <= ((*(p))) && ((*(p))) <= 31) {
                        goto _st0;
                    }
                    { goto _ctr30; }
                _ctr30 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 668 "include/nil/actor/network/http/request_parser.hh"

                    goto _st20;
                _ctr33 : {
#line 68 "src/http/request_parser.rl"

                    // Needs a start to be marked beforehand.
                    // Used to mark the candidate end of value string. Can be moved furhter by repetitive use
                    // of this action.
                    // To store the string that ends on the last checkpoint (instead of the last processed character)
                    // use %no_mark_store_value instead of %store_value
                    g.mark_end(p);
                    g.mark_start(p);
                }

#line 684 "include/nil/actor/network/http/request_parser.hh"

                    goto _st20;
                _st20:
                    p += 1;
                    if (p == pe)
                        goto _test_eof20;
                st_case_20:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr34;
                        }
                        case 13: {
                            goto _ctr35;
                        }
                        case 32: {
                            goto _ctr34;
                        }
                        case 127: {
                            goto _st0;
                        }
                    }
                    if (0 <= ((*(p))) && ((*(p))) <= 31) {
                        goto _st0;
                    }
                    { goto _ctr33; }
                _ctr34 : {
#line 68 "src/http/request_parser.rl"

                    // Needs a start to be marked beforehand.
                    // Used to mark the candidate end of value string. Can be moved furhter by repetitive use
                    // of this action.
                    // To store the string that ends on the last checkpoint (instead of the last processed character)
                    // use %no_mark_store_value instead of %store_value
                    g.mark_end(p);
                    g.mark_start(p);
                }

#line 725 "include/nil/actor/network/http/request_parser.hh"

                    goto _st21;
                _st21:
                    p += 1;
                    if (p == pe)
                        goto _test_eof21;
                st_case_21:
                    switch (((*(p)))) {
                        case 9: {
                            goto _st21;
                        }
                        case 13: {
                            goto _ctr37;
                        }
                        case 32: {
                            goto _st21;
                        }
                        case 127: {
                            goto _st0;
                        }
                    }
                    if (0 <= ((*(p))) && ((*(p))) <= 31) {
                        goto _st0;
                    }
                    { goto _st20; }
                _ctr31 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 760 "include/nil/actor/network/http/request_parser.hh"

                    {
#line 63 "src/http/request_parser.rl"

                        _value = get_str();
                        g.mark_start(nullptr);
                    }

#line 769 "include/nil/actor/network/http/request_parser.hh"

                    goto _st22;
                _ctr35 : {
#line 68 "src/http/request_parser.rl"

                    // Needs a start to be marked beforehand.
                    // Used to mark the candidate end of value string. Can be moved furhter by repetitive use
                    // of this action.
                    // To store the string that ends on the last checkpoint (instead of the last processed character)
                    // use %no_mark_store_value instead of %store_value
                    g.mark_end(p);
                    g.mark_start(p);
                }

#line 785 "include/nil/actor/network/http/request_parser.hh"

                    {
#line 63 "src/http/request_parser.rl"

                        _value = get_str();
                        g.mark_start(nullptr);
                    }

#line 794 "include/nil/actor/network/http/request_parser.hh"

                    goto _st22;
                _ctr37 : {
#line 63 "src/http/request_parser.rl"

                    _value = get_str();
                    g.mark_start(nullptr);
                }

#line 805 "include/nil/actor/network/http/request_parser.hh"

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
                            goto _ctr40;
                        }
                        case 13: {
                            goto _ctr41;
                        }
                        case 32: {
                            goto _ctr40;
                        }
                        case 33: {
                            goto _ctr42;
                        }
                        case 124: {
                            goto _ctr42;
                        }
                        case 126: {
                            goto _ctr42;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr42;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr42;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr42;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr42;
                            }
                        } else {
                            goto _ctr42;
                        }
                    } else {
                        goto _ctr42;
                    }
                    { goto _st0; }
                _ctr40 : {
#line 78 "src/http/request_parser.rl"

                    if (_req->_headers.count(_field_name)) {
                        // RFC 7230, section 3.2.2.  Field Parsing:
                        // A recipient MAY combine multiple header fields with the same field name into one
                        // "field-name: field-value" pair, without changing the semantics of the message,
                        // by appending each subsequent field value to the combined field value in order, separated by a
                        // comma.
                        _req->_headers[_field_name] += sstring(",") + std::move(_value);
                    } else {
                        _req->_headers[_field_name] = std::move(_value);
                    }
                }

#line 885 "include/nil/actor/network/http/request_parser.hh"

                    goto _st24;
                _ctr54 : {
#line 90 "src/http/request_parser.rl"

                    // RFC 7230, section 3.2.4.  Field Order:
                    // A server that receives an obs-fold in a request message that is not
                    // within a message/http container MUST either reject the message [...]
                    // or replace each received obs-fold with one or more SP octets [...]
                    _req->_headers[_field_name] += sstring(" ") + std::move(_value);
                }

#line 899 "include/nil/actor/network/http/request_parser.hh"

                    goto _st24;
                _st24:
                    p += 1;
                    if (p == pe)
                        goto _test_eof24;
                st_case_24:
                    switch (((*(p)))) {
                        case 9: {
                            goto _st24;
                        }
                        case 13: {
                            goto _ctr45;
                        }
                        case 32: {
                            goto _st24;
                        }
                        case 127: {
                            goto _st0;
                        }
                    }
                    if (0 <= ((*(p))) && ((*(p))) <= 31) {
                        goto _st0;
                    }
                    { goto _ctr44; }
                _ctr44 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 934 "include/nil/actor/network/http/request_parser.hh"

                    goto _st25;
                _ctr47 : {
#line 68 "src/http/request_parser.rl"

                    // Needs a start to be marked beforehand.
                    // Used to mark the candidate end of value string. Can be moved furhter by repetitive use
                    // of this action.
                    // To store the string that ends on the last checkpoint (instead of the last processed character)
                    // use %no_mark_store_value instead of %store_value
                    g.mark_end(p);
                    g.mark_start(p);
                }

#line 950 "include/nil/actor/network/http/request_parser.hh"

                    goto _st25;
                _st25:
                    p += 1;
                    if (p == pe)
                        goto _test_eof25;
                st_case_25:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr48;
                        }
                        case 13: {
                            goto _ctr49;
                        }
                        case 32: {
                            goto _ctr48;
                        }
                        case 127: {
                            goto _st0;
                        }
                    }
                    if (0 <= ((*(p))) && ((*(p))) <= 31) {
                        goto _st0;
                    }
                    { goto _ctr47; }
                _ctr48 : {
#line 68 "src/http/request_parser.rl"

                    // Needs a start to be marked beforehand.
                    // Used to mark the candidate end of value string. Can be moved furhter by repetitive use
                    // of this action.
                    // To store the string that ends on the last checkpoint (instead of the last processed character)
                    // use %no_mark_store_value instead of %store_value
                    g.mark_end(p);
                    g.mark_start(p);
                }

#line 991 "include/nil/actor/network/http/request_parser.hh"

                    goto _st26;
                _st26:
                    p += 1;
                    if (p == pe)
                        goto _test_eof26;
                st_case_26:
                    switch (((*(p)))) {
                        case 9: {
                            goto _st26;
                        }
                        case 13: {
                            goto _ctr51;
                        }
                        case 32: {
                            goto _st26;
                        }
                        case 127: {
                            goto _st0;
                        }
                    }
                    if (0 <= ((*(p))) && ((*(p))) <= 31) {
                        goto _st0;
                    }
                    { goto _st25; }
                _ctr45 : {
#line 39 "src/http/request_parser.rl"

                    g.mark_start(p);
                }

#line 1026 "include/nil/actor/network/http/request_parser.hh"

                    {
#line 63 "src/http/request_parser.rl"

                        _value = get_str();
                        g.mark_start(nullptr);
                    }

#line 1035 "include/nil/actor/network/http/request_parser.hh"

                    goto _st27;
                _ctr49 : {
#line 68 "src/http/request_parser.rl"

                    // Needs a start to be marked beforehand.
                    // Used to mark the candidate end of value string. Can be moved furhter by repetitive use
                    // of this action.
                    // To store the string that ends on the last checkpoint (instead of the last processed character)
                    // use %no_mark_store_value instead of %store_value
                    g.mark_end(p);
                    g.mark_start(p);
                }

#line 1051 "include/nil/actor/network/http/request_parser.hh"

                    {
#line 63 "src/http/request_parser.rl"

                        _value = get_str();
                        g.mark_start(nullptr);
                    }

#line 1060 "include/nil/actor/network/http/request_parser.hh"

                    goto _st27;
                _ctr51 : {
#line 63 "src/http/request_parser.rl"

                    _value = get_str();
                    g.mark_start(nullptr);
                }

#line 1071 "include/nil/actor/network/http/request_parser.hh"

                    goto _st27;
                _st27:
                    p += 1;
                    if (p == pe)
                        goto _test_eof27;
                st_case_27:
                    if (((*(p))) == 10) {
                        goto _st28;
                    }
                    { goto _st0; }
                _st28:
                    p += 1;
                    if (p == pe)
                        goto _test_eof28;
                st_case_28:
                    switch (((*(p)))) {
                        case 9: {
                            goto _ctr54;
                        }
                        case 13: {
                            goto _ctr55;
                        }
                        case 32: {
                            goto _ctr54;
                        }
                        case 33: {
                            goto _ctr56;
                        }
                        case 124: {
                            goto _ctr56;
                        }
                        case 126: {
                            goto _ctr56;
                        }
                    }
                    if (((*(p))) < 45) {
                        if (((*(p))) > 39) {
                            if (42 <= ((*(p))) && ((*(p))) <= 43) {
                                goto _ctr56;
                            }
                        } else if (((*(p))) >= 35) {
                            goto _ctr56;
                        }
                    } else if (((*(p))) > 46) {
                        if (((*(p))) < 65) {
                            if (48 <= ((*(p))) && ((*(p))) <= 57) {
                                goto _ctr56;
                            }
                        } else if (((*(p))) > 90) {
                            if (94 <= ((*(p))) && ((*(p))) <= 122) {
                                goto _ctr56;
                            }
                        } else {
                            goto _ctr56;
                        }
                    } else {
                        goto _ctr56;
                    }
                    { goto _st0; }
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
                _test_eof29:
                    _fsm_cs = 29;
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

                _test_eof : { }
                    if (_fsm_cs >= 29)
                        goto _out;
                _pop : { }
                _out : { }
                }

#line 162 "src/http/request_parser.rl"

                //#ifdef __clang__
                //#pragma clang diagnostic pop
                //#endif
                if (!done) {
                    if (p == eof) {
                        _state = state::eof;
                    } else if (p != pe) {
                        _state = state::error;
                    } else {
                        p = nullptr;
                    }
                } else {
                    _state = state::done;
                }
                return p;
            }
            auto get_parsed_request() {
                return std::move(_req);
            }
            bool eof() const {
                return _state == state::eof;
            }
            bool failed() const {
                return _state == state::error;
            }
        };

    }    // namespace actor
}    // namespace nil
