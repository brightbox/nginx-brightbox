#include "tut.h"
#include "ScgiRequestParser.h"

using namespace Passenger;
using namespace std;

namespace tut {
	struct ScgiRequestParserTest {
		ScgiRequestParser parser;
	};
	
	DEFINE_TEST_GROUP(ScgiRequestParserTest);

	TEST_METHOD(1) {
		// It has an initial state of READING_LENGTH_STRING and does
		// not have anything in its header data buffer.
		ensure_equals(parser.getState(), ScgiRequestParser::READING_LENGTH_STRING);
		ensure(parser.getHeaderData().empty());
	}
	
	/***** Test parsing a complete SCGI request in a single pass. *****/
	
	TEST_METHOD(2) {
		// Parsing a request with a single header and no body.
		static const char data[] = "12:hello\0world\0,";
		ensure_equals("It accepted all input.",
			parser.feed(data, sizeof(data) - 1),
			sizeof(data) - 1);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0", 12));
		ensure(parser.getHeader("hello") == "world");
	}
	
	TEST_METHOD(3) {
		// Parsing a request with a single header and a body.
		static const char data[] = "12:hello\0world\0,data";
		ensure_equals("It accepted all input.",
			parser.feed(data, sizeof(data) - 1 - 4),
			sizeof(data) - 1 - 4);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0", 12));
		ensure(parser.getHeader("hello") == "world");
	}
	
	TEST_METHOD(4) {
		// Parsing a request with multiple headers and no body.
		static const char data[] = "19:hello\0world\0SCGI\0001\0,";
		ensure_equals("It accepted all input.",
			parser.feed(data, sizeof(data) - 1),
			sizeof(data) - 1);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0SCGI\0001\0", 19));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("SCGI") == "1");
	}
	
	TEST_METHOD(5) {
		// Parsing a request with multiple headers and a body.
		static const char data[] = "19:hello\0world\0SCGI\0001\0,body";
		ensure_equals("It accepted all input.",
			parser.feed(data, sizeof(data) - 1 - 4),
			sizeof(data) - 1 - 4);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0SCGI\0001\0", 19));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("SCGI") == "1");
	}
	
	/***** Test parsing a complete SCGI request in multiple passes. *****/
	
	TEST_METHOD(8) {
		// Parsing a request with multiple headers and a body.
		// 1 byte per pass.
		static const char data[] = "20:hello\0world\0foo\0bar\0,data";
		for (unsigned int i = 0; i < sizeof(data) - 1 - 4; i++) {
			ensure_equals(parser.feed(&data[i], 1), 1u);
		}
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0foo\0bar\0", 20));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("foo") == "bar");
	}
	
	TEST_METHOD(9) {
		// Parsing a request with multiple headers and a body.
		// Half element per pass.
		ensure_equals(parser.feed("2", 1), 1u);
		ensure_equals(parser.feed("0", 1), 1u);
		ensure_equals(parser.feed(":", 1), 1u);
		ensure_equals(parser.feed("hello\0world\0", 12), 12u);
		ensure_equals(parser.feed("foo\0bar\0", 8), 8u);
		ensure_equals(parser.feed(",", 1), 1u);
		ensure_equals(parser.feed("da", 2), 0u);
		ensure_equals(parser.feed("ta", 2), 0u);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0foo\0bar\0", 20));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("foo") == "bar");
	}
	
	TEST_METHOD(10) {
		// Parsing a request with multiple headers and a body.
		// 1 element per pass.
		ensure_equals(parser.feed("20", 2), 2u);
		ensure_equals(parser.feed(":", 1), 1u);
		ensure_equals(parser.feed("hello\0world\0foo\0bar\0", 20), 20u);
		ensure_equals(parser.feed(",", 1), 1u);
		ensure_equals(parser.feed("data", 4), 0u);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0foo\0bar\0", 20));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("foo") == "bar");
	}
	
	TEST_METHOD(11) {
		// Parsing a request with multiple headers and a body.
		// 2 elements per pass.
		ensure_equals(parser.feed("20:", 3), 3u);
		ensure_equals(parser.feed("hello\0world\0foo\0bar\0,", 21), 21u);
		ensure_equals(parser.feed("data", 4), 0u);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0foo\0bar\0", 20));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("foo") == "bar");
	}
	
	TEST_METHOD(12) {
		// Parsing a request with multiple headers and a body.
		// Variable number of elements per pass.
		ensure_equals(parser.feed("20:h", 4), 4u);
		ensure_equals(parser.feed("ello\0world\0foo\0bar", 18), 18u);
		ensure_equals(parser.feed("\0,data", 6), 2u);
		ensure_equals("It is in the accepting state.",
			parser.getState(), ScgiRequestParser::DONE);
		ensure_equals("It parsed the header data.",
			parser.getHeaderData(),
			string("hello\0world\0foo\0bar\0", 20));
		ensure(parser.getHeader("hello") == "world");
		ensure(parser.getHeader("foo") == "bar");
	}
	
	/***** Test parsing invalid SCGI requests in one pass. *****/
	
	TEST_METHOD(16) {
		// Invalid first character for length string.
		ensure_equals("Parser did not accept anything.",
			parser.feed("hello world!", 12), 0u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(17) {
		// Invalid character inside length string.
		ensure_equals(parser.feed("12x:hello world!", 16), 2u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(18) {
		// Invalid character in place of colon.
		ensure_equals(parser.feed("12#hello world!", 15), 2u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(19) {
		// Invalid character in place of comma.
		ensure_equals(parser.feed("12:hello\0world\0!", 16), 15u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(20) {
		// Only a header name, without even a null terminator.
		ensure_equals(parser.feed("5:hello,", 8), 7u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(21) {
		// Only a header name, with a null terminator.
		ensure_equals(parser.feed("6:hello\0,", 9), 8u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(22) {
		// A header name with its value not having a null terminator.
		ensure_equals(parser.feed("7:foo\0bar,", 10), 9u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(23) {
		// A header name without corresponding value.
		ensure_equals(parser.feed("10:foo\0bar\0a\0,", 14), 13u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(24) {
		// Length string is too large.
		static const char data[] = "999999999999999999999";
		ensure(parser.feed(data, sizeof(data) - 1) < sizeof(data) - 1);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	/***** Test parsing invalid SCGI requests in multiple passes. *****/
	
	TEST_METHOD(27) {
		// Once the parser has entered the error state, it stays there.
		ensure_equals(parser.feed("hello world!", 12), 0u);
		ensure_equals(parser.feed("1", 1), 0u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(28) {
		// Invalid character inside length string.
		ensure_equals(parser.feed("12", 2), 2u);
		ensure_equals(parser.feed("x:", 2), 0u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(29) {
		// Invalid character in place of colon.
		ensure_equals(parser.feed("12", 2), 2u);
		ensure_equals(parser.feed("#", 1), 0u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(30) {
		// Invalid character in place of comma.
		ensure_equals(parser.feed("12:hello\0world\0", 15), 15u);
		ensure_equals(parser.feed("!", 1), 0u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(31) {
		// Only a header name, without even a null terminator.
		ensure_equals(parser.feed("5:hell", 6), 6u);
		ensure_equals(parser.feed("o,", 1), 1u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(32) {
		// Only a header name, with a null terminator.
		ensure_equals(parser.feed("6:hello", 7), 7u);
		ensure_equals(parser.feed("\0,", 1), 1u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(33) {
		// A header name with its value not having a null terminator.
		ensure_equals(parser.feed("7:foo\0ba", 8), 8u);
		ensure_equals(parser.feed("r,", 2), 1u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(34) {
		// A header name without corresponding value.
		ensure_equals(parser.feed("10:foo\0bar\0a", 12), 12u);
		ensure_equals(parser.feed("\0,", 2), 1u);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	TEST_METHOD(35) {
		// Length string is too large.
		static const char data[] = "999999999999999999999";
		ensure_equals(parser.feed("99", 2), 2u);
		ensure(parser.feed(data, sizeof(data) - 1) < sizeof(data) - 1);
		ensure_equals("Parser is in the error state.",
			parser.getState(), ScgiRequestParser::ERROR);
	}
	
	/***** Test parsing incomplete SCGI requests. *****/
	
	TEST_METHOD(40) {
		// Incomplete length string.
		ensure_equals(parser.feed("2", 1), 1u);
		ensure_equals("Parser is still waiting for length string input.",
			parser.getState(), ScgiRequestParser::READING_LENGTH_STRING);
	}
	
	TEST_METHOD(41) {
		// Incomplete header.
		ensure_equals(parser.feed("21:", 3), 3u);
		ensure_equals("Parser is waiting for header data input.",
			parser.getState(), ScgiRequestParser::READING_HEADER_DATA);
	}
	
	TEST_METHOD(42) {
		// Incomplete header.
		ensure_equals(parser.feed("20:hel", 6), 6u);
		ensure_equals("Parser is waiting for header data input.",
			parser.getState(), ScgiRequestParser::READING_HEADER_DATA);
	}
	
	TEST_METHOD(43) {
		// Complete header but no comma.
		ensure_equals(parser.feed("8:foo\0bar\0", 10), 10u);
		ensure_equals("Parser is waiting for comma.",
			parser.getState(), ScgiRequestParser::EXPECTING_COMMA);
	}
}
