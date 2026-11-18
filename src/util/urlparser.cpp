
#line 1 "url_parser.rl"
/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
// url_parser.rl
// 用法: ragel -G2 -o url_parser.cpp url_parser.rl
// 或者: ragel -T0 -o url_parser.cpp url_parser.rl  (表驱动)

#include <iostream>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include "src/util/url.h"
#include "src/util/urlutils.h"

#line 33 "urlparser.cpp"
static const int url_parser_start = 187;
static const int url_parser_first_final = 187;
static const int url_parser_error = 0;

static const int url_parser_en_main = 187;

#line 32 "url_parser.rl"

namespace blue
{
	std::shared_ptr<Url> Url::CreateUrl(const std::string &url_str)
	{
		try
		{
			auto url = std::make_shared<Url>();

			int cs;
			const char *p = url_str.c_str();
			const char *pe = p + url_str.size();
			const char *eof = pe;

			const char *scheme_start = nullptr;
			const char *userinfo_start = nullptr;
			const char *host_start = nullptr;
			const char *port_start = nullptr;
			const char *path_start = nullptr;
			const char *query_start = nullptr;
			const char *fragment_start = nullptr;

			int host_type = 0; // 0=域名, 4=IPv4, 6=IPv6

#line 68 "urlparser.cpp"
			{
				cs = url_parser_start;
			}

#line 73 "urlparser.cpp"
			{
				if (p == pe)
					goto _test_eof;
				switch (cs)
				{
				case 187:
					switch ((*p))
					{
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 63:
						goto tr209;
					}
					if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto tr210;
					}
					else if ((*p) >= 65)
						goto tr210;
					goto st0;
				st0:
					cs = 0;
					goto _out;
				tr207:
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st188;
				tr224:
#line 63 "url_parser.rl"
					{
						query_start = p;
					}
					goto st188;
				tr232:
#line 61 "url_parser.rl"
					{
						port_start = p;
					}
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st188;
				tr253:
#line 65 "url_parser.rl"
					{
						host_type = 4;
					}
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st188;
				st188:
					if (++p == pe)
						goto _test_eof188;
				case 188:
#line 118 "urlparser.cpp"
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto tr211;
					}
					else if ((*p) > -17)
					{
						if ((*p) > -12)
						{
							if (32 <= (*p) && (*p) <= 126)
								goto tr214;
						}
						else if ((*p) >= -16)
							goto tr213;
					}
					else
						goto tr212;
					goto st0;
				tr211:
#line 64 "url_parser.rl"
					{
						fragment_start = p;
					}
					goto st1;
				st1:
					if (++p == pe)
						goto _test_eof1;
				case 1:
#line 139 "urlparser.cpp"
					if ((*p) <= -65)
						goto st189;
					goto st0;
				tr214:
#line 64 "url_parser.rl"
					{
						fragment_start = p;
					}
					goto st189;
				st189:
					if (++p == pe)
						goto _test_eof189;
				case 189:
#line 151 "urlparser.cpp"
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st1;
					}
					else if ((*p) > -17)
					{
						if ((*p) > -12)
						{
							if (32 <= (*p) && (*p) <= 126)
								goto st189;
						}
						else if ((*p) >= -16)
							goto st3;
					}
					else
						goto st2;
					goto st0;
				tr212:
#line 64 "url_parser.rl"
					{
						fragment_start = p;
					}
					goto st2;
				st2:
					if (++p == pe)
						goto _test_eof2;
				case 2:
#line 172 "urlparser.cpp"
					if ((*p) <= -65)
						goto st1;
					goto st0;
				tr213:
#line 64 "url_parser.rl"
					{
						fragment_start = p;
					}
					goto st3;
				st3:
					if (++p == pe)
						goto _test_eof3;
				case 3:
#line 184 "urlparser.cpp"
					if ((*p) <= -65)
						goto st2;
					goto st0;
				tr208:
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st190;
				tr233:
#line 61 "url_parser.rl"
					{
						port_start = p;
					}
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st190;
				tr254:
#line 65 "url_parser.rl"
					{
						host_type = 4;
					}
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st190;
				st190:
					if (++p == pe)
						goto _test_eof190;
				case 190:
#line 208 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st190;
					case 35:
						goto st188;
					case 37:
						goto st7;
					case 61:
						goto st190;
					case 63:
						goto st191;
					case 95:
						goto st190;
					case 126:
						goto st190;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st5;
						}
						else if ((*p) >= -62)
							goto st4;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 64)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st190;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st190;
						}
						else
							goto st190;
					}
					else
						goto st6;
					goto st0;
				st4:
					if (++p == pe)
						goto _test_eof4;
				case 4:
					if ((*p) <= -65)
						goto st190;
					goto st0;
				st5:
					if (++p == pe)
						goto _test_eof5;
				case 5:
					if ((*p) <= -65)
						goto st4;
					goto st0;
				st6:
					if (++p == pe)
						goto _test_eof6;
				case 6:
					if ((*p) <= -65)
						goto st5;
					goto st0;
				st7:
					if (++p == pe)
						goto _test_eof7;
				case 7:
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st8;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st8;
					}
					else
						goto st8;
					goto st0;
				st8:
					if (++p == pe)
						goto _test_eof8;
				case 8:
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st190;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st190;
					}
					else
						goto st190;
					goto st0;
				tr209:
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st191;
				tr235:
#line 61 "url_parser.rl"
					{
						port_start = p;
					}
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st191;
				tr256:
#line 65 "url_parser.rl"
					{
						host_type = 4;
					}
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					goto st191;
				st191:
					if (++p == pe)
						goto _test_eof191;
				case 191:
#line 303 "urlparser.cpp"
					if ((*p) == 35)
						goto tr224;
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto tr220;
					}
					else if ((*p) > -17)
					{
						if ((*p) > -12)
						{
							if (32 <= (*p) && (*p) <= 126)
								goto tr223;
						}
						else if ((*p) >= -16)
							goto tr222;
					}
					else
						goto tr221;
					goto st0;
				tr220:
#line 63 "url_parser.rl"
					{
						query_start = p;
					}
					goto st9;
				st9:
					if (++p == pe)
						goto _test_eof9;
				case 9:
#line 326 "urlparser.cpp"
					if ((*p) <= -65)
						goto st192;
					goto st0;
				tr223:
#line 63 "url_parser.rl"
					{
						query_start = p;
					}
					goto st192;
				st192:
					if (++p == pe)
						goto _test_eof192;
				case 192:
#line 338 "urlparser.cpp"
					if ((*p) == 35)
						goto st188;
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st9;
					}
					else if ((*p) > -17)
					{
						if ((*p) > -12)
						{
							if (32 <= (*p) && (*p) <= 126)
								goto st192;
						}
						else if ((*p) >= -16)
							goto st11;
					}
					else
						goto st10;
					goto st0;
				tr221:
#line 63 "url_parser.rl"
					{
						query_start = p;
					}
					goto st10;
				st10:
					if (++p == pe)
						goto _test_eof10;
				case 10:
#line 361 "urlparser.cpp"
					if ((*p) <= -65)
						goto st9;
					goto st0;
				tr222:
#line 63 "url_parser.rl"
					{
						query_start = p;
					}
					goto st11;
				st11:
					if (++p == pe)
						goto _test_eof11;
				case 11:
#line 373 "urlparser.cpp"
					if ((*p) <= -65)
						goto st10;
					goto st0;
				tr210:
#line 58 "url_parser.rl"
					{
						scheme_start = p;
					}
					goto st12;
				st12:
					if (++p == pe)
						goto _test_eof12;
				case 12:
#line 385 "urlparser.cpp"
					switch ((*p))
					{
					case 43:
						goto st12;
					case 58:
						goto st13;
					}
					if ((*p) < 48)
					{
						if (45 <= (*p) && (*p) <= 46)
							goto st12;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st12;
						}
						else if ((*p) >= 65)
							goto st12;
					}
					else
						goto st12;
					goto st0;
				st13:
					if (++p == pe)
						goto _test_eof13;
				case 13:
					if ((*p) == 47)
						goto st14;
					goto st0;
				st14:
					if (++p == pe)
						goto _test_eof14;
				case 14:
					if ((*p) == 47)
						goto st15;
					goto st0;
				st15:
					if (++p == pe)
						goto _test_eof15;
				case 15:
					switch ((*p))
					{
					case 33:
						goto tr18;
					case 37:
						goto tr19;
					case 48:
						goto tr20;
					case 49:
						goto tr21;
					case 50:
						goto tr22;
					case 58:
						goto tr24;
					case 59:
						goto tr18;
					case 61:
						goto tr18;
					case 64:
						goto tr25;
					case 91:
						goto tr26;
					case 95:
						goto tr18;
					case 126:
						goto tr18;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto tr15;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto tr17;
						}
						else
							goto tr16;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto tr23;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto tr18;
						}
						else
							goto tr18;
					}
					else
						goto tr18;
					goto st0;
				tr15:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st16;
				st16:
					if (++p == pe)
						goto _test_eof16;
				case 16:
#line 465 "urlparser.cpp"
					if ((*p) <= -65)
						goto st193;
					goto st0;
				tr18:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st193;
				st193:
					if (++p == pe)
						goto _test_eof193;
				case 193:
#line 479 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st17;
						}
						else if ((*p) >= -62)
							goto st16;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st193;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st18;
					goto st0;
				tr16:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st17;
				st17:
					if (++p == pe)
						goto _test_eof17;
				case 17:
#line 520 "urlparser.cpp"
					if ((*p) <= -65)
						goto st16;
					goto st0;
				tr17:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st18;
				st18:
					if (++p == pe)
						goto _test_eof18;
				case 18:
#line 534 "urlparser.cpp"
					if ((*p) <= -65)
						goto st17;
					goto st0;
				tr19:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st194;
				st194:
					if (++p == pe)
						goto _test_eof194;
				case 194:
#line 548 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 48)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if ((*p) > -12)
							{
								if (36 <= (*p) && (*p) <= 46)
									goto st195;
							}
							else if ((*p) >= -16)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 57)
					{
						if ((*p) < 71)
						{
							if (65 <= (*p) && (*p) <= 70)
								goto st198;
						}
						else if ((*p) > 90)
						{
							if ((*p) > 102)
							{
								if (103 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 97)
								goto st198;
						}
						else
							goto st195;
					}
					else
						goto st198;
					goto st0;
				tr40:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st19;
				st19:
					if (++p == pe)
						goto _test_eof19;
				case 19:
#line 595 "urlparser.cpp"
					if ((*p) <= -65)
						goto st195;
					goto st0;
				tr43:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st195;
				st195:
					if (++p == pe)
						goto _test_eof195;
				case 195:
#line 607 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st20;
						}
						else if ((*p) >= -62)
							goto st19;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st195;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st21;
					goto st0;
				tr41:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st20;
				st20:
					if (++p == pe)
						goto _test_eof20;
				case 20:
#line 644 "urlparser.cpp"
					if ((*p) <= -65)
						goto st19;
					goto st0;
				tr42:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st21;
				st21:
					if (++p == pe)
						goto _test_eof21;
				case 21:
#line 656 "urlparser.cpp"
					if ((*p) <= -65)
						goto st20;
					goto st0;
				tr255:
#line 65 "url_parser.rl"
					{
						host_type = 4;
					}
					goto st196;
				st196:
					if (++p == pe)
						goto _test_eof196;
				case 196:
#line 668 "urlparser.cpp"
					switch ((*p))
					{
					case 35:
						goto tr232;
					case 47:
						goto tr233;
					case 63:
						goto tr235;
					}
					if (48 <= (*p) && (*p) <= 57)
						goto tr234;
					goto st0;
				tr234:
#line 61 "url_parser.rl"
					{
						port_start = p;
					}
					goto st197;
				st197:
					if (++p == pe)
						goto _test_eof197;
				case 197:
#line 685 "urlparser.cpp"
					switch ((*p))
					{
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 63:
						goto tr209;
					}
					if (48 <= (*p) && (*p) <= 57)
						goto st197;
					goto st0;
				st198:
					if (++p == pe)
						goto _test_eof198;
				case 198:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 48)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if ((*p) > -12)
							{
								if (36 <= (*p) && (*p) <= 46)
									goto st195;
							}
							else if ((*p) >= -16)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 57)
					{
						if ((*p) < 71)
						{
							if (65 <= (*p) && (*p) <= 70)
								goto st193;
						}
						else if ((*p) > 90)
						{
							if ((*p) > 102)
							{
								if (103 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 97)
								goto st193;
						}
						else
							goto st195;
					}
					else
						goto st193;
					goto st0;
				tr279:
#line 65 "url_parser.rl"
					{
						host_type = 4;
					}
					goto st199;
				st199:
					if (++p == pe)
						goto _test_eof199;
				case 199:
#line 744 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st23;
					case 35:
						goto tr232;
					case 37:
						goto st26;
					case 47:
						goto tr233;
					case 61:
						goto st23;
					case 63:
						goto tr235;
					case 64:
						goto st28;
					case 95:
						goto st23;
					case 126:
						goto st23;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st22;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st25;
						}
						else
							goto st24;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 58)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto tr237;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st23;
							}
							else if ((*p) >= 65)
								goto st23;
						}
						else
							goto st23;
					}
					else
						goto st23;
					goto st0;
				st22:
					if (++p == pe)
						goto _test_eof22;
				case 22:
					if ((*p) <= -65)
						goto st23;
					goto st0;
				tr24:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
					goto st23;
				st23:
					if (++p == pe)
						goto _test_eof23;
				case 23:
#line 795 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st23;
					case 37:
						goto st26;
					case 61:
						goto st23;
					case 64:
						goto st28;
					case 95:
						goto st23;
					case 126:
						goto st23;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st22;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st25;
						}
						else
							goto st24;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 59)
								goto st23;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st23;
						}
						else
							goto st23;
					}
					else
						goto st23;
					goto st0;
				st24:
					if (++p == pe)
						goto _test_eof24;
				case 24:
					if ((*p) <= -65)
						goto st22;
					goto st0;
				st25:
					if (++p == pe)
						goto _test_eof25;
				case 25:
					if ((*p) <= -65)
						goto st24;
					goto st0;
				st26:
					if (++p == pe)
						goto _test_eof26;
				case 26:
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st27;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st27;
					}
					else
						goto st27;
					goto st0;
				st27:
					if (++p == pe)
						goto _test_eof27;
				case 27:
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st23;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st23;
					}
					else
						goto st23;
					goto st0;
				tr25:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
					goto st28;
				st28:
					if (++p == pe)
						goto _test_eof28;
				case 28:
#line 873 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto tr43;
					case 48:
						goto tr44;
					case 49:
						goto tr45;
					case 50:
						goto tr46;
					case 59:
						goto tr43;
					case 61:
						goto tr43;
					case 91:
						goto tr26;
					case 95:
						goto tr43;
					case 126:
						goto tr43;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto tr40;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto tr42;
						}
						else
							goto tr41;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto tr47;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto tr43;
						}
						else
							goto tr43;
					}
					else
						goto tr43;
					goto st0;
				tr44:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st200;
				st200:
					if (++p == pe)
						goto _test_eof200;
				case 200:
#line 914 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st201;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st20;
						}
						else if ((*p) >= -62)
							goto st19;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st195;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st21;
					goto st0;
				st201:
					if (++p == pe)
						goto _test_eof201;
				case 201:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 48:
						goto st202;
					case 49:
						goto st215;
					case 50:
						goto st217;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto st216;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st202:
					if (++p == pe)
						goto _test_eof202;
				case 202:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st203;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st20;
						}
						else if ((*p) >= -62)
							goto st19;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st195;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st21;
					goto st0;
				st203:
					if (++p == pe)
						goto _test_eof203;
				case 203:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 48:
						goto st204;
					case 49:
						goto st211;
					case 50:
						goto st213;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto st212;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st204:
					if (++p == pe)
						goto _test_eof204;
				case 204:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st205;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st20;
						}
						else if ((*p) >= -62)
							goto st19;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st195;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st21;
					goto st0;
				st205:
					if (++p == pe)
						goto _test_eof205;
				case 205:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 48:
						goto st206;
					case 49:
						goto st207;
					case 50:
						goto st209;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto st208;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st206:
					if (++p == pe)
						goto _test_eof206;
				case 206:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr253;
					case 47:
						goto tr254;
					case 58:
						goto tr255;
					case 61:
						goto st195;
					case 63:
						goto tr256;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st20;
						}
						else if ((*p) >= -62)
							goto st19;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st195;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st21;
					goto st0;
				st207:
					if (++p == pe)
						goto _test_eof207;
				case 207:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr253;
					case 47:
						goto tr254;
					case 58:
						goto tr255;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr256;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st208;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st208:
					if (++p == pe)
						goto _test_eof208;
				case 208:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr253;
					case 47:
						goto tr254;
					case 58:
						goto tr255;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr256;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st206;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st209:
					if (++p == pe)
						goto _test_eof209;
				case 209:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr253;
					case 47:
						goto tr254;
					case 53:
						goto st210;
					case 58:
						goto tr255;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr256;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st208;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st206;
					}
					else
						goto st195;
					goto st0;
				st210:
					if (++p == pe)
						goto _test_eof210;
				case 210:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr253;
					case 47:
						goto tr254;
					case 58:
						goto tr255;
					case 61:
						goto st195;
					case 63:
						goto tr256;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st206;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st211:
					if (++p == pe)
						goto _test_eof211;
				case 211:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st205;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st212;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st212:
					if (++p == pe)
						goto _test_eof212;
				case 212:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st205;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st204;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st213:
					if (++p == pe)
						goto _test_eof213;
				case 213:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st205;
					case 47:
						goto tr208;
					case 53:
						goto st214;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st212;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st204;
					}
					else
						goto st195;
					goto st0;
				st214:
					if (++p == pe)
						goto _test_eof214;
				case 214:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st205;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st204;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st215:
					if (++p == pe)
						goto _test_eof215;
				case 215:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st203;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st216;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st216:
					if (++p == pe)
						goto _test_eof216;
				case 216:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st203;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st202;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				st217:
					if (++p == pe)
						goto _test_eof217;
				case 217:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st203;
					case 47:
						goto tr208;
					case 53:
						goto st218;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st216;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st202;
					}
					else
						goto st195;
					goto st0;
				st218:
					if (++p == pe)
						goto _test_eof218;
				case 218:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st203;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st202;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				tr45:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st219;
				st219:
					if (++p == pe)
						goto _test_eof219;
				case 219:
#line 1625 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st201;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st220;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				tr47:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st220;
				st220:
					if (++p == pe)
						goto _test_eof220;
				case 220:
#line 1667 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st201;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st200;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				tr46:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st221;
				st221:
					if (++p == pe)
						goto _test_eof221;
				case 221:
#line 1709 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st201;
					case 47:
						goto tr208;
					case 53:
						goto st222;
					case 58:
						goto st196;
					case 59:
						goto st195;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st220;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st200;
					}
					else
						goto st195;
					goto st0;
				st222:
					if (++p == pe)
						goto _test_eof222;
				case 222:
					switch ((*p))
					{
					case 33:
						goto st195;
					case 35:
						goto tr207;
					case 46:
						goto st201;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 61:
						goto st195;
					case 63:
						goto tr209;
					case 95:
						goto st195;
					case 126:
						goto st195;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st19;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st21;
						}
						else
							goto st20;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st200;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st195;
							}
							else if ((*p) >= 65)
								goto st195;
						}
						else
							goto st195;
					}
					else
						goto st195;
					goto st0;
				tr26:
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st29;
				st29:
					if (++p == pe)
						goto _test_eof29;
				case 29:
#line 1794 "urlparser.cpp"
					switch ((*p))
					{
					case 58:
						goto st165;
					case 118:
						goto st180;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st30;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st30;
					}
					else
						goto st30;
					goto st0;
				st30:
					if (++p == pe)
						goto _test_eof30;
				case 30:
					if ((*p) == 58)
						goto st34;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st31;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st31;
					}
					else
						goto st31;
					goto st0;
				st31:
					if (++p == pe)
						goto _test_eof31;
				case 31:
					if ((*p) == 58)
						goto st34;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st32;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st32;
					}
					else
						goto st32;
					goto st0;
				st32:
					if (++p == pe)
						goto _test_eof32;
				case 32:
					if ((*p) == 58)
						goto st34;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st33;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st33;
					}
					else
						goto st33;
					goto st0;
				st33:
					if (++p == pe)
						goto _test_eof33;
				case 33:
					if ((*p) == 58)
						goto st34;
					goto st0;
				st34:
					if (++p == pe)
						goto _test_eof34;
				case 34:
					if ((*p) == 58)
						goto st151;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st35;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st35;
					}
					else
						goto st35;
					goto st0;
				st35:
					if (++p == pe)
						goto _test_eof35;
				case 35:
					if ((*p) == 58)
						goto st39;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st36;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st36;
					}
					else
						goto st36;
					goto st0;
				st36:
					if (++p == pe)
						goto _test_eof36;
				case 36:
					if ((*p) == 58)
						goto st39;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st37;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st37;
					}
					else
						goto st37;
					goto st0;
				st37:
					if (++p == pe)
						goto _test_eof37;
				case 37:
					if ((*p) == 58)
						goto st39;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st38;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st38;
					}
					else
						goto st38;
					goto st0;
				st38:
					if (++p == pe)
						goto _test_eof38;
				case 38:
					if ((*p) == 58)
						goto st39;
					goto st0;
				st39:
					if (++p == pe)
						goto _test_eof39;
				case 39:
					if ((*p) == 58)
						goto st137;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st40;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st40;
					}
					else
						goto st40;
					goto st0;
				st40:
					if (++p == pe)
						goto _test_eof40;
				case 40:
					if ((*p) == 58)
						goto st44;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st41;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st41;
					}
					else
						goto st41;
					goto st0;
				st41:
					if (++p == pe)
						goto _test_eof41;
				case 41:
					if ((*p) == 58)
						goto st44;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st42;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st42;
					}
					else
						goto st42;
					goto st0;
				st42:
					if (++p == pe)
						goto _test_eof42;
				case 42:
					if ((*p) == 58)
						goto st44;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st43;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st43;
					}
					else
						goto st43;
					goto st0;
				st43:
					if (++p == pe)
						goto _test_eof43;
				case 43:
					if ((*p) == 58)
						goto st44;
					goto st0;
				st44:
					if (++p == pe)
						goto _test_eof44;
				case 44:
					if ((*p) == 58)
						goto st123;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st45;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st45;
					}
					else
						goto st45;
					goto st0;
				st45:
					if (++p == pe)
						goto _test_eof45;
				case 45:
					if ((*p) == 58)
						goto st49;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st46;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st46;
					}
					else
						goto st46;
					goto st0;
				st46:
					if (++p == pe)
						goto _test_eof46;
				case 46:
					if ((*p) == 58)
						goto st49;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st47;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st47;
					}
					else
						goto st47;
					goto st0;
				st47:
					if (++p == pe)
						goto _test_eof47;
				case 47:
					if ((*p) == 58)
						goto st49;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st48;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st48;
					}
					else
						goto st48;
					goto st0;
				st48:
					if (++p == pe)
						goto _test_eof48;
				case 48:
					if ((*p) == 58)
						goto st49;
					goto st0;
				st49:
					if (++p == pe)
						goto _test_eof49;
				case 49:
					if ((*p) == 58)
						goto st109;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st50;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st50;
					}
					else
						goto st50;
					goto st0;
				st50:
					if (++p == pe)
						goto _test_eof50;
				case 50:
					if ((*p) == 58)
						goto st54;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st51;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st51;
					}
					else
						goto st51;
					goto st0;
				st51:
					if (++p == pe)
						goto _test_eof51;
				case 51:
					if ((*p) == 58)
						goto st54;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st52;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st52;
					}
					else
						goto st52;
					goto st0;
				st52:
					if (++p == pe)
						goto _test_eof52;
				case 52:
					if ((*p) == 58)
						goto st54;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st53;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st53;
					}
					else
						goto st53;
					goto st0;
				st53:
					if (++p == pe)
						goto _test_eof53;
				case 53:
					if ((*p) == 58)
						goto st54;
					goto st0;
				st54:
					if (++p == pe)
						goto _test_eof54;
				case 54:
					if ((*p) == 58)
						goto st95;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st55;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st55;
					}
					else
						goto st55;
					goto st0;
				st55:
					if (++p == pe)
						goto _test_eof55;
				case 55:
					if ((*p) == 58)
						goto st59;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st56;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st56;
					}
					else
						goto st56;
					goto st0;
				st56:
					if (++p == pe)
						goto _test_eof56;
				case 56:
					if ((*p) == 58)
						goto st59;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st57;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st57;
					}
					else
						goto st57;
					goto st0;
				st57:
					if (++p == pe)
						goto _test_eof57;
				case 57:
					if ((*p) == 58)
						goto st59;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st58;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st58;
					}
					else
						goto st58;
					goto st0;
				st58:
					if (++p == pe)
						goto _test_eof58;
				case 58:
					if ((*p) == 58)
						goto st59;
					goto st0;
				st59:
					if (++p == pe)
						goto _test_eof59;
				case 59:
					switch ((*p))
					{
					case 48:
						goto st60;
					case 49:
						goto st86;
					case 50:
						goto st89;
					case 58:
						goto st93;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st92;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st94;
					}
					else
						goto st94;
					goto st0;
				st60:
					if (++p == pe)
						goto _test_eof60;
				case 60:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st79;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st79;
					}
					else
						goto st79;
					goto st0;
				st61:
					if (++p == pe)
						goto _test_eof61;
				case 61:
					switch ((*p))
					{
					case 48:
						goto st62;
					case 49:
						goto st75;
					case 50:
						goto st77;
					}
					if (51 <= (*p) && (*p) <= 57)
						goto st76;
					goto st0;
				st62:
					if (++p == pe)
						goto _test_eof62;
				case 62:
					if ((*p) == 46)
						goto st63;
					goto st0;
				st63:
					if (++p == pe)
						goto _test_eof63;
				case 63:
					switch ((*p))
					{
					case 48:
						goto st64;
					case 49:
						goto st71;
					case 50:
						goto st73;
					}
					if (51 <= (*p) && (*p) <= 57)
						goto st72;
					goto st0;
				st64:
					if (++p == pe)
						goto _test_eof64;
				case 64:
					if ((*p) == 46)
						goto st65;
					goto st0;
				st65:
					if (++p == pe)
						goto _test_eof65;
				case 65:
					switch ((*p))
					{
					case 48:
						goto st66;
					case 49:
						goto st67;
					case 50:
						goto st69;
					}
					if (51 <= (*p) && (*p) <= 57)
						goto st68;
					goto st0;
				st66:
					if (++p == pe)
						goto _test_eof66;
				case 66:
					if ((*p) == 93)
						goto tr108;
					goto st0;
				tr108:
#line 66 "url_parser.rl"
					{
						host_type = 6;
					}
					goto st223;
				st223:
					if (++p == pe)
						goto _test_eof223;
				case 223:
#line 2296 "urlparser.cpp"
					switch ((*p))
					{
					case 35:
						goto tr207;
					case 47:
						goto tr208;
					case 58:
						goto st196;
					case 63:
						goto tr209;
					}
					goto st0;
				st67:
					if (++p == pe)
						goto _test_eof67;
				case 67:
					if ((*p) == 93)
						goto tr108;
					if (48 <= (*p) && (*p) <= 57)
						goto st68;
					goto st0;
				st68:
					if (++p == pe)
						goto _test_eof68;
				case 68:
					if ((*p) == 93)
						goto tr108;
					if (48 <= (*p) && (*p) <= 57)
						goto st66;
					goto st0;
				st69:
					if (++p == pe)
						goto _test_eof69;
				case 69:
					switch ((*p))
					{
					case 53:
						goto st70;
					case 93:
						goto tr108;
					}
					if ((*p) > 52)
					{
						if (54 <= (*p) && (*p) <= 57)
							goto st66;
					}
					else if ((*p) >= 48)
						goto st68;
					goto st0;
				st70:
					if (++p == pe)
						goto _test_eof70;
				case 70:
					if ((*p) == 93)
						goto tr108;
					if (48 <= (*p) && (*p) <= 53)
						goto st66;
					goto st0;
				st71:
					if (++p == pe)
						goto _test_eof71;
				case 71:
					if ((*p) == 46)
						goto st65;
					if (48 <= (*p) && (*p) <= 57)
						goto st72;
					goto st0;
				st72:
					if (++p == pe)
						goto _test_eof72;
				case 72:
					if ((*p) == 46)
						goto st65;
					if (48 <= (*p) && (*p) <= 57)
						goto st64;
					goto st0;
				st73:
					if (++p == pe)
						goto _test_eof73;
				case 73:
					switch ((*p))
					{
					case 46:
						goto st65;
					case 53:
						goto st74;
					}
					if ((*p) > 52)
					{
						if (54 <= (*p) && (*p) <= 57)
							goto st64;
					}
					else if ((*p) >= 48)
						goto st72;
					goto st0;
				st74:
					if (++p == pe)
						goto _test_eof74;
				case 74:
					if ((*p) == 46)
						goto st65;
					if (48 <= (*p) && (*p) <= 53)
						goto st64;
					goto st0;
				st75:
					if (++p == pe)
						goto _test_eof75;
				case 75:
					if ((*p) == 46)
						goto st63;
					if (48 <= (*p) && (*p) <= 57)
						goto st76;
					goto st0;
				st76:
					if (++p == pe)
						goto _test_eof76;
				case 76:
					if ((*p) == 46)
						goto st63;
					if (48 <= (*p) && (*p) <= 57)
						goto st62;
					goto st0;
				st77:
					if (++p == pe)
						goto _test_eof77;
				case 77:
					switch ((*p))
					{
					case 46:
						goto st63;
					case 53:
						goto st78;
					}
					if ((*p) > 52)
					{
						if (54 <= (*p) && (*p) <= 57)
							goto st62;
					}
					else if ((*p) >= 48)
						goto st76;
					goto st0;
				st78:
					if (++p == pe)
						goto _test_eof78;
				case 78:
					if ((*p) == 46)
						goto st63;
					if (48 <= (*p) && (*p) <= 53)
						goto st62;
					goto st0;
				st79:
					if (++p == pe)
						goto _test_eof79;
				case 79:
					if ((*p) == 58)
						goto st82;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st80;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st80;
					}
					else
						goto st80;
					goto st0;
				st80:
					if (++p == pe)
						goto _test_eof80;
				case 80:
					if ((*p) == 58)
						goto st82;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st81;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st81;
					}
					else
						goto st81;
					goto st0;
				st81:
					if (++p == pe)
						goto _test_eof81;
				case 81:
					if ((*p) == 58)
						goto st82;
					goto st0;
				st82:
					if (++p == pe)
						goto _test_eof82;
				case 82:
					if ((*p) == 58)
						goto st66;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st83;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st83;
					}
					else
						goto st83;
					goto st0;
				st83:
					if (++p == pe)
						goto _test_eof83;
				case 83:
					if ((*p) == 93)
						goto tr108;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st84;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st84;
					}
					else
						goto st84;
					goto st0;
				st84:
					if (++p == pe)
						goto _test_eof84;
				case 84:
					if ((*p) == 93)
						goto tr108;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st85;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st85;
					}
					else
						goto st85;
					goto st0;
				st85:
					if (++p == pe)
						goto _test_eof85;
				case 85:
					if ((*p) == 93)
						goto tr108;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st66;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st66;
					}
					else
						goto st66;
					goto st0;
				st86:
					if (++p == pe)
						goto _test_eof86;
				case 86:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st87;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st79;
					}
					else
						goto st79;
					goto st0;
				st87:
					if (++p == pe)
						goto _test_eof87;
				case 87:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st88;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st80;
					}
					else
						goto st80;
					goto st0;
				st88:
					if (++p == pe)
						goto _test_eof88;
				case 88:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st81;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st81;
					}
					else
						goto st81;
					goto st0;
				st89:
					if (++p == pe)
						goto _test_eof89;
				case 89:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st90;
					case 58:
						goto st82;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st87;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st79;
						}
						else if ((*p) >= 65)
							goto st79;
					}
					else
						goto st91;
					goto st0;
				st90:
					if (++p == pe)
						goto _test_eof90;
				case 90:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st88;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st80;
						}
						else if ((*p) >= 65)
							goto st80;
					}
					else
						goto st80;
					goto st0;
				st91:
					if (++p == pe)
						goto _test_eof91;
				case 91:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st80;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st80;
					}
					else
						goto st80;
					goto st0;
				st92:
					if (++p == pe)
						goto _test_eof92;
				case 92:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st82;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st91;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st79;
					}
					else
						goto st79;
					goto st0;
				st93:
					if (++p == pe)
						goto _test_eof93;
				case 93:
					if ((*p) == 93)
						goto tr108;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st83;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st83;
					}
					else
						goto st83;
					goto st0;
				st94:
					if (++p == pe)
						goto _test_eof94;
				case 94:
					if ((*p) == 58)
						goto st82;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st79;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st79;
					}
					else
						goto st79;
					goto st0;
				st95:
					if (++p == pe)
						goto _test_eof95;
				case 95:
					switch ((*p))
					{
					case 48:
						goto st96;
					case 49:
						goto st101;
					case 50:
						goto st104;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st107;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st108;
					}
					else
						goto st108;
					goto st0;
				st96:
					if (++p == pe)
						goto _test_eof96;
				case 96:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st97;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st97;
					}
					else
						goto st97;
					goto st0;
				st97:
					if (++p == pe)
						goto _test_eof97;
				case 97:
					switch ((*p))
					{
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st98;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st98;
					}
					else
						goto st98;
					goto st0;
				st98:
					if (++p == pe)
						goto _test_eof98;
				case 98:
					switch ((*p))
					{
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st99;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st99;
					}
					else
						goto st99;
					goto st0;
				st99:
					if (++p == pe)
						goto _test_eof99;
				case 99:
					switch ((*p))
					{
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					goto st0;
				st100:
					if (++p == pe)
						goto _test_eof100;
				case 100:
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st83;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st83;
					}
					else
						goto st83;
					goto st0;
				st101:
					if (++p == pe)
						goto _test_eof101;
				case 101:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st102;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st97;
					}
					else
						goto st97;
					goto st0;
				st102:
					if (++p == pe)
						goto _test_eof102;
				case 102:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st103;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st98;
					}
					else
						goto st98;
					goto st0;
				st103:
					if (++p == pe)
						goto _test_eof103;
				case 103:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st99;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st99;
					}
					else
						goto st99;
					goto st0;
				st104:
					if (++p == pe)
						goto _test_eof104;
				case 104:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st105;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st102;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st97;
						}
						else if ((*p) >= 65)
							goto st97;
					}
					else
						goto st106;
					goto st0;
				st105:
					if (++p == pe)
						goto _test_eof105;
				case 105:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st103;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st98;
						}
						else if ((*p) >= 65)
							goto st98;
					}
					else
						goto st98;
					goto st0;
				st106:
					if (++p == pe)
						goto _test_eof106;
				case 106:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st98;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st98;
					}
					else
						goto st98;
					goto st0;
				st107:
					if (++p == pe)
						goto _test_eof107;
				case 107:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st106;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st97;
					}
					else
						goto st97;
					goto st0;
				st108:
					if (++p == pe)
						goto _test_eof108;
				case 108:
					switch ((*p))
					{
					case 58:
						goto st100;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st97;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st97;
					}
					else
						goto st97;
					goto st0;
				st109:
					if (++p == pe)
						goto _test_eof109;
				case 109:
					switch ((*p))
					{
					case 48:
						goto st110;
					case 49:
						goto st115;
					case 50:
						goto st118;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st121;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st122;
					}
					else
						goto st122;
					goto st0;
				st110:
					if (++p == pe)
						goto _test_eof110;
				case 110:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st111;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st111;
					}
					else
						goto st111;
					goto st0;
				st111:
					if (++p == pe)
						goto _test_eof111;
				case 111:
					switch ((*p))
					{
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st112;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st112;
					}
					else
						goto st112;
					goto st0;
				st112:
					if (++p == pe)
						goto _test_eof112;
				case 112:
					switch ((*p))
					{
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st113;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st113;
					}
					else
						goto st113;
					goto st0;
				st113:
					if (++p == pe)
						goto _test_eof113;
				case 113:
					switch ((*p))
					{
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					goto st0;
				st114:
					if (++p == pe)
						goto _test_eof114;
				case 114:
					switch ((*p))
					{
					case 48:
						goto st96;
					case 49:
						goto st101;
					case 50:
						goto st104;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st107;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st108;
					}
					else
						goto st108;
					goto st0;
				st115:
					if (++p == pe)
						goto _test_eof115;
				case 115:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st116;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st111;
					}
					else
						goto st111;
					goto st0;
				st116:
					if (++p == pe)
						goto _test_eof116;
				case 116:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st117;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st112;
					}
					else
						goto st112;
					goto st0;
				st117:
					if (++p == pe)
						goto _test_eof117;
				case 117:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st113;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st113;
					}
					else
						goto st113;
					goto st0;
				st118:
					if (++p == pe)
						goto _test_eof118;
				case 118:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st119;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st116;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st111;
						}
						else if ((*p) >= 65)
							goto st111;
					}
					else
						goto st120;
					goto st0;
				st119:
					if (++p == pe)
						goto _test_eof119;
				case 119:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st117;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st112;
						}
						else if ((*p) >= 65)
							goto st112;
					}
					else
						goto st112;
					goto st0;
				st120:
					if (++p == pe)
						goto _test_eof120;
				case 120:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st112;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st112;
					}
					else
						goto st112;
					goto st0;
				st121:
					if (++p == pe)
						goto _test_eof121;
				case 121:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st120;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st111;
					}
					else
						goto st111;
					goto st0;
				st122:
					if (++p == pe)
						goto _test_eof122;
				case 122:
					switch ((*p))
					{
					case 58:
						goto st114;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st111;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st111;
					}
					else
						goto st111;
					goto st0;
				st123:
					if (++p == pe)
						goto _test_eof123;
				case 123:
					switch ((*p))
					{
					case 48:
						goto st124;
					case 49:
						goto st129;
					case 50:
						goto st132;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st135;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st136;
					}
					else
						goto st136;
					goto st0;
				st124:
					if (++p == pe)
						goto _test_eof124;
				case 124:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st125;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st125;
					}
					else
						goto st125;
					goto st0;
				st125:
					if (++p == pe)
						goto _test_eof125;
				case 125:
					switch ((*p))
					{
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st126;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st126;
					}
					else
						goto st126;
					goto st0;
				st126:
					if (++p == pe)
						goto _test_eof126;
				case 126:
					switch ((*p))
					{
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st127;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st127;
					}
					else
						goto st127;
					goto st0;
				st127:
					if (++p == pe)
						goto _test_eof127;
				case 127:
					switch ((*p))
					{
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					goto st0;
				st128:
					if (++p == pe)
						goto _test_eof128;
				case 128:
					switch ((*p))
					{
					case 48:
						goto st110;
					case 49:
						goto st115;
					case 50:
						goto st118;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st121;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st122;
					}
					else
						goto st122;
					goto st0;
				st129:
					if (++p == pe)
						goto _test_eof129;
				case 129:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st130;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st125;
					}
					else
						goto st125;
					goto st0;
				st130:
					if (++p == pe)
						goto _test_eof130;
				case 130:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st131;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st126;
					}
					else
						goto st126;
					goto st0;
				st131:
					if (++p == pe)
						goto _test_eof131;
				case 131:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st127;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st127;
					}
					else
						goto st127;
					goto st0;
				st132:
					if (++p == pe)
						goto _test_eof132;
				case 132:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st133;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st130;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st125;
						}
						else if ((*p) >= 65)
							goto st125;
					}
					else
						goto st134;
					goto st0;
				st133:
					if (++p == pe)
						goto _test_eof133;
				case 133:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st131;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st126;
						}
						else if ((*p) >= 65)
							goto st126;
					}
					else
						goto st126;
					goto st0;
				st134:
					if (++p == pe)
						goto _test_eof134;
				case 134:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st126;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st126;
					}
					else
						goto st126;
					goto st0;
				st135:
					if (++p == pe)
						goto _test_eof135;
				case 135:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st134;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st125;
					}
					else
						goto st125;
					goto st0;
				st136:
					if (++p == pe)
						goto _test_eof136;
				case 136:
					switch ((*p))
					{
					case 58:
						goto st128;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st125;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st125;
					}
					else
						goto st125;
					goto st0;
				st137:
					if (++p == pe)
						goto _test_eof137;
				case 137:
					switch ((*p))
					{
					case 48:
						goto st138;
					case 49:
						goto st143;
					case 50:
						goto st146;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st149;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st150;
					}
					else
						goto st150;
					goto st0;
				st138:
					if (++p == pe)
						goto _test_eof138;
				case 138:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st139;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st139;
					}
					else
						goto st139;
					goto st0;
				st139:
					if (++p == pe)
						goto _test_eof139;
				case 139:
					switch ((*p))
					{
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st140;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st140;
					}
					else
						goto st140;
					goto st0;
				st140:
					if (++p == pe)
						goto _test_eof140;
				case 140:
					switch ((*p))
					{
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st141;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st141;
					}
					else
						goto st141;
					goto st0;
				st141:
					if (++p == pe)
						goto _test_eof141;
				case 141:
					switch ((*p))
					{
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					goto st0;
				st142:
					if (++p == pe)
						goto _test_eof142;
				case 142:
					switch ((*p))
					{
					case 48:
						goto st124;
					case 49:
						goto st129;
					case 50:
						goto st132;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st135;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st136;
					}
					else
						goto st136;
					goto st0;
				st143:
					if (++p == pe)
						goto _test_eof143;
				case 143:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st144;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st139;
					}
					else
						goto st139;
					goto st0;
				st144:
					if (++p == pe)
						goto _test_eof144;
				case 144:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st145;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st140;
					}
					else
						goto st140;
					goto st0;
				st145:
					if (++p == pe)
						goto _test_eof145;
				case 145:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st141;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st141;
					}
					else
						goto st141;
					goto st0;
				st146:
					if (++p == pe)
						goto _test_eof146;
				case 146:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st147;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st144;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st139;
						}
						else if ((*p) >= 65)
							goto st139;
					}
					else
						goto st148;
					goto st0;
				st147:
					if (++p == pe)
						goto _test_eof147;
				case 147:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st145;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st140;
						}
						else if ((*p) >= 65)
							goto st140;
					}
					else
						goto st140;
					goto st0;
				st148:
					if (++p == pe)
						goto _test_eof148;
				case 148:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st140;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st140;
					}
					else
						goto st140;
					goto st0;
				st149:
					if (++p == pe)
						goto _test_eof149;
				case 149:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st148;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st139;
					}
					else
						goto st139;
					goto st0;
				st150:
					if (++p == pe)
						goto _test_eof150;
				case 150:
					switch ((*p))
					{
					case 58:
						goto st142;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st139;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st139;
					}
					else
						goto st139;
					goto st0;
				st151:
					if (++p == pe)
						goto _test_eof151;
				case 151:
					switch ((*p))
					{
					case 48:
						goto st152;
					case 49:
						goto st157;
					case 50:
						goto st160;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st163;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st164;
					}
					else
						goto st164;
					goto st0;
				st152:
					if (++p == pe)
						goto _test_eof152;
				case 152:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st153;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st153;
					}
					else
						goto st153;
					goto st0;
				st153:
					if (++p == pe)
						goto _test_eof153;
				case 153:
					switch ((*p))
					{
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st154;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st154;
					}
					else
						goto st154;
					goto st0;
				st154:
					if (++p == pe)
						goto _test_eof154;
				case 154:
					switch ((*p))
					{
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st155;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st155;
					}
					else
						goto st155;
					goto st0;
				st155:
					if (++p == pe)
						goto _test_eof155;
				case 155:
					switch ((*p))
					{
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					goto st0;
				st156:
					if (++p == pe)
						goto _test_eof156;
				case 156:
					switch ((*p))
					{
					case 48:
						goto st138;
					case 49:
						goto st143;
					case 50:
						goto st146;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st149;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st150;
					}
					else
						goto st150;
					goto st0;
				st157:
					if (++p == pe)
						goto _test_eof157;
				case 157:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st158;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st153;
					}
					else
						goto st153;
					goto st0;
				st158:
					if (++p == pe)
						goto _test_eof158;
				case 158:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st159;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st154;
					}
					else
						goto st154;
					goto st0;
				st159:
					if (++p == pe)
						goto _test_eof159;
				case 159:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st155;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st155;
					}
					else
						goto st155;
					goto st0;
				st160:
					if (++p == pe)
						goto _test_eof160;
				case 160:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st161;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st158;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st153;
						}
						else if ((*p) >= 65)
							goto st153;
					}
					else
						goto st162;
					goto st0;
				st161:
					if (++p == pe)
						goto _test_eof161;
				case 161:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st159;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st154;
						}
						else if ((*p) >= 65)
							goto st154;
					}
					else
						goto st154;
					goto st0;
				st162:
					if (++p == pe)
						goto _test_eof162;
				case 162:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st154;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st154;
					}
					else
						goto st154;
					goto st0;
				st163:
					if (++p == pe)
						goto _test_eof163;
				case 163:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st162;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st153;
					}
					else
						goto st153;
					goto st0;
				st164:
					if (++p == pe)
						goto _test_eof164;
				case 164:
					switch ((*p))
					{
					case 58:
						goto st156;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st153;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st153;
					}
					else
						goto st153;
					goto st0;
				st165:
					if (++p == pe)
						goto _test_eof165;
				case 165:
					if ((*p) == 58)
						goto st166;
					goto st0;
				st166:
					if (++p == pe)
						goto _test_eof166;
				case 166:
					switch ((*p))
					{
					case 48:
						goto st167;
					case 49:
						goto st172;
					case 50:
						goto st175;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st178;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st179;
					}
					else
						goto st179;
					goto st0;
				st167:
					if (++p == pe)
						goto _test_eof167;
				case 167:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st168;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st168;
					}
					else
						goto st168;
					goto st0;
				st168:
					if (++p == pe)
						goto _test_eof168;
				case 168:
					switch ((*p))
					{
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st169;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st169;
					}
					else
						goto st169;
					goto st0;
				st169:
					if (++p == pe)
						goto _test_eof169;
				case 169:
					switch ((*p))
					{
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st170;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st170;
					}
					else
						goto st170;
					goto st0;
				st170:
					if (++p == pe)
						goto _test_eof170;
				case 170:
					switch ((*p))
					{
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					goto st0;
				st171:
					if (++p == pe)
						goto _test_eof171;
				case 171:
					switch ((*p))
					{
					case 48:
						goto st152;
					case 49:
						goto st157;
					case 50:
						goto st160;
					}
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st163;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st164;
					}
					else
						goto st164;
					goto st0;
				st172:
					if (++p == pe)
						goto _test_eof172;
				case 172:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st173;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st168;
					}
					else
						goto st168;
					goto st0;
				st173:
					if (++p == pe)
						goto _test_eof173;
				case 173:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st174;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st169;
					}
					else
						goto st169;
					goto st0;
				st174:
					if (++p == pe)
						goto _test_eof174;
				case 174:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st170;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st170;
					}
					else
						goto st170;
					goto st0;
				st175:
					if (++p == pe)
						goto _test_eof175;
				case 175:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 53:
						goto st176;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st173;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st168;
						}
						else if ((*p) >= 65)
							goto st168;
					}
					else
						goto st177;
					goto st0;
				st176:
					if (++p == pe)
						goto _test_eof176;
				case 176:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st174;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 70)
						{
							if (97 <= (*p) && (*p) <= 102)
								goto st169;
						}
						else if ((*p) >= 65)
							goto st169;
					}
					else
						goto st169;
					goto st0;
				st177:
					if (++p == pe)
						goto _test_eof177;
				case 177:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st169;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st169;
					}
					else
						goto st169;
					goto st0;
				st178:
					if (++p == pe)
						goto _test_eof178;
				case 178:
					switch ((*p))
					{
					case 46:
						goto st61;
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st177;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st168;
					}
					else
						goto st168;
					goto st0;
				st179:
					if (++p == pe)
						goto _test_eof179;
				case 179:
					switch ((*p))
					{
					case 58:
						goto st171;
					case 93:
						goto tr108;
					}
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st168;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st168;
					}
					else
						goto st168;
					goto st0;
				st180:
					if (++p == pe)
						goto _test_eof180;
				case 180:
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st181;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st181;
					}
					else
						goto st181;
					goto st0;
				st181:
					if (++p == pe)
						goto _test_eof181;
				case 181:
					if ((*p) == 46)
						goto st182;
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st181;
					}
					else if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st181;
					}
					else
						goto st181;
					goto st0;
				st182:
					if (++p == pe)
						goto _test_eof182;
				case 182:
					switch ((*p))
					{
					case 33:
						goto st184;
					case 36:
						goto st184;
					case 61:
						goto st184;
					case 95:
						goto st184;
					case 126:
						goto st184;
					}
					if ((*p) < 38)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st183;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st186;
						}
						else
							goto st185;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 59)
								goto st184;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st184;
						}
						else
							goto st184;
					}
					else
						goto st184;
					goto st0;
				st183:
					if (++p == pe)
						goto _test_eof183;
				case 183:
					if ((*p) <= -65)
						goto st184;
					goto st0;
				st184:
					if (++p == pe)
						goto _test_eof184;
				case 184:
					switch ((*p))
					{
					case 33:
						goto st184;
					case 36:
						goto st184;
					case 61:
						goto st184;
					case 93:
						goto st223;
					case 95:
						goto st184;
					case 126:
						goto st184;
					}
					if ((*p) < 38)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st183;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st186;
						}
						else
							goto st185;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 59)
								goto st184;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st184;
						}
						else
							goto st184;
					}
					else
						goto st184;
					goto st0;
				st185:
					if (++p == pe)
						goto _test_eof185;
				case 185:
					if ((*p) <= -65)
						goto st183;
					goto st0;
				st186:
					if (++p == pe)
						goto _test_eof186;
				case 186:
					if ((*p) <= -65)
						goto st185;
					goto st0;
				tr237:
#line 61 "url_parser.rl"
					{
						port_start = p;
					}
					goto st224;
				st224:
					if (++p == pe)
						goto _test_eof224;
				case 224:
#line 4292 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st23;
					case 35:
						goto tr207;
					case 37:
						goto st26;
					case 47:
						goto tr208;
					case 61:
						goto st23;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st23;
					case 126:
						goto st23;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st22;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st25;
						}
						else
							goto st24;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 58)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st224;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st23;
							}
							else if ((*p) >= 65)
								goto st23;
						}
						else
							goto st23;
					}
					else
						goto st23;
					goto st0;
				tr20:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st225;
				st225:
					if (++p == pe)
						goto _test_eof225;
				case 225:
#line 4338 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st226;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st17;
						}
						else if ((*p) >= -62)
							goto st16;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st193;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st18;
					goto st0;
				st226:
					if (++p == pe)
						goto _test_eof226;
				case 226:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 47:
						goto tr208;
					case 48:
						goto st227;
					case 49:
						goto st240;
					case 50:
						goto st242;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto st241;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st227:
					if (++p == pe)
						goto _test_eof227;
				case 227:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st228;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st17;
						}
						else if ((*p) >= -62)
							goto st16;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st193;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st18;
					goto st0;
				st228:
					if (++p == pe)
						goto _test_eof228;
				case 228:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 47:
						goto tr208;
					case 48:
						goto st229;
					case 49:
						goto st236;
					case 50:
						goto st238;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto st237;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st229:
					if (++p == pe)
						goto _test_eof229;
				case 229:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st230;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st17;
						}
						else if ((*p) >= -62)
							goto st16;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st193;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st18;
					goto st0;
				st230:
					if (++p == pe)
						goto _test_eof230;
				case 230:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 47:
						goto tr208;
					case 48:
						goto st231;
					case 49:
						goto st232;
					case 50:
						goto st234;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (51 <= (*p) && (*p) <= 57)
								goto st233;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st231:
					if (++p == pe)
						goto _test_eof231;
				case 231:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr253;
					case 37:
						goto st194;
					case 47:
						goto tr254;
					case 58:
						goto tr279;
					case 61:
						goto st193;
					case 63:
						goto tr256;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < -16)
					{
						if ((*p) > -33)
						{
							if (-32 <= (*p) && (*p) <= -17)
								goto st17;
						}
						else if ((*p) >= -62)
							goto st16;
					}
					else if ((*p) > -12)
					{
						if ((*p) < 65)
						{
							if (36 <= (*p) && (*p) <= 59)
								goto st193;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st18;
					goto st0;
				st232:
					if (++p == pe)
						goto _test_eof232;
				case 232:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr253;
					case 37:
						goto st194;
					case 47:
						goto tr254;
					case 58:
						goto tr279;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr256;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st233;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st233:
					if (++p == pe)
						goto _test_eof233;
				case 233:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr253;
					case 37:
						goto st194;
					case 47:
						goto tr254;
					case 58:
						goto tr279;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr256;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st231;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st234:
					if (++p == pe)
						goto _test_eof234;
				case 234:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr253;
					case 37:
						goto st194;
					case 47:
						goto tr254;
					case 53:
						goto st235;
					case 58:
						goto tr279;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr256;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st233;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st231;
					}
					else
						goto st193;
					goto st0;
				st235:
					if (++p == pe)
						goto _test_eof235;
				case 235:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr253;
					case 37:
						goto st194;
					case 47:
						goto tr254;
					case 58:
						goto tr279;
					case 61:
						goto st193;
					case 63:
						goto tr256;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 46)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st231;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st236:
					if (++p == pe)
						goto _test_eof236;
				case 236:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st230;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st237;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st237:
					if (++p == pe)
						goto _test_eof237;
				case 237:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st230;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st229;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st238:
					if (++p == pe)
						goto _test_eof238;
				case 238:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st230;
					case 47:
						goto tr208;
					case 53:
						goto st239;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st237;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st229;
					}
					else
						goto st193;
					goto st0;
				st239:
					if (++p == pe)
						goto _test_eof239;
				case 239:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st230;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st229;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st240:
					if (++p == pe)
						goto _test_eof240;
				case 240:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st228;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st241;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st241:
					if (++p == pe)
						goto _test_eof241;
				case 241:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st228;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st227;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				st242:
					if (++p == pe)
						goto _test_eof242;
				case 242:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st228;
					case 47:
						goto tr208;
					case 53:
						goto st243;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st241;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st227;
					}
					else
						goto st193;
					goto st0;
				st243:
					if (++p == pe)
						goto _test_eof243;
				case 243:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st228;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st227;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				tr21:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st244;
				st244:
					if (++p == pe)
						goto _test_eof244;
				case 244:
#line 5089 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st226;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st245;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				tr23:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st245;
				st245:
					if (++p == pe)
						goto _test_eof245;
				case 245:
#line 5135 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st226;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 65)
						{
							if (48 <= (*p) && (*p) <= 57)
								goto st225;
						}
						else if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				tr22:
#line 59 "url_parser.rl"
					{
						userinfo_start = p;
					}
#line 60 "url_parser.rl"
					{
						host_start = p;
					}
					goto st246;
				st246:
					if (++p == pe)
						goto _test_eof246;
				case 246:
#line 5181 "urlparser.cpp"
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st226;
					case 47:
						goto tr208;
					case 53:
						goto st247;
					case 58:
						goto st199;
					case 59:
						goto st193;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 52)
								goto st245;
						}
						else if ((*p) > 57)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st225;
					}
					else
						goto st193;
					goto st0;
				st247:
					if (++p == pe)
						goto _test_eof247;
				case 247:
					switch ((*p))
					{
					case 33:
						goto st193;
					case 35:
						goto tr207;
					case 37:
						goto st194;
					case 46:
						goto st226;
					case 47:
						goto tr208;
					case 58:
						goto st199;
					case 61:
						goto st193;
					case 63:
						goto tr209;
					case 64:
						goto st28;
					case 95:
						goto st193;
					case 126:
						goto st193;
					}
					if ((*p) < 36)
					{
						if ((*p) < -32)
						{
							if (-62 <= (*p) && (*p) <= -33)
								goto st16;
						}
						else if ((*p) > -17)
						{
							if (-16 <= (*p) && (*p) <= -12)
								goto st18;
						}
						else
							goto st17;
					}
					else if ((*p) > 45)
					{
						if ((*p) < 54)
						{
							if (48 <= (*p) && (*p) <= 53)
								goto st225;
						}
						else if ((*p) > 59)
						{
							if ((*p) > 90)
							{
								if (97 <= (*p) && (*p) <= 122)
									goto st193;
							}
							else if ((*p) >= 65)
								goto st193;
						}
						else
							goto st193;
					}
					else
						goto st193;
					goto st0;
				}
			_test_eof188:
				cs = 188;
				goto _test_eof;
			_test_eof1:
				cs = 1;
				goto _test_eof;
			_test_eof189:
				cs = 189;
				goto _test_eof;
			_test_eof2:
				cs = 2;
				goto _test_eof;
			_test_eof3:
				cs = 3;
				goto _test_eof;
			_test_eof190:
				cs = 190;
				goto _test_eof;
			_test_eof4:
				cs = 4;
				goto _test_eof;
			_test_eof5:
				cs = 5;
				goto _test_eof;
			_test_eof6:
				cs = 6;
				goto _test_eof;
			_test_eof7:
				cs = 7;
				goto _test_eof;
			_test_eof8:
				cs = 8;
				goto _test_eof;
			_test_eof191:
				cs = 191;
				goto _test_eof;
			_test_eof9:
				cs = 9;
				goto _test_eof;
			_test_eof192:
				cs = 192;
				goto _test_eof;
			_test_eof10:
				cs = 10;
				goto _test_eof;
			_test_eof11:
				cs = 11;
				goto _test_eof;
			_test_eof12:
				cs = 12;
				goto _test_eof;
			_test_eof13:
				cs = 13;
				goto _test_eof;
			_test_eof14:
				cs = 14;
				goto _test_eof;
			_test_eof15:
				cs = 15;
				goto _test_eof;
			_test_eof16:
				cs = 16;
				goto _test_eof;
			_test_eof193:
				cs = 193;
				goto _test_eof;
			_test_eof17:
				cs = 17;
				goto _test_eof;
			_test_eof18:
				cs = 18;
				goto _test_eof;
			_test_eof194:
				cs = 194;
				goto _test_eof;
			_test_eof19:
				cs = 19;
				goto _test_eof;
			_test_eof195:
				cs = 195;
				goto _test_eof;
			_test_eof20:
				cs = 20;
				goto _test_eof;
			_test_eof21:
				cs = 21;
				goto _test_eof;
			_test_eof196:
				cs = 196;
				goto _test_eof;
			_test_eof197:
				cs = 197;
				goto _test_eof;
			_test_eof198:
				cs = 198;
				goto _test_eof;
			_test_eof199:
				cs = 199;
				goto _test_eof;
			_test_eof22:
				cs = 22;
				goto _test_eof;
			_test_eof23:
				cs = 23;
				goto _test_eof;
			_test_eof24:
				cs = 24;
				goto _test_eof;
			_test_eof25:
				cs = 25;
				goto _test_eof;
			_test_eof26:
				cs = 26;
				goto _test_eof;
			_test_eof27:
				cs = 27;
				goto _test_eof;
			_test_eof28:
				cs = 28;
				goto _test_eof;
			_test_eof200:
				cs = 200;
				goto _test_eof;
			_test_eof201:
				cs = 201;
				goto _test_eof;
			_test_eof202:
				cs = 202;
				goto _test_eof;
			_test_eof203:
				cs = 203;
				goto _test_eof;
			_test_eof204:
				cs = 204;
				goto _test_eof;
			_test_eof205:
				cs = 205;
				goto _test_eof;
			_test_eof206:
				cs = 206;
				goto _test_eof;
			_test_eof207:
				cs = 207;
				goto _test_eof;
			_test_eof208:
				cs = 208;
				goto _test_eof;
			_test_eof209:
				cs = 209;
				goto _test_eof;
			_test_eof210:
				cs = 210;
				goto _test_eof;
			_test_eof211:
				cs = 211;
				goto _test_eof;
			_test_eof212:
				cs = 212;
				goto _test_eof;
			_test_eof213:
				cs = 213;
				goto _test_eof;
			_test_eof214:
				cs = 214;
				goto _test_eof;
			_test_eof215:
				cs = 215;
				goto _test_eof;
			_test_eof216:
				cs = 216;
				goto _test_eof;
			_test_eof217:
				cs = 217;
				goto _test_eof;
			_test_eof218:
				cs = 218;
				goto _test_eof;
			_test_eof219:
				cs = 219;
				goto _test_eof;
			_test_eof220:
				cs = 220;
				goto _test_eof;
			_test_eof221:
				cs = 221;
				goto _test_eof;
			_test_eof222:
				cs = 222;
				goto _test_eof;
			_test_eof29:
				cs = 29;
				goto _test_eof;
			_test_eof30:
				cs = 30;
				goto _test_eof;
			_test_eof31:
				cs = 31;
				goto _test_eof;
			_test_eof32:
				cs = 32;
				goto _test_eof;
			_test_eof33:
				cs = 33;
				goto _test_eof;
			_test_eof34:
				cs = 34;
				goto _test_eof;
			_test_eof35:
				cs = 35;
				goto _test_eof;
			_test_eof36:
				cs = 36;
				goto _test_eof;
			_test_eof37:
				cs = 37;
				goto _test_eof;
			_test_eof38:
				cs = 38;
				goto _test_eof;
			_test_eof39:
				cs = 39;
				goto _test_eof;
			_test_eof40:
				cs = 40;
				goto _test_eof;
			_test_eof41:
				cs = 41;
				goto _test_eof;
			_test_eof42:
				cs = 42;
				goto _test_eof;
			_test_eof43:
				cs = 43;
				goto _test_eof;
			_test_eof44:
				cs = 44;
				goto _test_eof;
			_test_eof45:
				cs = 45;
				goto _test_eof;
			_test_eof46:
				cs = 46;
				goto _test_eof;
			_test_eof47:
				cs = 47;
				goto _test_eof;
			_test_eof48:
				cs = 48;
				goto _test_eof;
			_test_eof49:
				cs = 49;
				goto _test_eof;
			_test_eof50:
				cs = 50;
				goto _test_eof;
			_test_eof51:
				cs = 51;
				goto _test_eof;
			_test_eof52:
				cs = 52;
				goto _test_eof;
			_test_eof53:
				cs = 53;
				goto _test_eof;
			_test_eof54:
				cs = 54;
				goto _test_eof;
			_test_eof55:
				cs = 55;
				goto _test_eof;
			_test_eof56:
				cs = 56;
				goto _test_eof;
			_test_eof57:
				cs = 57;
				goto _test_eof;
			_test_eof58:
				cs = 58;
				goto _test_eof;
			_test_eof59:
				cs = 59;
				goto _test_eof;
			_test_eof60:
				cs = 60;
				goto _test_eof;
			_test_eof61:
				cs = 61;
				goto _test_eof;
			_test_eof62:
				cs = 62;
				goto _test_eof;
			_test_eof63:
				cs = 63;
				goto _test_eof;
			_test_eof64:
				cs = 64;
				goto _test_eof;
			_test_eof65:
				cs = 65;
				goto _test_eof;
			_test_eof66:
				cs = 66;
				goto _test_eof;
			_test_eof223:
				cs = 223;
				goto _test_eof;
			_test_eof67:
				cs = 67;
				goto _test_eof;
			_test_eof68:
				cs = 68;
				goto _test_eof;
			_test_eof69:
				cs = 69;
				goto _test_eof;
			_test_eof70:
				cs = 70;
				goto _test_eof;
			_test_eof71:
				cs = 71;
				goto _test_eof;
			_test_eof72:
				cs = 72;
				goto _test_eof;
			_test_eof73:
				cs = 73;
				goto _test_eof;
			_test_eof74:
				cs = 74;
				goto _test_eof;
			_test_eof75:
				cs = 75;
				goto _test_eof;
			_test_eof76:
				cs = 76;
				goto _test_eof;
			_test_eof77:
				cs = 77;
				goto _test_eof;
			_test_eof78:
				cs = 78;
				goto _test_eof;
			_test_eof79:
				cs = 79;
				goto _test_eof;
			_test_eof80:
				cs = 80;
				goto _test_eof;
			_test_eof81:
				cs = 81;
				goto _test_eof;
			_test_eof82:
				cs = 82;
				goto _test_eof;
			_test_eof83:
				cs = 83;
				goto _test_eof;
			_test_eof84:
				cs = 84;
				goto _test_eof;
			_test_eof85:
				cs = 85;
				goto _test_eof;
			_test_eof86:
				cs = 86;
				goto _test_eof;
			_test_eof87:
				cs = 87;
				goto _test_eof;
			_test_eof88:
				cs = 88;
				goto _test_eof;
			_test_eof89:
				cs = 89;
				goto _test_eof;
			_test_eof90:
				cs = 90;
				goto _test_eof;
			_test_eof91:
				cs = 91;
				goto _test_eof;
			_test_eof92:
				cs = 92;
				goto _test_eof;
			_test_eof93:
				cs = 93;
				goto _test_eof;
			_test_eof94:
				cs = 94;
				goto _test_eof;
			_test_eof95:
				cs = 95;
				goto _test_eof;
			_test_eof96:
				cs = 96;
				goto _test_eof;
			_test_eof97:
				cs = 97;
				goto _test_eof;
			_test_eof98:
				cs = 98;
				goto _test_eof;
			_test_eof99:
				cs = 99;
				goto _test_eof;
			_test_eof100:
				cs = 100;
				goto _test_eof;
			_test_eof101:
				cs = 101;
				goto _test_eof;
			_test_eof102:
				cs = 102;
				goto _test_eof;
			_test_eof103:
				cs = 103;
				goto _test_eof;
			_test_eof104:
				cs = 104;
				goto _test_eof;
			_test_eof105:
				cs = 105;
				goto _test_eof;
			_test_eof106:
				cs = 106;
				goto _test_eof;
			_test_eof107:
				cs = 107;
				goto _test_eof;
			_test_eof108:
				cs = 108;
				goto _test_eof;
			_test_eof109:
				cs = 109;
				goto _test_eof;
			_test_eof110:
				cs = 110;
				goto _test_eof;
			_test_eof111:
				cs = 111;
				goto _test_eof;
			_test_eof112:
				cs = 112;
				goto _test_eof;
			_test_eof113:
				cs = 113;
				goto _test_eof;
			_test_eof114:
				cs = 114;
				goto _test_eof;
			_test_eof115:
				cs = 115;
				goto _test_eof;
			_test_eof116:
				cs = 116;
				goto _test_eof;
			_test_eof117:
				cs = 117;
				goto _test_eof;
			_test_eof118:
				cs = 118;
				goto _test_eof;
			_test_eof119:
				cs = 119;
				goto _test_eof;
			_test_eof120:
				cs = 120;
				goto _test_eof;
			_test_eof121:
				cs = 121;
				goto _test_eof;
			_test_eof122:
				cs = 122;
				goto _test_eof;
			_test_eof123:
				cs = 123;
				goto _test_eof;
			_test_eof124:
				cs = 124;
				goto _test_eof;
			_test_eof125:
				cs = 125;
				goto _test_eof;
			_test_eof126:
				cs = 126;
				goto _test_eof;
			_test_eof127:
				cs = 127;
				goto _test_eof;
			_test_eof128:
				cs = 128;
				goto _test_eof;
			_test_eof129:
				cs = 129;
				goto _test_eof;
			_test_eof130:
				cs = 130;
				goto _test_eof;
			_test_eof131:
				cs = 131;
				goto _test_eof;
			_test_eof132:
				cs = 132;
				goto _test_eof;
			_test_eof133:
				cs = 133;
				goto _test_eof;
			_test_eof134:
				cs = 134;
				goto _test_eof;
			_test_eof135:
				cs = 135;
				goto _test_eof;
			_test_eof136:
				cs = 136;
				goto _test_eof;
			_test_eof137:
				cs = 137;
				goto _test_eof;
			_test_eof138:
				cs = 138;
				goto _test_eof;
			_test_eof139:
				cs = 139;
				goto _test_eof;
			_test_eof140:
				cs = 140;
				goto _test_eof;
			_test_eof141:
				cs = 141;
				goto _test_eof;
			_test_eof142:
				cs = 142;
				goto _test_eof;
			_test_eof143:
				cs = 143;
				goto _test_eof;
			_test_eof144:
				cs = 144;
				goto _test_eof;
			_test_eof145:
				cs = 145;
				goto _test_eof;
			_test_eof146:
				cs = 146;
				goto _test_eof;
			_test_eof147:
				cs = 147;
				goto _test_eof;
			_test_eof148:
				cs = 148;
				goto _test_eof;
			_test_eof149:
				cs = 149;
				goto _test_eof;
			_test_eof150:
				cs = 150;
				goto _test_eof;
			_test_eof151:
				cs = 151;
				goto _test_eof;
			_test_eof152:
				cs = 152;
				goto _test_eof;
			_test_eof153:
				cs = 153;
				goto _test_eof;
			_test_eof154:
				cs = 154;
				goto _test_eof;
			_test_eof155:
				cs = 155;
				goto _test_eof;
			_test_eof156:
				cs = 156;
				goto _test_eof;
			_test_eof157:
				cs = 157;
				goto _test_eof;
			_test_eof158:
				cs = 158;
				goto _test_eof;
			_test_eof159:
				cs = 159;
				goto _test_eof;
			_test_eof160:
				cs = 160;
				goto _test_eof;
			_test_eof161:
				cs = 161;
				goto _test_eof;
			_test_eof162:
				cs = 162;
				goto _test_eof;
			_test_eof163:
				cs = 163;
				goto _test_eof;
			_test_eof164:
				cs = 164;
				goto _test_eof;
			_test_eof165:
				cs = 165;
				goto _test_eof;
			_test_eof166:
				cs = 166;
				goto _test_eof;
			_test_eof167:
				cs = 167;
				goto _test_eof;
			_test_eof168:
				cs = 168;
				goto _test_eof;
			_test_eof169:
				cs = 169;
				goto _test_eof;
			_test_eof170:
				cs = 170;
				goto _test_eof;
			_test_eof171:
				cs = 171;
				goto _test_eof;
			_test_eof172:
				cs = 172;
				goto _test_eof;
			_test_eof173:
				cs = 173;
				goto _test_eof;
			_test_eof174:
				cs = 174;
				goto _test_eof;
			_test_eof175:
				cs = 175;
				goto _test_eof;
			_test_eof176:
				cs = 176;
				goto _test_eof;
			_test_eof177:
				cs = 177;
				goto _test_eof;
			_test_eof178:
				cs = 178;
				goto _test_eof;
			_test_eof179:
				cs = 179;
				goto _test_eof;
			_test_eof180:
				cs = 180;
				goto _test_eof;
			_test_eof181:
				cs = 181;
				goto _test_eof;
			_test_eof182:
				cs = 182;
				goto _test_eof;
			_test_eof183:
				cs = 183;
				goto _test_eof;
			_test_eof184:
				cs = 184;
				goto _test_eof;
			_test_eof185:
				cs = 185;
				goto _test_eof;
			_test_eof186:
				cs = 186;
				goto _test_eof;
			_test_eof224:
				cs = 224;
				goto _test_eof;
			_test_eof225:
				cs = 225;
				goto _test_eof;
			_test_eof226:
				cs = 226;
				goto _test_eof;
			_test_eof227:
				cs = 227;
				goto _test_eof;
			_test_eof228:
				cs = 228;
				goto _test_eof;
			_test_eof229:
				cs = 229;
				goto _test_eof;
			_test_eof230:
				cs = 230;
				goto _test_eof;
			_test_eof231:
				cs = 231;
				goto _test_eof;
			_test_eof232:
				cs = 232;
				goto _test_eof;
			_test_eof233:
				cs = 233;
				goto _test_eof;
			_test_eof234:
				cs = 234;
				goto _test_eof;
			_test_eof235:
				cs = 235;
				goto _test_eof;
			_test_eof236:
				cs = 236;
				goto _test_eof;
			_test_eof237:
				cs = 237;
				goto _test_eof;
			_test_eof238:
				cs = 238;
				goto _test_eof;
			_test_eof239:
				cs = 239;
				goto _test_eof;
			_test_eof240:
				cs = 240;
				goto _test_eof;
			_test_eof241:
				cs = 241;
				goto _test_eof;
			_test_eof242:
				cs = 242;
				goto _test_eof;
			_test_eof243:
				cs = 243;
				goto _test_eof;
			_test_eof244:
				cs = 244;
				goto _test_eof;
			_test_eof245:
				cs = 245;
				goto _test_eof;
			_test_eof246:
				cs = 246;
				goto _test_eof;
			_test_eof247:
				cs = 247;
				goto _test_eof;

			_test_eof:
				{
				}
				if (p == eof)
				{
					switch (cs)
					{
					case 187:
					case 193:
					case 194:
					case 195:
					case 197:
					case 198:
					case 200:
					case 201:
					case 202:
					case 203:
					case 204:
					case 205:
					case 211:
					case 212:
					case 213:
					case 214:
					case 215:
					case 216:
					case 217:
					case 218:
					case 219:
					case 220:
					case 221:
					case 222:
					case 223:
					case 224:
					case 225:
					case 226:
					case 227:
					case 228:
					case 229:
					case 230:
					case 236:
					case 237:
					case 238:
					case 239:
					case 240:
					case 241:
					case 242:
					case 243:
					case 244:
					case 245:
					case 246:
					case 247:
#line 62 "url_parser.rl"
					{
						path_start = p;
					}
					break;
					case 191:
#line 63 "url_parser.rl"
					{
						query_start = p;
					}
					break;
					case 188:
#line 64 "url_parser.rl"
					{
						fragment_start = p;
					}
					break;
					case 196:
					case 199:
#line 61 "url_parser.rl"
					{
						port_start = p;
					}
#line 62 "url_parser.rl"
						{
							path_start = p;
						}
						break;
					case 206:
					case 207:
					case 208:
					case 209:
					case 210:
					case 231:
					case 232:
					case 233:
					case 234:
					case 235:
#line 65 "url_parser.rl"
					{
						host_type = 4;
					}
#line 62 "url_parser.rl"
						{
							path_start = p;
						}
						break;
#line 5591 "urlparser.cpp"
					}
				}

			_out:
				{
				}
			}

#line 157 "url_parser.rl"

			if (cs < url_parser_first_final || cs == url_parser_error)
			{
				return nullptr;
			}

			// 计算每个字段的右边界：取"下一个已知起点"的最小值，再回退分隔符
			auto next_field = [&](std::initializer_list<const char *> starts) -> const char *
			{
				const char *end = pe;
				for (auto s : starts)
				{
					if (s && s < end)
					{
						end = s;
					}
				}
				return end;
			};

			if (scheme_start)
			{
				const char *scheme_end = std::find(scheme_start, next_field({host_start, pe}), ':');
				if (scheme_end > scheme_start)
				{
					url->setScheme(std::string(scheme_start, scheme_end));
				}
			}

			if (userinfo_start)
			{
				const char *userinfo_end = next_field({host_start, pe});
				if (userinfo_end > userinfo_start && *(userinfo_end - 1) == '@')
				{
					--userinfo_end; // 去掉 '@'
				}
				if (userinfo_end > userinfo_start)
				{
					std::string userinfo(userinfo_start, userinfo_end);
					url->setUserinfo(userinfo);
					url->setDecodedUserinfo(URLUtils::UrlDecode(userinfo));
				}
			}

			if (host_start)
			{
				const char *host_end = next_field({port_start, path_start, query_start, fragment_start, pe});
				// port_start 指向 ':' 后第一个数字，':' 本身要排除
				if (port_start && host_end == port_start && port_start > host_start)
				{
					--host_end;
				}

				std::string host_str;
				if (host_end > host_start)
				{
					host_str = std::string(host_start, host_end);
				}

				bool hasbrackets = false;
				if (host_str.size() >= 2 && host_str.front() == '[' && host_str.back() == ']')
				{
					host_str = host_str.substr(1, host_str.size() - 2);
					hasbrackets = true;
				}

				if (host_type == 4)
				{
					url->m_hostType = HostType::IPv4;
					url->setHost(host_str);
					inet_pton(AF_INET, host_str.c_str(), url->m_ipBinary.data());
				}
				else if (host_type == 6)
				{
					url->m_hostType = HostType::IPv6;
					url->setHost(hasbrackets ? "[" + host_str + "]" : host_str);
					inet_pton(AF_INET6, host_str.c_str(), url->m_ipBinary.data());
				}
				else
				{
					url->m_hostType = HostType::HOSTNAME;
					url->setUnicodeHost(host_str);
					url->setHost(URLUtils::DomainToASCII(host_str));
				}
			}

			if (port_start)
			{
				const char *port_end = next_field({path_start, query_start, fragment_start, pe});
				if (port_end > port_start)
				{
					std::string port_str(port_start, port_end);
					url->setPort(port_str.empty() ? 0 : std::stoi(port_str));
				}
			}

			if (path_start)
			{
				// path 可能以 '?' 或 '#' 结束，但 path_start 本身是 '/'，不会误含分隔符
				const char *path_end = next_field({query_start, fragment_start, pe});
				if (path_end > path_start && *(path_end - 1) == '?')
					--path_end; // 防御
				if (path_end > path_start)
				{
					std::string path(path_start, path_end);
					url->setPath(path);
					url->setDecodedPath(URLUtils::UrlDecode(path));
				}
			}

			if (query_start)
			{
				const char *query_end = next_field({fragment_start, pe});
				if (query_end > query_start && *(query_end - 1) == '#')
					--query_end;
				if (query_end > query_start)
				{
					std::string query(query_start, query_end);
					url->setQuery(query);
					url->setDecodedQuery(URLUtils::UrlDecode(query));
				}
			}

			if (fragment_start)
			{
				std::string fragment(fragment_start, pe);
				url->setFragment(fragment);
				url->setDecodedFragment(URLUtils::UrlDecode(fragment));
			}
			if (url)
			{
				return url;
			}
			return nullptr;
		}
		catch (...)
		{
			return nullptr;
		}
	}

} // namespace blue