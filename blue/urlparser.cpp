
#line 1 "/home/blue/c_projects/sylar/blue/url_parser.rl"
// url_parser.rl
// 用法: ragel -G2 -o url_parser.cpp url_parser.rl
// 或者: ragel -T0 -o url_parser.cpp url_parser.rl  (表驱动)

#include <iostream>
#include <cstring>
#include <algorithm>
#include "url.h"
#include "urlutils.h"

#line 15 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
static const int url_parser_start = 1;
static const int url_parser_first_final = 238;
static const int url_parser_error = 0;

static const int url_parser_en_main = 1;

#line 14 "/home/blue/c_projects/sylar/blue/url_parser.rl"

namespace blue
{
	std::shared_ptr<Url> Url::CreateUrl(const std::string &url_str)
	{
		auto url = std::make_shared<Url>();

		int cs;
		const char *p = url_str.c_str();
		const char *pe = p + url_str.size();
		const char *eof = pe;

		// 各字段的标记指针
		const char *scheme_start = nullptr;
		const char *userinfo_start = nullptr;
		const char *host_start = nullptr;
		const char *port_start = nullptr;
		const char *path_start = nullptr;
		const char *query_start = nullptr;
		const char *fragment_start = nullptr;

		// 标记 IP 类型
		int host_type = 0; // 0=域名, 4=IPv4, 6=IPv6

#line 50 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
		static const int url_parser_start = 1;
		static const int url_parser_first_final = 238;
		static const int url_parser_error = 0;

		static const int url_parser_en_main = 1;

#line 58 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
		{
			cs = url_parser_start;
		}

#line 63 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
		{
			if (p == pe)
				goto _test_eof;
			switch (cs)
			{
			case 1:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 63:
					goto tr4;
				}
				if ((*p) > 90)
				{
					if (97 <= (*p) && (*p) <= 122)
						goto tr5;
				}
				else if ((*p) >= 65)
					goto tr5;
				goto st0;
			st0:
				cs = 0;
				goto _out;
			tr0:
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				path_start = p;
			}
				goto st238;
			tr9:
#line 46 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				fragment_start = p;
			}
				goto st238;
			tr27:
#line 45 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				query_start = p;
			}
				goto st238;
			tr41:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st238;
			tr67:
#line 43 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				port_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st238;
			st238:
				if (++p == pe)
					goto _test_eof238;
			case 238:
#line 113 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				goto st0;
			tr2:
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				path_start = p;
			}
				goto st2;
			tr29:
#line 45 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				query_start = p;
			}
				goto st2;
			tr43:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st2;
			tr68:
#line 43 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				port_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st2;
			st2:
				if (++p == pe)
					goto _test_eof2;
			case 2:
#line 139 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 0)
					goto tr9;
				if ((*p) < -32)
				{
					if (-62 <= (*p) && (*p) <= -33)
						goto tr6;
				}
				else if ((*p) > -17)
				{
					if ((*p) > -12)
					{
						if (32 <= (*p) && (*p) <= 126)
							goto tr10;
					}
					else if ((*p) >= -16)
						goto tr8;
				}
				else
					goto tr7;
				goto st0;
			tr6:
#line 46 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				fragment_start = p;
			}
				goto st3;
			st3:
				if (++p == pe)
					goto _test_eof3;
			case 3:
#line 162 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st4;
				goto st0;
			tr10:
#line 46 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				fragment_start = p;
			}
				goto st4;
			st4:
				if (++p == pe)
					goto _test_eof4;
			case 4:
#line 174 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 0)
					goto st238;
				if ((*p) < -32)
				{
					if (-62 <= (*p) && (*p) <= -33)
						goto st3;
				}
				else if ((*p) > -17)
				{
					if ((*p) > -12)
					{
						if (32 <= (*p) && (*p) <= 126)
							goto st4;
					}
					else if ((*p) >= -16)
						goto st6;
				}
				else
					goto st5;
				goto st0;
			tr7:
#line 46 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				fragment_start = p;
			}
				goto st5;
			st5:
				if (++p == pe)
					goto _test_eof5;
			case 5:
#line 197 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st3;
				goto st0;
			tr8:
#line 46 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				fragment_start = p;
			}
				goto st6;
			st6:
				if (++p == pe)
					goto _test_eof6;
			case 6:
#line 209 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st5;
				goto st0;
			tr3:
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				path_start = p;
			}
				goto st7;
			tr45:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st7;
			tr69:
#line 43 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				port_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st7;
			st7:
				if (++p == pe)
					goto _test_eof7;
			case 7:
#line 233 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto st238;
				case 33:
					goto st7;
				case 35:
					goto st2;
				case 37:
					goto st11;
				case 61:
					goto st7;
				case 63:
					goto st13;
				case 95:
					goto st7;
				case 126:
					goto st7;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st9;
					}
					else if ((*p) >= -62)
						goto st8;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 64)
					{
						if (36 <= (*p) && (*p) <= 59)
							goto st7;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st7;
					}
					else
						goto st7;
				}
				else
					goto st10;
				goto st0;
			st8:
				if (++p == pe)
					goto _test_eof8;
			case 8:
				if ((*p) <= -65)
					goto st7;
				goto st0;
			st9:
				if (++p == pe)
					goto _test_eof9;
			case 9:
				if ((*p) <= -65)
					goto st8;
				goto st0;
			st10:
				if (++p == pe)
					goto _test_eof10;
			case 10:
				if ((*p) <= -65)
					goto st9;
				goto st0;
			st11:
				if (++p == pe)
					goto _test_eof11;
			case 11:
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st12;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st12;
				}
				else
					goto st12;
				goto st0;
			st12:
				if (++p == pe)
					goto _test_eof12;
			case 12:
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st7;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st7;
				}
				else
					goto st7;
				goto st0;
			tr4:
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				path_start = p;
			}
				goto st13;
			tr51:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st13;
			tr71:
#line 43 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				port_start = p;
			}
#line 44 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					path_start = p;
				}
				goto st13;
			st13:
				if (++p == pe)
					goto _test_eof13;
			case 13:
#line 329 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr27;
				case 35:
					goto tr29;
				}
				if ((*p) < -32)
				{
					if (-62 <= (*p) && (*p) <= -33)
						goto tr24;
				}
				else if ((*p) > -17)
				{
					if ((*p) > -12)
					{
						if (32 <= (*p) && (*p) <= 126)
							goto tr28;
					}
					else if ((*p) >= -16)
						goto tr26;
				}
				else
					goto tr25;
				goto st0;
			tr24:
#line 45 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				query_start = p;
			}
				goto st14;
			st14:
				if (++p == pe)
					goto _test_eof14;
			case 14:
#line 354 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st15;
				goto st0;
			tr28:
#line 45 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				query_start = p;
			}
				goto st15;
			st15:
				if (++p == pe)
					goto _test_eof15;
			case 15:
#line 366 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto st238;
				case 35:
					goto st2;
				}
				if ((*p) < -32)
				{
					if (-62 <= (*p) && (*p) <= -33)
						goto st14;
				}
				else if ((*p) > -17)
				{
					if ((*p) > -12)
					{
						if (32 <= (*p) && (*p) <= 126)
							goto st15;
					}
					else if ((*p) >= -16)
						goto st17;
				}
				else
					goto st16;
				goto st0;
			tr25:
#line 45 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				query_start = p;
			}
				goto st16;
			st16:
				if (++p == pe)
					goto _test_eof16;
			case 16:
#line 391 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st14;
				goto st0;
			tr26:
#line 45 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				query_start = p;
			}
				goto st17;
			st17:
				if (++p == pe)
					goto _test_eof17;
			case 17:
#line 403 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st16;
				goto st0;
			tr5:
#line 40 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				scheme_start = p;
			}
				goto st18;
			st18:
				if (++p == pe)
					goto _test_eof18;
			case 18:
#line 415 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 43:
					goto st18;
				case 58:
					goto st19;
				}
				if ((*p) < 48)
				{
					if (45 <= (*p) && (*p) <= 46)
						goto st18;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st18;
					}
					else if ((*p) >= 65)
						goto st18;
				}
				else
					goto st18;
				goto st0;
			st19:
				if (++p == pe)
					goto _test_eof19;
			case 19:
				if ((*p) == 47)
					goto st20;
				goto st0;
			st20:
				if (++p == pe)
					goto _test_eof20;
			case 20:
				if ((*p) == 47)
					goto st21;
				goto st0;
			st21:
				if (++p == pe)
					goto _test_eof21;
			case 21:
				switch ((*p))
				{
				case 0:
					goto tr41;
				case 33:
					goto tr42;
				case 35:
					goto tr43;
				case 37:
					goto tr44;
				case 47:
					goto tr45;
				case 48:
					goto tr46;
				case 49:
					goto tr47;
				case 50:
					goto tr48;
				case 58:
					goto tr50;
				case 59:
					goto tr42;
				case 61:
					goto tr42;
				case 63:
					goto tr51;
				case 64:
					goto tr52;
				case 91:
					goto tr53;
				case 95:
					goto tr42;
				case 126:
					goto tr42;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto tr38;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto tr40;
					}
					else
						goto tr39;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto tr49;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto tr42;
					}
					else
						goto tr42;
				}
				else
					goto tr42;
				goto st0;
			tr38:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st22;
			st22:
				if (++p == pe)
					goto _test_eof22;
			case 22:
#line 499 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st23;
				goto st0;
			tr42:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st23;
			tr279:
#line 47 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 4;
			}
				goto st23;
			st23:
				if (++p == pe)
					goto _test_eof23;
			case 23:
#line 517 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
				case 95:
					goto st23;
				case 126:
					goto st23;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st24;
					}
					else if ((*p) >= -62)
						goto st22;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
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
					goto st25;
				goto st0;
			tr39:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st24;
			st24:
				if (++p == pe)
					goto _test_eof24;
			case 24:
#line 559 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st22;
				goto st0;
			tr40:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st25;
			st25:
				if (++p == pe)
					goto _test_eof25;
			case 25:
#line 573 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st24;
				goto st0;
			tr44:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st26;
			st26:
				if (++p == pe)
					goto _test_eof26;
			case 26:
#line 587 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 48)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if ((*p) > -12)
						{
							if (36 <= (*p) && (*p) <= 46)
								goto st28;
						}
						else if ((*p) >= -16)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 57)
				{
					if ((*p) < 71)
					{
						if (65 <= (*p) && (*p) <= 70)
							goto st33;
					}
					else if ((*p) > 90)
					{
						if ((*p) > 102)
						{
							if (103 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 97)
							goto st33;
					}
					else
						goto st28;
				}
				else
					goto st33;
				goto st0;
			tr80:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st27;
			st27:
				if (++p == pe)
					goto _test_eof27;
			case 27:
#line 635 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st28;
				goto st0;
			tr83:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st28;
			tr100:
#line 47 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 4;
			}
				goto st28;
			st28:
				if (++p == pe)
					goto _test_eof28;
			case 28:
#line 651 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st29;
					}
					else if ((*p) >= -62)
						goto st27;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
							goto st28;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st30;
				goto st0;
			tr81:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st29;
			st29:
				if (++p == pe)
					goto _test_eof29;
			case 29:
#line 689 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st27;
				goto st0;
			tr82:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st30;
			st30:
				if (++p == pe)
					goto _test_eof30;
			case 30:
#line 701 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) <= -65)
					goto st29;
				goto st0;
			tr88:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st31;
			st31:
				if (++p == pe)
					goto _test_eof31;
			case 31:
#line 713 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr67;
				case 35:
					goto tr68;
				case 47:
					goto tr69;
				case 63:
					goto tr71;
				}
				if (48 <= (*p) && (*p) <= 57)
					goto tr70;
				goto st0;
			tr70:
#line 43 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				port_start = p;
			}
				goto st32;
			st32:
				if (++p == pe)
					goto _test_eof32;
			case 32:
#line 731 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 63:
					goto tr4;
				}
				if (48 <= (*p) && (*p) <= 57)
					goto st32;
				goto st0;
			st33:
				if (++p == pe)
					goto _test_eof33;
			case 33:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 48)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if ((*p) > -12)
						{
							if (36 <= (*p) && (*p) <= 46)
								goto st28;
						}
						else if ((*p) >= -16)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 57)
				{
					if ((*p) < 71)
					{
						if (65 <= (*p) && (*p) <= 70)
							goto st23;
					}
					else if ((*p) > 90)
					{
						if ((*p) > 102)
						{
							if (103 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 97)
							goto st23;
					}
					else
						goto st28;
				}
				else
					goto st23;
				goto st0;
			tr50:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st34;
			st34:
				if (++p == pe)
					goto _test_eof34;
			case 34:
#line 794 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr67;
				case 33:
					goto st36;
				case 35:
					goto tr68;
				case 37:
					goto st39;
				case 47:
					goto tr69;
				case 61:
					goto st36;
				case 63:
					goto tr71;
				case 64:
					goto st41;
				case 95:
					goto st36;
				case 126:
					goto st36;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st35;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st38;
					}
					else
						goto st37;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 58)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto tr78;
					}
					else if ((*p) > 59)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st36;
						}
						else if ((*p) >= 65)
							goto st36;
					}
					else
						goto st36;
				}
				else
					goto st36;
				goto st0;
			st35:
				if (++p == pe)
					goto _test_eof35;
			case 35:
				if ((*p) <= -65)
					goto st36;
				goto st0;
			st36:
				if (++p == pe)
					goto _test_eof36;
			case 36:
				switch ((*p))
				{
				case 33:
					goto st36;
				case 37:
					goto st39;
				case 61:
					goto st36;
				case 64:
					goto st41;
				case 95:
					goto st36;
				case 126:
					goto st36;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st35;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st38;
					}
					else
						goto st37;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 59)
							goto st36;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st36;
					}
					else
						goto st36;
				}
				else
					goto st36;
				goto st0;
			st37:
				if (++p == pe)
					goto _test_eof37;
			case 37:
				if ((*p) <= -65)
					goto st35;
				goto st0;
			st38:
				if (++p == pe)
					goto _test_eof38;
			case 38:
				if ((*p) <= -65)
					goto st37;
				goto st0;
			st39:
				if (++p == pe)
					goto _test_eof39;
			case 39:
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
			tr52:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
				goto st41;
			st41:
				if (++p == pe)
					goto _test_eof41;
			case 41:
#line 919 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr41;
				case 33:
					goto tr83;
				case 35:
					goto tr43;
				case 47:
					goto tr45;
				case 48:
					goto tr84;
				case 49:
					goto tr85;
				case 50:
					goto tr86;
				case 58:
					goto tr88;
				case 59:
					goto tr83;
				case 61:
					goto tr83;
				case 63:
					goto tr51;
				case 91:
					goto tr53;
				case 95:
					goto tr83;
				case 126:
					goto tr83;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto tr80;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto tr82;
					}
					else
						goto tr81;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto tr87;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto tr83;
					}
					else
						goto tr83;
				}
				else
					goto tr83;
				goto st0;
			tr84:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st42;
			st42:
				if (++p == pe)
					goto _test_eof42;
			case 42:
#line 965 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st43;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st29;
					}
					else if ((*p) >= -62)
						goto st27;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
							goto st28;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st30;
				goto st0;
			st43:
				if (++p == pe)
					goto _test_eof43;
			case 43:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 48:
					goto st44;
				case 49:
					goto st52;
				case 50:
					goto st54;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st53;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st44:
				if (++p == pe)
					goto _test_eof44;
			case 44:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st45;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st29;
					}
					else if ((*p) >= -62)
						goto st27;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
							goto st28;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st30;
				goto st0;
			st45:
				if (++p == pe)
					goto _test_eof45;
			case 45:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 48:
					goto st46;
				case 49:
					goto st48;
				case 50:
					goto st50;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (51 <= (*p) && (*p) <= 57)
							goto st49;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st46:
				if (++p == pe)
					goto _test_eof46;
			case 46:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st47;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st29;
					}
					else if ((*p) >= -62)
						goto st27;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
							goto st28;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st30;
				goto st0;
			st47:
				if (++p == pe)
					goto _test_eof47;
			case 47:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto tr100;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st48:
				if (++p == pe)
					goto _test_eof48;
			case 48:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st47;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st49;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st49:
				if (++p == pe)
					goto _test_eof49;
			case 49:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st47;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st46;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st50:
				if (++p == pe)
					goto _test_eof50;
			case 50:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st47;
				case 47:
					goto tr3;
				case 53:
					goto st51;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st49;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 65)
							goto st28;
					}
					else
						goto st46;
				}
				else
					goto st28;
				goto st0;
			st51:
				if (++p == pe)
					goto _test_eof51;
			case 51:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st47;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st46;
					}
					else if ((*p) > 59)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 65)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st52:
				if (++p == pe)
					goto _test_eof52;
			case 52:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st45;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st53;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st53:
				if (++p == pe)
					goto _test_eof53;
			case 53:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st45;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st44;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			st54:
				if (++p == pe)
					goto _test_eof54;
			case 54:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st45;
				case 47:
					goto tr3;
				case 53:
					goto st55;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st53;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 65)
							goto st28;
					}
					else
						goto st44;
				}
				else
					goto st28;
				goto st0;
			st55:
				if (++p == pe)
					goto _test_eof55;
			case 55:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st45;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st44;
					}
					else if ((*p) > 59)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 65)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			tr85:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st56;
			st56:
				if (++p == pe)
					goto _test_eof56;
			case 56:
#line 1505 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st43;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st57;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			tr87:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st57;
			st57:
				if (++p == pe)
					goto _test_eof57;
			case 57:
#line 1548 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st43;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st42;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			tr86:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st58;
			st58:
				if (++p == pe)
					goto _test_eof58;
			case 58:
#line 1591 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st43;
				case 47:
					goto tr3;
				case 53:
					goto st59;
				case 58:
					goto st31;
				case 59:
					goto st28;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st57;
					}
					else if ((*p) > 57)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 65)
							goto st28;
					}
					else
						goto st42;
				}
				else
					goto st28;
				goto st0;
			st59:
				if (++p == pe)
					goto _test_eof59;
			case 59:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st28;
				case 35:
					goto tr2;
				case 46:
					goto st43;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 61:
					goto st28;
				case 63:
					goto tr4;
				case 95:
					goto st28;
				case 126:
					goto st28;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st27;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st30;
					}
					else
						goto st29;
				}
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st42;
					}
					else if ((*p) > 59)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st28;
						}
						else if ((*p) >= 65)
							goto st28;
					}
					else
						goto st28;
				}
				else
					goto st28;
				goto st0;
			tr53:
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_start = p;
			}
				goto st60;
			st60:
				if (++p == pe)
					goto _test_eof60;
			case 60:
#line 1678 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto tr107;
				case 118:
					goto st212;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st61;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st61;
				}
				else
					goto st61;
				goto st0;
			st61:
				if (++p == pe)
					goto _test_eof61;
			case 61:
				if ((*p) == 58)
					goto tr110;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st62;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st62;
				}
				else
					goto st62;
				goto st0;
			st62:
				if (++p == pe)
					goto _test_eof62;
			case 62:
				if ((*p) == 58)
					goto tr110;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st63;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st63;
				}
				else
					goto st63;
				goto st0;
			st63:
				if (++p == pe)
					goto _test_eof63;
			case 63:
				if ((*p) == 58)
					goto tr110;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st64;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st64;
				}
				else
					goto st64;
				goto st0;
			st64:
				if (++p == pe)
					goto _test_eof64;
			case 64:
				if ((*p) == 58)
					goto tr110;
				goto st0;
			tr110:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st65;
			st65:
				if (++p == pe)
					goto _test_eof65;
			case 65:
#line 1752 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st183;
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
			st66:
				if (++p == pe)
					goto _test_eof66;
			case 66:
				if ((*p) == 58)
					goto tr116;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st67;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st67;
				}
				else
					goto st67;
				goto st0;
			st67:
				if (++p == pe)
					goto _test_eof67;
			case 67:
				if ((*p) == 58)
					goto tr116;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st68;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st68;
				}
				else
					goto st68;
				goto st0;
			st68:
				if (++p == pe)
					goto _test_eof68;
			case 68:
				if ((*p) == 58)
					goto tr116;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st69;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st69;
				}
				else
					goto st69;
				goto st0;
			st69:
				if (++p == pe)
					goto _test_eof69;
			case 69:
				if ((*p) == 58)
					goto tr116;
				goto st0;
			tr116:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st70;
			st70:
				if (++p == pe)
					goto _test_eof70;
			case 70:
#line 1824 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st169;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st71;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st71;
				}
				else
					goto st71;
				goto st0;
			st71:
				if (++p == pe)
					goto _test_eof71;
			case 71:
				if ((*p) == 58)
					goto tr122;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st72;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st72;
				}
				else
					goto st72;
				goto st0;
			st72:
				if (++p == pe)
					goto _test_eof72;
			case 72:
				if ((*p) == 58)
					goto tr122;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st73;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st73;
				}
				else
					goto st73;
				goto st0;
			st73:
				if (++p == pe)
					goto _test_eof73;
			case 73:
				if ((*p) == 58)
					goto tr122;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st74;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st74;
				}
				else
					goto st74;
				goto st0;
			st74:
				if (++p == pe)
					goto _test_eof74;
			case 74:
				if ((*p) == 58)
					goto tr122;
				goto st0;
			tr122:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st75;
			st75:
				if (++p == pe)
					goto _test_eof75;
			case 75:
#line 1896 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st155;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st76;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st76;
				}
				else
					goto st76;
				goto st0;
			st76:
				if (++p == pe)
					goto _test_eof76;
			case 76:
				if ((*p) == 58)
					goto tr128;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st77;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st77;
				}
				else
					goto st77;
				goto st0;
			st77:
				if (++p == pe)
					goto _test_eof77;
			case 77:
				if ((*p) == 58)
					goto tr128;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st78;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st78;
				}
				else
					goto st78;
				goto st0;
			st78:
				if (++p == pe)
					goto _test_eof78;
			case 78:
				if ((*p) == 58)
					goto tr128;
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
			st79:
				if (++p == pe)
					goto _test_eof79;
			case 79:
				if ((*p) == 58)
					goto tr128;
				goto st0;
			tr128:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st80;
			st80:
				if (++p == pe)
					goto _test_eof80;
			case 80:
#line 1968 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st141;
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
					goto tr134;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st82;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st82;
				}
				else
					goto st82;
				goto st0;
			st82:
				if (++p == pe)
					goto _test_eof82;
			case 82:
				if ((*p) == 58)
					goto tr134;
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
				if ((*p) == 58)
					goto tr134;
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
				if ((*p) == 58)
					goto tr134;
				goto st0;
			tr134:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st85;
			st85:
				if (++p == pe)
					goto _test_eof85;
			case 85:
#line 2040 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st127;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st86;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st86;
				}
				else
					goto st86;
				goto st0;
			st86:
				if (++p == pe)
					goto _test_eof86;
			case 86:
				if ((*p) == 58)
					goto tr140;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st87;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st87;
				}
				else
					goto st87;
				goto st0;
			st87:
				if (++p == pe)
					goto _test_eof87;
			case 87:
				if ((*p) == 58)
					goto tr140;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st88;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st88;
				}
				else
					goto st88;
				goto st0;
			st88:
				if (++p == pe)
					goto _test_eof88;
			case 88:
				if ((*p) == 58)
					goto tr140;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st89;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st89;
				}
				else
					goto st89;
				goto st0;
			st89:
				if (++p == pe)
					goto _test_eof89;
			case 89:
				if ((*p) == 58)
					goto tr140;
				goto st0;
			tr140:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st90;
			st90:
				if (++p == pe)
					goto _test_eof90;
			case 90:
#line 2112 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 48:
					goto tr143;
				case 49:
					goto tr144;
				case 50:
					goto tr145;
				case 58:
					goto st125;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr146;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr148;
				}
				else
					goto tr148;
				goto st0;
			tr143:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st91;
			st91:
				if (++p == pe)
					goto _test_eof91;
			case 91:
#line 2136 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto tr151;
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
			st92:
				if (++p == pe)
					goto _test_eof92;
			case 92:
				switch ((*p))
				{
				case 48:
					goto st93;
				case 49:
					goto st107;
				case 50:
					goto st109;
				}
				if (51 <= (*p) && (*p) <= 57)
					goto st108;
				goto st0;
			st93:
				if (++p == pe)
					goto _test_eof93;
			case 93:
				if ((*p) == 46)
					goto st94;
				goto st0;
			st94:
				if (++p == pe)
					goto _test_eof94;
			case 94:
				switch ((*p))
				{
				case 48:
					goto st95;
				case 49:
					goto st103;
				case 50:
					goto st105;
				}
				if (51 <= (*p) && (*p) <= 57)
					goto st104;
				goto st0;
			st95:
				if (++p == pe)
					goto _test_eof95;
			case 95:
				if ((*p) == 46)
					goto st96;
				goto st0;
			st96:
				if (++p == pe)
					goto _test_eof96;
			case 96:
				switch ((*p))
				{
				case 48:
					goto tr162;
				case 49:
					goto tr163;
				case 50:
					goto tr164;
				}
				if (51 <= (*p) && (*p) <= 57)
					goto tr165;
				goto st0;
			tr162:
#line 47 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 4;
			}
				goto st97;
			st97:
				if (++p == pe)
					goto _test_eof97;
			case 97:
#line 2208 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 93)
					goto st98;
				goto st0;
			st98:
				if (++p == pe)
					goto _test_eof98;
			case 98:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 35:
					goto tr2;
				case 47:
					goto tr3;
				case 58:
					goto st31;
				case 63:
					goto tr4;
				}
				goto st0;
			tr163:
#line 47 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 4;
			}
				goto st99;
			st99:
				if (++p == pe)
					goto _test_eof99;
			case 99:
#line 2232 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 93)
					goto st98;
				if (48 <= (*p) && (*p) <= 57)
					goto st100;
				goto st0;
			tr165:
#line 47 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 4;
			}
				goto st100;
			st100:
				if (++p == pe)
					goto _test_eof100;
			case 100:
#line 2246 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 93)
					goto st98;
				if (48 <= (*p) && (*p) <= 57)
					goto st97;
				goto st0;
			tr164:
#line 47 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 4;
			}
				goto st101;
			st101:
				if (++p == pe)
					goto _test_eof101;
			case 101:
#line 2260 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 53:
					goto st102;
				case 93:
					goto st98;
				}
				if ((*p) > 52)
				{
					if (54 <= (*p) && (*p) <= 57)
						goto st97;
				}
				else if ((*p) >= 48)
					goto st100;
				goto st0;
			st102:
				if (++p == pe)
					goto _test_eof102;
			case 102:
				if ((*p) == 93)
					goto st98;
				if (48 <= (*p) && (*p) <= 53)
					goto st97;
				goto st0;
			st103:
				if (++p == pe)
					goto _test_eof103;
			case 103:
				if ((*p) == 46)
					goto st96;
				if (48 <= (*p) && (*p) <= 57)
					goto st104;
				goto st0;
			st104:
				if (++p == pe)
					goto _test_eof104;
			case 104:
				if ((*p) == 46)
					goto st96;
				if (48 <= (*p) && (*p) <= 57)
					goto st95;
				goto st0;
			st105:
				if (++p == pe)
					goto _test_eof105;
			case 105:
				switch ((*p))
				{
				case 46:
					goto st96;
				case 53:
					goto st106;
				}
				if ((*p) > 52)
				{
					if (54 <= (*p) && (*p) <= 57)
						goto st95;
				}
				else if ((*p) >= 48)
					goto st104;
				goto st0;
			st106:
				if (++p == pe)
					goto _test_eof106;
			case 106:
				if ((*p) == 46)
					goto st96;
				if (48 <= (*p) && (*p) <= 53)
					goto st95;
				goto st0;
			st107:
				if (++p == pe)
					goto _test_eof107;
			case 107:
				if ((*p) == 46)
					goto st94;
				if (48 <= (*p) && (*p) <= 57)
					goto st108;
				goto st0;
			st108:
				if (++p == pe)
					goto _test_eof108;
			case 108:
				if ((*p) == 46)
					goto st94;
				if (48 <= (*p) && (*p) <= 57)
					goto st93;
				goto st0;
			st109:
				if (++p == pe)
					goto _test_eof109;
			case 109:
				switch ((*p))
				{
				case 46:
					goto st94;
				case 53:
					goto st110;
				}
				if ((*p) > 52)
				{
					if (54 <= (*p) && (*p) <= 57)
						goto st93;
				}
				else if ((*p) >= 48)
					goto st108;
				goto st0;
			st110:
				if (++p == pe)
					goto _test_eof110;
			case 110:
				if ((*p) == 46)
					goto st94;
				if (48 <= (*p) && (*p) <= 53)
					goto st93;
				goto st0;
			st111:
				if (++p == pe)
					goto _test_eof111;
			case 111:
				if ((*p) == 58)
					goto tr151;
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
				if ((*p) == 58)
					goto tr151;
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
				if ((*p) == 58)
					goto tr151;
				goto st0;
			tr151:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st114;
			st114:
				if (++p == pe)
					goto _test_eof114;
			case 114:
#line 2407 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st97;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st115;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st115;
				}
				else
					goto st115;
				goto st0;
			tr181:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st115;
			st115:
				if (++p == pe)
					goto _test_eof115;
			case 115:
#line 2427 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 93)
					goto st98;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st116;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st116;
				}
				else
					goto st116;
				goto st0;
			st116:
				if (++p == pe)
					goto _test_eof116;
			case 116:
				if ((*p) == 93)
					goto st98;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st117;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st117;
				}
				else
					goto st117;
				goto st0;
			st117:
				if (++p == pe)
					goto _test_eof117;
			case 117:
				if ((*p) == 93)
					goto st98;
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
			tr144:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st118;
			st118:
				if (++p == pe)
					goto _test_eof118;
			case 118:
#line 2477 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto tr151;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st119;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st111;
				}
				else
					goto st111;
				goto st0;
			st119:
				if (++p == pe)
					goto _test_eof119;
			case 119:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto tr151;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st120;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
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
					goto st92;
				case 58:
					goto tr151;
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
			tr145:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st121;
			st121:
				if (++p == pe)
					goto _test_eof121;
			case 121:
#line 2533 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st122;
				case 58:
					goto tr151;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st119;
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
					goto st123;
				goto st0;
			st122:
				if (++p == pe)
					goto _test_eof122;
			case 122:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto tr151;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st120;
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
			st123:
				if (++p == pe)
					goto _test_eof123;
			case 123:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto tr151;
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
			tr146:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st124;
			st124:
				if (++p == pe)
					goto _test_eof124;
			case 124:
#line 2596 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto tr151;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st123;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st111;
				}
				else
					goto st111;
				goto st0;
			st125:
				if (++p == pe)
					goto _test_eof125;
			case 125:
				if ((*p) == 93)
					goto st98;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto tr181;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr181;
				}
				else
					goto tr181;
				goto st0;
			tr148:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st126;
			st126:
				if (++p == pe)
					goto _test_eof126;
			case 126:
#line 2633 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto tr151;
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
			st127:
				if (++p == pe)
					goto _test_eof127;
			case 127:
				switch ((*p))
				{
				case 48:
					goto tr182;
				case 49:
					goto tr183;
				case 50:
					goto tr184;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr185;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr186;
				}
				else
					goto tr186;
				goto st0;
			tr182:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st128;
			st128:
				if (++p == pe)
					goto _test_eof128;
			case 128:
#line 2672 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st129;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st129;
				}
				else
					goto st129;
				goto st0;
			st129:
				if (++p == pe)
					goto _test_eof129;
			case 129:
				switch ((*p))
				{
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st130;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st130;
				}
				else
					goto st130;
				goto st0;
			st130:
				if (++p == pe)
					goto _test_eof130;
			case 130:
				switch ((*p))
				{
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st131;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st131;
				}
				else
					goto st131;
				goto st0;
			st131:
				if (++p == pe)
					goto _test_eof131;
			case 131:
				switch ((*p))
				{
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				goto st0;
			st132:
				if (++p == pe)
					goto _test_eof132;
			case 132:
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st115;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st115;
				}
				else
					goto st115;
				goto st0;
			tr183:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st133;
			st133:
				if (++p == pe)
					goto _test_eof133;
			case 133:
#line 2751 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st134;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st129;
				}
				else
					goto st129;
				goto st0;
			st134:
				if (++p == pe)
					goto _test_eof134;
			case 134:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st135;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st130;
				}
				else
					goto st130;
				goto st0;
			st135:
				if (++p == pe)
					goto _test_eof135;
			case 135:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st131;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st131;
				}
				else
					goto st131;
				goto st0;
			tr184:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st136;
			st136:
				if (++p == pe)
					goto _test_eof136;
			case 136:
#line 2810 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st137;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st134;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st129;
					}
					else if ((*p) >= 65)
						goto st129;
				}
				else
					goto st138;
				goto st0;
			st137:
				if (++p == pe)
					goto _test_eof137;
			case 137:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st135;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st130;
					}
					else if ((*p) >= 65)
						goto st130;
				}
				else
					goto st130;
				goto st0;
			st138:
				if (++p == pe)
					goto _test_eof138;
			case 138:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st130;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st130;
				}
				else
					goto st130;
				goto st0;
			tr185:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st139;
			st139:
				if (++p == pe)
					goto _test_eof139;
			case 139:
#line 2876 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st138;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st129;
				}
				else
					goto st129;
				goto st0;
			tr186:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st140;
			st140:
				if (++p == pe)
					goto _test_eof140;
			case 140:
#line 2899 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto st132;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st129;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st129;
				}
				else
					goto st129;
				goto st0;
			st141:
				if (++p == pe)
					goto _test_eof141;
			case 141:
				switch ((*p))
				{
				case 48:
					goto tr195;
				case 49:
					goto tr196;
				case 50:
					goto tr197;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr198;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr199;
				}
				else
					goto tr199;
				goto st0;
			tr195:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st142;
			st142:
				if (++p == pe)
					goto _test_eof142;
			case 142:
#line 2940 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st143;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st143;
				}
				else
					goto st143;
				goto st0;
			st143:
				if (++p == pe)
					goto _test_eof143;
			case 143:
				switch ((*p))
				{
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st144;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st144;
				}
				else
					goto st144;
				goto st0;
			st144:
				if (++p == pe)
					goto _test_eof144;
			case 144:
				switch ((*p))
				{
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st145;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st145;
				}
				else
					goto st145;
				goto st0;
			st145:
				if (++p == pe)
					goto _test_eof145;
			case 145:
				switch ((*p))
				{
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				goto st0;
			st146:
				if (++p == pe)
					goto _test_eof146;
			case 146:
				switch ((*p))
				{
				case 48:
					goto tr182;
				case 49:
					goto tr183;
				case 50:
					goto tr184;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr185;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr186;
				}
				else
					goto tr186;
				goto st0;
			tr196:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st147;
			st147:
				if (++p == pe)
					goto _test_eof147;
			case 147:
#line 3024 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st148;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st143;
				}
				else
					goto st143;
				goto st0;
			st148:
				if (++p == pe)
					goto _test_eof148;
			case 148:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st149;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st144;
				}
				else
					goto st144;
				goto st0;
			st149:
				if (++p == pe)
					goto _test_eof149;
			case 149:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st145;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st145;
				}
				else
					goto st145;
				goto st0;
			tr197:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st150;
			st150:
				if (++p == pe)
					goto _test_eof150;
			case 150:
#line 3083 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st151;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st148;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st143;
					}
					else if ((*p) >= 65)
						goto st143;
				}
				else
					goto st152;
				goto st0;
			st151:
				if (++p == pe)
					goto _test_eof151;
			case 151:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st149;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st144;
					}
					else if ((*p) >= 65)
						goto st144;
				}
				else
					goto st144;
				goto st0;
			st152:
				if (++p == pe)
					goto _test_eof152;
			case 152:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st144;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st144;
				}
				else
					goto st144;
				goto st0;
			tr198:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st153;
			st153:
				if (++p == pe)
					goto _test_eof153;
			case 153:
#line 3149 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st152;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st143;
				}
				else
					goto st143;
				goto st0;
			tr199:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st154;
			st154:
				if (++p == pe)
					goto _test_eof154;
			case 154:
#line 3172 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto st146;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st143;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st143;
				}
				else
					goto st143;
				goto st0;
			st155:
				if (++p == pe)
					goto _test_eof155;
			case 155:
				switch ((*p))
				{
				case 48:
					goto tr208;
				case 49:
					goto tr209;
				case 50:
					goto tr210;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr211;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr212;
				}
				else
					goto tr212;
				goto st0;
			tr208:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st156;
			st156:
				if (++p == pe)
					goto _test_eof156;
			case 156:
#line 3213 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st157;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st157;
				}
				else
					goto st157;
				goto st0;
			st157:
				if (++p == pe)
					goto _test_eof157;
			case 157:
				switch ((*p))
				{
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st158;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st158;
				}
				else
					goto st158;
				goto st0;
			st158:
				if (++p == pe)
					goto _test_eof158;
			case 158:
				switch ((*p))
				{
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st159;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st159;
				}
				else
					goto st159;
				goto st0;
			st159:
				if (++p == pe)
					goto _test_eof159;
			case 159:
				switch ((*p))
				{
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				goto st0;
			st160:
				if (++p == pe)
					goto _test_eof160;
			case 160:
				switch ((*p))
				{
				case 48:
					goto tr195;
				case 49:
					goto tr196;
				case 50:
					goto tr197;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr198;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr199;
				}
				else
					goto tr199;
				goto st0;
			tr209:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st161;
			st161:
				if (++p == pe)
					goto _test_eof161;
			case 161:
#line 3297 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st162;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st157;
				}
				else
					goto st157;
				goto st0;
			st162:
				if (++p == pe)
					goto _test_eof162;
			case 162:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st163;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st158;
				}
				else
					goto st158;
				goto st0;
			st163:
				if (++p == pe)
					goto _test_eof163;
			case 163:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st159;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st159;
				}
				else
					goto st159;
				goto st0;
			tr210:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st164;
			st164:
				if (++p == pe)
					goto _test_eof164;
			case 164:
#line 3356 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st165;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st162;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st157;
					}
					else if ((*p) >= 65)
						goto st157;
				}
				else
					goto st166;
				goto st0;
			st165:
				if (++p == pe)
					goto _test_eof165;
			case 165:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st163;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st158;
					}
					else if ((*p) >= 65)
						goto st158;
				}
				else
					goto st158;
				goto st0;
			st166:
				if (++p == pe)
					goto _test_eof166;
			case 166:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st158;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st158;
				}
				else
					goto st158;
				goto st0;
			tr211:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st167;
			st167:
				if (++p == pe)
					goto _test_eof167;
			case 167:
#line 3422 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st166;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st157;
				}
				else
					goto st157;
				goto st0;
			tr212:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st168;
			st168:
				if (++p == pe)
					goto _test_eof168;
			case 168:
#line 3445 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto st160;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st157;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st157;
				}
				else
					goto st157;
				goto st0;
			st169:
				if (++p == pe)
					goto _test_eof169;
			case 169:
				switch ((*p))
				{
				case 48:
					goto tr221;
				case 49:
					goto tr222;
				case 50:
					goto tr223;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr224;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr225;
				}
				else
					goto tr225;
				goto st0;
			tr221:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st170;
			st170:
				if (++p == pe)
					goto _test_eof170;
			case 170:
#line 3486 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st171;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st171;
				}
				else
					goto st171;
				goto st0;
			st171:
				if (++p == pe)
					goto _test_eof171;
			case 171:
				switch ((*p))
				{
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st172;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st172;
				}
				else
					goto st172;
				goto st0;
			st172:
				if (++p == pe)
					goto _test_eof172;
			case 172:
				switch ((*p))
				{
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st173;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st173;
				}
				else
					goto st173;
				goto st0;
			st173:
				if (++p == pe)
					goto _test_eof173;
			case 173:
				switch ((*p))
				{
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				goto st0;
			st174:
				if (++p == pe)
					goto _test_eof174;
			case 174:
				switch ((*p))
				{
				case 48:
					goto tr208;
				case 49:
					goto tr209;
				case 50:
					goto tr210;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr211;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr212;
				}
				else
					goto tr212;
				goto st0;
			tr222:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st175;
			st175:
				if (++p == pe)
					goto _test_eof175;
			case 175:
#line 3570 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st176;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st171;
				}
				else
					goto st171;
				goto st0;
			st176:
				if (++p == pe)
					goto _test_eof176;
			case 176:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st177;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st172;
				}
				else
					goto st172;
				goto st0;
			st177:
				if (++p == pe)
					goto _test_eof177;
			case 177:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st173;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st173;
				}
				else
					goto st173;
				goto st0;
			tr223:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st178;
			st178:
				if (++p == pe)
					goto _test_eof178;
			case 178:
#line 3629 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st179;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st176;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st171;
					}
					else if ((*p) >= 65)
						goto st171;
				}
				else
					goto st180;
				goto st0;
			st179:
				if (++p == pe)
					goto _test_eof179;
			case 179:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st177;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st172;
					}
					else if ((*p) >= 65)
						goto st172;
				}
				else
					goto st172;
				goto st0;
			st180:
				if (++p == pe)
					goto _test_eof180;
			case 180:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st172;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st172;
				}
				else
					goto st172;
				goto st0;
			tr224:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st181;
			st181:
				if (++p == pe)
					goto _test_eof181;
			case 181:
#line 3695 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st180;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st171;
				}
				else
					goto st171;
				goto st0;
			tr225:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st182;
			st182:
				if (++p == pe)
					goto _test_eof182;
			case 182:
#line 3718 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto st174;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st171;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st171;
				}
				else
					goto st171;
				goto st0;
			st183:
				if (++p == pe)
					goto _test_eof183;
			case 183:
				switch ((*p))
				{
				case 48:
					goto tr234;
				case 49:
					goto tr235;
				case 50:
					goto tr236;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr237;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr238;
				}
				else
					goto tr238;
				goto st0;
			tr234:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st184;
			st184:
				if (++p == pe)
					goto _test_eof184;
			case 184:
#line 3759 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st185;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st185;
				}
				else
					goto st185;
				goto st0;
			st185:
				if (++p == pe)
					goto _test_eof185;
			case 185:
				switch ((*p))
				{
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st186;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st186;
				}
				else
					goto st186;
				goto st0;
			st186:
				if (++p == pe)
					goto _test_eof186;
			case 186:
				switch ((*p))
				{
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st187;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st187;
				}
				else
					goto st187;
				goto st0;
			st187:
				if (++p == pe)
					goto _test_eof187;
			case 187:
				switch ((*p))
				{
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				goto st0;
			st188:
				if (++p == pe)
					goto _test_eof188;
			case 188:
				switch ((*p))
				{
				case 48:
					goto tr221;
				case 49:
					goto tr222;
				case 50:
					goto tr223;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr224;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr225;
				}
				else
					goto tr225;
				goto st0;
			tr235:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st189;
			st189:
				if (++p == pe)
					goto _test_eof189;
			case 189:
#line 3843 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st190;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st185;
				}
				else
					goto st185;
				goto st0;
			st190:
				if (++p == pe)
					goto _test_eof190;
			case 190:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st191;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st186;
				}
				else
					goto st186;
				goto st0;
			st191:
				if (++p == pe)
					goto _test_eof191;
			case 191:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st187;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st187;
				}
				else
					goto st187;
				goto st0;
			tr236:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st192;
			st192:
				if (++p == pe)
					goto _test_eof192;
			case 192:
#line 3902 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st193;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st190;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st185;
					}
					else if ((*p) >= 65)
						goto st185;
				}
				else
					goto st194;
				goto st0;
			st193:
				if (++p == pe)
					goto _test_eof193;
			case 193:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st191;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st186;
					}
					else if ((*p) >= 65)
						goto st186;
				}
				else
					goto st186;
				goto st0;
			st194:
				if (++p == pe)
					goto _test_eof194;
			case 194:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st186;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st186;
				}
				else
					goto st186;
				goto st0;
			tr237:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st195;
			st195:
				if (++p == pe)
					goto _test_eof195;
			case 195:
#line 3968 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st194;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st185;
				}
				else
					goto st185;
				goto st0;
			tr238:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st196;
			st196:
				if (++p == pe)
					goto _test_eof196;
			case 196:
#line 3991 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto st188;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st185;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st185;
				}
				else
					goto st185;
				goto st0;
			tr107:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st197;
			st197:
				if (++p == pe)
					goto _test_eof197;
			case 197:
#line 4013 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				if ((*p) == 58)
					goto st198;
				goto st0;
			st198:
				if (++p == pe)
					goto _test_eof198;
			case 198:
				switch ((*p))
				{
				case 48:
					goto tr248;
				case 49:
					goto tr249;
				case 50:
					goto tr250;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr251;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr252;
				}
				else
					goto tr252;
				goto st0;
			tr248:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st199;
			st199:
				if (++p == pe)
					goto _test_eof199;
			case 199:
#line 4044 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st200;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st200;
				}
				else
					goto st200;
				goto st0;
			st200:
				if (++p == pe)
					goto _test_eof200;
			case 200:
				switch ((*p))
				{
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st201;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st201;
				}
				else
					goto st201;
				goto st0;
			st201:
				if (++p == pe)
					goto _test_eof201;
			case 201:
				switch ((*p))
				{
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st202;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st202;
				}
				else
					goto st202;
				goto st0;
			st202:
				if (++p == pe)
					goto _test_eof202;
			case 202:
				switch ((*p))
				{
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				goto st0;
			st203:
				if (++p == pe)
					goto _test_eof203;
			case 203:
				switch ((*p))
				{
				case 48:
					goto tr234;
				case 49:
					goto tr235;
				case 50:
					goto tr236;
				}
				if ((*p) < 65)
				{
					if (51 <= (*p) && (*p) <= 57)
						goto tr237;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto tr238;
				}
				else
					goto tr238;
				goto st0;
			tr249:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st204;
			st204:
				if (++p == pe)
					goto _test_eof204;
			case 204:
#line 4128 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st205;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st200;
				}
				else
					goto st200;
				goto st0;
			st205:
				if (++p == pe)
					goto _test_eof205;
			case 205:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st206;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st201;
				}
				else
					goto st201;
				goto st0;
			st206:
				if (++p == pe)
					goto _test_eof206;
			case 206:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st202;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st202;
				}
				else
					goto st202;
				goto st0;
			tr250:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st207;
			st207:
				if (++p == pe)
					goto _test_eof207;
			case 207:
#line 4187 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 53:
					goto st208;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 52)
						goto st205;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st200;
					}
					else if ((*p) >= 65)
						goto st200;
				}
				else
					goto st209;
				goto st0;
			st208:
				if (++p == pe)
					goto _test_eof208;
			case 208:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 54)
				{
					if (48 <= (*p) && (*p) <= 53)
						goto st206;
				}
				else if ((*p) > 57)
				{
					if ((*p) > 70)
					{
						if (97 <= (*p) && (*p) <= 102)
							goto st201;
					}
					else if ((*p) >= 65)
						goto st201;
				}
				else
					goto st201;
				goto st0;
			st209:
				if (++p == pe)
					goto _test_eof209;
			case 209:
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st201;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st201;
				}
				else
					goto st201;
				goto st0;
			tr251:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st210;
			st210:
				if (++p == pe)
					goto _test_eof210;
			case 210:
#line 4253 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 46:
					goto st92;
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st209;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st200;
				}
				else
					goto st200;
				goto st0;
			tr252:
#line 48 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				host_type = 6;
			}
				goto st211;
			st211:
				if (++p == pe)
					goto _test_eof211;
			case 211:
#line 4276 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 58:
					goto st203;
				case 93:
					goto st98;
				}
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st200;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st200;
				}
				else
					goto st200;
				goto st0;
			st212:
				if (++p == pe)
					goto _test_eof212;
			case 212:
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st213;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st213;
				}
				else
					goto st213;
				goto st0;
			st213:
				if (++p == pe)
					goto _test_eof213;
			case 213:
				if ((*p) == 46)
					goto st214;
				if ((*p) < 65)
				{
					if (48 <= (*p) && (*p) <= 57)
						goto st213;
				}
				else if ((*p) > 70)
				{
					if (97 <= (*p) && (*p) <= 102)
						goto st213;
				}
				else
					goto st213;
				goto st0;
			st214:
				if (++p == pe)
					goto _test_eof214;
			case 214:
				switch ((*p))
				{
				case 33:
					goto st216;
				case 36:
					goto st216;
				case 61:
					goto st216;
				case 95:
					goto st216;
				case 126:
					goto st216;
				}
				if ((*p) < 38)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st215;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st218;
					}
					else
						goto st217;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 59)
							goto st216;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st216;
					}
					else
						goto st216;
				}
				else
					goto st216;
				goto st0;
			st215:
				if (++p == pe)
					goto _test_eof215;
			case 215:
				if ((*p) <= -65)
					goto st216;
				goto st0;
			st216:
				if (++p == pe)
					goto _test_eof216;
			case 216:
				switch ((*p))
				{
				case 33:
					goto st216;
				case 36:
					goto st216;
				case 61:
					goto st216;
				case 93:
					goto st98;
				case 95:
					goto st216;
				case 126:
					goto st216;
				}
				if ((*p) < 38)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st215;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st218;
					}
					else
						goto st217;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 59)
							goto st216;
					}
					else if ((*p) > 90)
					{
						if (97 <= (*p) && (*p) <= 122)
							goto st216;
					}
					else
						goto st216;
				}
				else
					goto st216;
				goto st0;
			st217:
				if (++p == pe)
					goto _test_eof217;
			case 217:
				if ((*p) <= -65)
					goto st215;
				goto st0;
			st218:
				if (++p == pe)
					goto _test_eof218;
			case 218:
				if ((*p) <= -65)
					goto st217;
				goto st0;
			tr78:
#line 43 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				port_start = p;
			}
				goto st219;
			st219:
				if (++p == pe)
					goto _test_eof219;
			case 219:
#line 4412 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st36;
				case 35:
					goto tr2;
				case 37:
					goto st39;
				case 47:
					goto tr3;
				case 61:
					goto st36;
				case 63:
					goto tr4;
				case 64:
					goto st41;
				case 95:
					goto st36;
				case 126:
					goto st36;
				}
				if ((*p) < 36)
				{
					if ((*p) < -32)
					{
						if (-62 <= (*p) && (*p) <= -33)
							goto st35;
					}
					else if ((*p) > -17)
					{
						if (-16 <= (*p) && (*p) <= -12)
							goto st38;
					}
					else
						goto st37;
				}
				else if ((*p) > 46)
				{
					if ((*p) < 58)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st219;
					}
					else if ((*p) > 59)
					{
						if ((*p) > 90)
						{
							if (97 <= (*p) && (*p) <= 122)
								goto st36;
						}
						else if ((*p) >= 65)
							goto st36;
					}
					else
						goto st36;
				}
				else
					goto st36;
				goto st0;
			tr46:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st220;
			st220:
				if (++p == pe)
					goto _test_eof220;
			case 220:
#line 4459 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st221;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
				case 95:
					goto st23;
				case 126:
					goto st23;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st24;
					}
					else if ((*p) >= -62)
						goto st22;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
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
					goto st25;
				goto st0;
			st221:
				if (++p == pe)
					goto _test_eof221;
			case 221:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 47:
					goto tr3;
				case 48:
					goto st222;
				case 49:
					goto st230;
				case 50:
					goto st232;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
						if (51 <= (*p) && (*p) <= 57)
							goto st231;
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
			st222:
				if (++p == pe)
					goto _test_eof222;
			case 222:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st223;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
				case 95:
					goto st23;
				case 126:
					goto st23;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st24;
					}
					else if ((*p) >= -62)
						goto st22;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
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
					goto st25;
				goto st0;
			st223:
				if (++p == pe)
					goto _test_eof223;
			case 223:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 47:
					goto tr3;
				case 48:
					goto st224;
				case 49:
					goto st226;
				case 50:
					goto st228;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
						if (51 <= (*p) && (*p) <= 57)
							goto st227;
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
			st224:
				if (++p == pe)
					goto _test_eof224;
			case 224:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st225;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
				case 95:
					goto st23;
				case 126:
					goto st23;
				}
				if ((*p) < -16)
				{
					if ((*p) > -33)
					{
						if (-32 <= (*p) && (*p) <= -17)
							goto st24;
					}
					else if ((*p) >= -62)
						goto st22;
				}
				else if ((*p) > -12)
				{
					if ((*p) < 65)
					{
						if (36 <= (*p) && (*p) <= 59)
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
					goto st25;
				goto st0;
			st225:
				if (++p == pe)
					goto _test_eof225;
			case 225:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
						if (48 <= (*p) && (*p) <= 57)
							goto tr279;
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
			st226:
				if (++p == pe)
					goto _test_eof226;
			case 226:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st225;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
							goto st23;
					}
					else
						goto st23;
				}
				else
					goto st23;
				goto st0;
			st227:
				if (++p == pe)
					goto _test_eof227;
			case 227:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st225;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st224;
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
			st228:
				if (++p == pe)
					goto _test_eof228;
			case 228:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st225;
				case 47:
					goto tr3;
				case 53:
					goto st229;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st227;
					}
					else if ((*p) > 57)
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
						goto st224;
				}
				else
					goto st23;
				goto st0;
			st229:
				if (++p == pe)
					goto _test_eof229;
			case 229:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st225;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
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
			st230:
				if (++p == pe)
					goto _test_eof230;
			case 230:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st223;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st231;
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
			st231:
				if (++p == pe)
					goto _test_eof231;
			case 231:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st223;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st222;
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
			st232:
				if (++p == pe)
					goto _test_eof232;
			case 232:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st223;
				case 47:
					goto tr3;
				case 53:
					goto st233;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st231;
					}
					else if ((*p) > 57)
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
						goto st222;
				}
				else
					goto st23;
				goto st0;
			st233:
				if (++p == pe)
					goto _test_eof233;
			case 233:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st223;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st222;
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
			tr47:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st234;
			st234:
				if (++p == pe)
					goto _test_eof234;
			case 234:
#line 5029 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st221;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 65)
					{
						if (48 <= (*p) && (*p) <= 57)
							goto st235;
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
			tr49:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st235;
			st235:
				if (++p == pe)
					goto _test_eof235;
			case 235:
#line 5076 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st221;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
							goto st23;
					}
					else
						goto st23;
				}
				else
					goto st23;
				goto st0;
			tr48:
#line 41 "/home/blue/c_projects/sylar/blue/url_parser.rl"
			{
				userinfo_start = p;
			}
#line 42 "/home/blue/c_projects/sylar/blue/url_parser.rl"
				{
					host_start = p;
				}
				goto st236;
			st236:
				if (++p == pe)
					goto _test_eof236;
			case 236:
#line 5123 "/home/blue/c_projects/sylar/blue/urlparser.cpp"
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st221;
				case 47:
					goto tr3;
				case 53:
					goto st237;
				case 58:
					goto st34;
				case 59:
					goto st23;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 52)
							goto st235;
					}
					else if ((*p) > 57)
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
						goto st220;
				}
				else
					goto st23;
				goto st0;
			st237:
				if (++p == pe)
					goto _test_eof237;
			case 237:
				switch ((*p))
				{
				case 0:
					goto tr0;
				case 33:
					goto st23;
				case 35:
					goto tr2;
				case 37:
					goto st26;
				case 46:
					goto st221;
				case 47:
					goto tr3;
				case 58:
					goto st34;
				case 61:
					goto st23;
				case 63:
					goto tr4;
				case 64:
					goto st41;
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
				else if ((*p) > 45)
				{
					if ((*p) < 54)
					{
						if (48 <= (*p) && (*p) <= 53)
							goto st220;
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
			}
		_test_eof238:
			cs = 238;
			goto _test_eof;
		_test_eof2:
			cs = 2;
			goto _test_eof;
		_test_eof3:
			cs = 3;
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
		_test_eof9:
			cs = 9;
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
		_test_eof17:
			cs = 17;
			goto _test_eof;
		_test_eof18:
			cs = 18;
			goto _test_eof;
		_test_eof19:
			cs = 19;
			goto _test_eof;
		_test_eof20:
			cs = 20;
			goto _test_eof;
		_test_eof21:
			cs = 21;
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
		_test_eof187:
			cs = 187;
			goto _test_eof;
		_test_eof188:
			cs = 188;
			goto _test_eof;
		_test_eof189:
			cs = 189;
			goto _test_eof;
		_test_eof190:
			cs = 190;
			goto _test_eof;
		_test_eof191:
			cs = 191;
			goto _test_eof;
		_test_eof192:
			cs = 192;
			goto _test_eof;
		_test_eof193:
			cs = 193;
			goto _test_eof;
		_test_eof194:
			cs = 194;
			goto _test_eof;
		_test_eof195:
			cs = 195;
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
		_test_eof223:
			cs = 223;
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

		_test_eof:
		{
		}
		_out:
		{
		}
		}

#line 147 "/home/blue/c_projects/sylar/blue/url_parser.rl"

		(void)url_parser_en_main;

		/*
			http://user@www.baidu.com:8080/blue/xxx?query#fragment
			^      ^    ^             ^   ^        ^     ^
			|      |    |             |   |        |     |
		   s_s   u_s  h_s            po_s pth_s    q_s   f_s
		*/
		if (p == pe)
		{
			if (scheme_start)
			{
				const char *scheme_end = std::find(scheme_start, (host_start ? host_start : pe), ':');
				if (scheme_end - scheme_start > 0)
				{
					url->setScheme(std::string(scheme_start, scheme_end));
				}
			}
			if (userinfo_start)
			{
				const char *userinfo_end = (host_start ? host_start - 1 : pe);
				if (userinfo_end - userinfo_start > 0)
				{
					std::string userinfo(userinfo_start, userinfo_end);
					url->setUserinfo(userinfo);
					url->setDecodedUserinfo(URLUtils::UrlDecode(userinfo));
				}
			}
			if (host_start)
			{
				const char *host_end = port_start ? port_start - 1 : (path_start ? path_start : pe);
				std::string host_str;
				if (host_end - host_start > 0)
				{
					host_str = std::string(host_start, host_end); // 正确构造完整字符串
				}

				bool hasbrackets = false;
				// 去掉 IPv6 的方括号
				if (!host_str.empty() && host_str[0] == '[' && host_str.back() == ']')
				{
					host_str = host_str.substr(1, host_str.size() - 2);
					hasbrackets = true;
				}

				// url->setHost(host_str);  // 设置host

				// 转换为二进制存储
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

					// 转换为 IDN ASCII 形式
					std::string ascii_host = URLUtils::DomainToASCII(host_str);
					url->setHost(ascii_host);
				}
			}
			if (port_start)
			{
				const char *port_end = path_start ? path_start : pe;
				if (port_end - port_start > 0)
				{
					std::string port_str(port_start, port_end);
					url->setPort(port_str.empty() ? 0 : std::stoi(port_str));
				}
			}
			if (path_start)
			{
				const char *path_end = query_start ? query_start - 1 : (fragment_start ? fragment_start - 1 : pe);
				if (path_end - path_start > 0)
				{
					std::string path(path_start, path_end);
					url->setPath(path);
					url->setDecodedPath(URLUtils::UrlDecode(path));
				}
			}
			if (query_start)
			{
				const char *query_end = fragment_start ? fragment_start - 1 : pe;
				if (query_end - query_start > 0)
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
			return url;
		}
		return nullptr;
	}

} // namespace blue