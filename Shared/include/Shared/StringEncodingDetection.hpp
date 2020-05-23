#pragma once

#include "Shared/String.hpp"

class BinaryStream;
class Buffer;

class StringEncodingDetector;

// Encoding detection for commonly-used encodings

class StringEncodingDetector final
{
public:
	enum class Encoding
	{
		Unknown = -1,
		// Unicode
		UTF8 = 0,
		// ISO 8859-15
		ISO8859,
		// Japanese
		ShiftJIS,
		// Korean
		CP949,
		MAX_VALUE
	};

	struct Option
	{
		// 0: process the whole input
		size_t maxLookahead = 64;

		// Assume these encodings unless proven invalid.
		// The encodings will be checked sequentially.
		std::vector<Encoding> assumptions = { Encoding::UTF8 };
	};

	static Encoding Detect(const char* str, const Option& option = Option());
	inline static Encoding Detect(String& str, const Option& option = Option()) { return Detect(str.c_str(), option); }
	static Encoding Detect(BinaryStream& stream, const Option& option = Option());

	static String ToUTF8(Encoding encoding, const char* str, size_t str_len);
	static String ToUTF8(const char* encoding, const char* str, size_t str_len);

	inline static String ToUTF8(Encoding encoding, const char* str)
	{
		return ToUTF8(encoding, str, strlen(str));
	}
	inline static String ToUTF8(Encoding encoding, const String& str)
	{
		return ToUTF8(encoding, str.c_str(), str.size());
	}
	inline static String ToUTF8(const char* encoding, const char* str)
	{
		return ToUTF8(encoding, str, strlen(str));
	}
	inline static String ToUTF8(const char* encoding, const String& str)
	{
		return ToUTF8(encoding, str.c_str(), str.size());
	}

	inline static constexpr char* ToString(Encoding encoding);

protected:
	StringEncodingDetector(BinaryStream& stream) : m_stream(stream) {}
	Encoding Detect(const Option& option);

	template<class Heuristic>
	void FeedInput(Heuristic& heuristic, const size_t maxLookahead);

	// Score: negative for invalid, lower is better
	void GetScores(int& score_shift_jis, int& score_cp949);

	BinaryStream& m_stream;

	// Max. bytes to be examined.
	constexpr static size_t MAX_READ_FOR_ENCODING_DETECTION = 64;

	// Size of the buffer for iconv
	constexpr static size_t ICONV_BUFFER_SIZE = 64;
};

inline constexpr char* StringEncodingDetector::ToString(Encoding encoding)
{
	switch (encoding)
	{
	case Encoding::UTF8:
		return "utf-8";
	case Encoding::ISO8859:
		return "iso-8859-15";
	case Encoding::ShiftJIS:
		return "shift_jis";
	case Encoding::CP949:
		return "cp949";
	case Encoding::Unknown:
		return "unknown";
	default:
		return "an unknown encoding";
	}
}

template<class Heuristic>
inline void StringEncodingDetector::FeedInput(Heuristic& heuristic, const size_t maxLookahead)
{
	m_stream.Seek(0);
	if (!heuristic.IsValid())
		return;

	for (size_t i = 0; maxLookahead == 0 || i < maxLookahead; i += sizeof(uint64_t))
	{
		uint64_t data = 0;
		const uint64_t data_len = m_stream.Serialize(&data, sizeof(uint64_t));

		for (uint8_t j = 0; j < data_len; ++j)
		{
			if (!heuristic.Consume(static_cast<uint8_t>(data & 0xFF)))
				return;
			data >>= 8;
		}

		if (data_len < sizeof(uint64_t))
		{
			heuristic.Finalize();
			return;
		}
	}
}
